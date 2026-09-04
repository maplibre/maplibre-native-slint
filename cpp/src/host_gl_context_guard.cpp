#include "host_gl_context_guard.hpp"

#if defined(_WIN32)
#include <windows.h>
#elif !defined(__APPLE__)
#include <dlfcn.h>
#include <initializer_list>
#endif

namespace mbgl_slint {
namespace {

#if defined(_WIN32)

struct WglApi {
    void* (*get_current_dc)() = nullptr;
    void* (*get_current_context)() = nullptr;
    int (*make_current)(void*, void*) = nullptr;

    bool usable() const {
        return get_current_dc && get_current_context && make_current;
    }
};

WglApi resolve_wgl_api() {
    WglApi api;
    // Only inspect opengl32 if something already loaded it: a Slint build on
    // the Skia/D3D path has no WGL context for us to preserve.
    HMODULE module = GetModuleHandleW(L"opengl32.dll");
    if (!module) {
        return api;
    }
    api.get_current_dc = reinterpret_cast<decltype(api.get_current_dc)>(
        reinterpret_cast<void*>(GetProcAddress(module, "wglGetCurrentDC")));
    api.get_current_context =
        reinterpret_cast<decltype(api.get_current_context)>(
            reinterpret_cast<void*>(
                GetProcAddress(module, "wglGetCurrentContext")));
    api.make_current = reinterpret_cast<decltype(api.make_current)>(
        reinterpret_cast<void*>(GetProcAddress(module, "wglMakeCurrent")));
    return api;
}

const WglApi& wgl_api() {
    // Cache only a *successful* resolution, so that a guard constructed before
    // the toolkit brought up its GL stack does not poison later ones.
    static WglApi api;
    if (!api.usable()) {
        api = resolve_wgl_api();
    }
    return api;
}

#elif !defined(__APPLE__)

// Resolve `name` from the first of `libraries` that the process has *already*
// loaded. RTLD_NOLOAD only consults the loaded-object list, so this never
// pulls in a GL stack that is not in use, and a toolkit that never brought EGL
// (or GLX) in has no context of its own that needs preserving.
//
// The handle is deliberately not released: holding a reference for the
// lifetime of the process keeps the resolved entry points valid without having
// to reason about who else still owns the library.
void* loaded_symbol(std::initializer_list<const char*> libraries,
                    const char* name) {
    for (const char* library : libraries) {
        void* handle = dlopen(library, RTLD_LAZY | RTLD_NOLOAD);
        if (!handle) {
            continue;
        }
        if (void* symbol = dlsym(handle, name)) {
            return symbol;
        }
    }
    return nullptr;
}

// From <EGL/egl.h>; spelled out so this file needs no EGL headers.
constexpr int EGL_DRAW_ATTRIBUTE = 0x3059;
constexpr int EGL_READ_ATTRIBUTE = 0x305A;

struct EglApi {
    void* (*get_current_display)() = nullptr;
    void* (*get_current_context)() = nullptr;
    void* (*get_current_surface)(int) = nullptr;
    unsigned int (*make_current)(void*, void*, void*, void*) = nullptr;

    bool usable() const {
        return get_current_display && get_current_context &&
               get_current_surface && make_current;
    }
};

EglApi resolve_egl_api() {
    const std::initializer_list<const char*> libraries = {"libEGL.so.1",
                                                          "libEGL.so"};
    EglApi api;
    api.get_current_display =
        reinterpret_cast<decltype(api.get_current_display)>(
            loaded_symbol(libraries, "eglGetCurrentDisplay"));
    api.get_current_context =
        reinterpret_cast<decltype(api.get_current_context)>(
            loaded_symbol(libraries, "eglGetCurrentContext"));
    api.get_current_surface =
        reinterpret_cast<decltype(api.get_current_surface)>(
            loaded_symbol(libraries, "eglGetCurrentSurface"));
    api.make_current = reinterpret_cast<decltype(api.make_current)>(
        loaded_symbol(libraries, "eglMakeCurrent"));
    return api;
}

const EglApi& egl_api() {
    // Cache only a *successful* resolution, so that a guard constructed before
    // the toolkit brought up its GL stack does not poison later ones.
    static EglApi api;
    if (!api.usable()) {
        api = resolve_egl_api();
    }
    return api;
}

struct GlxApi {
    void* (*get_current_display)() = nullptr;
    void* (*get_current_context)() = nullptr;
    unsigned long (*get_current_drawable)() = nullptr;
    unsigned long (*get_current_read_drawable)() = nullptr;
    int (*make_context_current)(void*, unsigned long, unsigned long,
                                void*) = nullptr;

    bool usable() const {
        return get_current_display && get_current_context &&
               get_current_drawable && get_current_read_drawable &&
               make_context_current;
    }
};

GlxApi resolve_glx_api() {
    // libGLX.so.0 only exists under glvnd, where libGL.so.1 pulls it in. On a
    // classic Mesa stack the glX* entry points live in libGL.so.1 itself, so
    // fall back to it rather than silently skipping GLX there.
    const std::initializer_list<const char*> libraries = {"libGLX.so.0",
                                                          "libGL.so.1"};
    GlxApi api;
    api.get_current_display =
        reinterpret_cast<decltype(api.get_current_display)>(
            loaded_symbol(libraries, "glXGetCurrentDisplay"));
    api.get_current_context =
        reinterpret_cast<decltype(api.get_current_context)>(
            loaded_symbol(libraries, "glXGetCurrentContext"));
    api.get_current_drawable =
        reinterpret_cast<decltype(api.get_current_drawable)>(
            loaded_symbol(libraries, "glXGetCurrentDrawable"));
    api.get_current_read_drawable =
        reinterpret_cast<decltype(api.get_current_read_drawable)>(
            loaded_symbol(libraries, "glXGetCurrentReadDrawable"));
    api.make_context_current =
        reinterpret_cast<decltype(api.make_context_current)>(
            loaded_symbol(libraries, "glXMakeContextCurrent"));
    return api;
}

const GlxApi& glx_api() {
    static GlxApi api;
    if (!api.usable()) {
        api = resolve_glx_api();
    }
    return api;
}

#endif

}  // namespace

HostGLContextGuard::HostGLContextGuard() {
#if defined(_WIN32)
    const WglApi& wgl = wgl_api();
    if (wgl.usable()) {
        wgl_context = wgl.get_current_context();
        if (wgl_context) {
            wgl_device_context = wgl.get_current_dc();
        }
    }
#elif !defined(__APPLE__)
    const EglApi& egl = egl_api();
    if (egl.usable()) {
        egl_context = egl.get_current_context();
        if (egl_context) {
            egl_display = egl.get_current_display();
            egl_draw = egl.get_current_surface(EGL_DRAW_ATTRIBUTE);
            egl_read = egl.get_current_surface(EGL_READ_ATTRIBUTE);
        }
    }

    const GlxApi& glx = glx_api();
    if (glx.usable()) {
        glx_context = glx.get_current_context();
        if (glx_context) {
            glx_display = glx.get_current_display();
            glx_draw = glx.get_current_drawable();
            glx_read = glx.get_current_read_drawable();
        }
    }
#endif
}

HostGLContextGuard::~HostGLContextGuard() {
    // A captured context implies the constructor resolved the API, so the
    // lookups below always hit the cache and never retry.
#if defined(_WIN32)
    if (wgl_context) {
        const WglApi& wgl = wgl_api();
        if (wgl.get_current_context() != wgl_context) {
            wgl.make_current(wgl_device_context, wgl_context);
        }
    }
#elif !defined(__APPLE__)
    if (egl_context) {
        const EglApi& egl = egl_api();
        if (egl.get_current_context() != egl_context) {
            egl.make_current(egl_display, egl_draw, egl_read, egl_context);
        }
    }

    if (glx_context) {
        const GlxApi& glx = glx_api();
        if (glx.get_current_context() != glx_context) {
            glx.make_context_current(glx_display, glx_draw, glx_read,
                                     glx_context);
        }
    }
#endif
}

}  // namespace mbgl_slint
