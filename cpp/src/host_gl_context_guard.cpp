#include "host_gl_context_guard.hpp"

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
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

const WglApi& wgl_api() {
    static const WglApi api = [] {
        WglApi a;
        // Only inspect opengl32 if something already loaded it: a Slint build
        // on the Skia/D3D path has no WGL context for us to preserve.
        HMODULE module = GetModuleHandleW(L"opengl32.dll");
        if (!module) {
            return a;
        }
        a.get_current_dc = reinterpret_cast<decltype(a.get_current_dc)>(
            reinterpret_cast<void*>(GetProcAddress(module, "wglGetCurrentDC")));
        a.get_current_context =
            reinterpret_cast<decltype(a.get_current_context)>(
                reinterpret_cast<void*>(
                    GetProcAddress(module, "wglGetCurrentContext")));
        a.make_current = reinterpret_cast<decltype(a.make_current)>(
            reinterpret_cast<void*>(GetProcAddress(module, "wglMakeCurrent")));
        return a;
    }();
    return api;
}

#else

// Look a symbol up in a library the process has *already* loaded. Returning
// null when it is absent is the point: a toolkit that never pulled in EGL (or
// GLX) has no context of its own that needs preserving.
void* loaded_symbol(const char* library, const char* name) {
    void* handle = dlopen(library, RTLD_LAZY | RTLD_NOLOAD);
    if (!handle) {
        return nullptr;
    }
    void* symbol = dlsym(handle, name);
    dlclose(handle);
    return symbol;
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

const EglApi& egl_api() {
    static const EglApi api = [] {
        EglApi a;
        a.get_current_display =
            reinterpret_cast<decltype(a.get_current_display)>(
                loaded_symbol("libEGL.so.1", "eglGetCurrentDisplay"));
        a.get_current_context =
            reinterpret_cast<decltype(a.get_current_context)>(
                loaded_symbol("libEGL.so.1", "eglGetCurrentContext"));
        a.get_current_surface =
            reinterpret_cast<decltype(a.get_current_surface)>(
                loaded_symbol("libEGL.so.1", "eglGetCurrentSurface"));
        a.make_current = reinterpret_cast<decltype(a.make_current)>(
            loaded_symbol("libEGL.so.1", "eglMakeCurrent"));
        return a;
    }();
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

const GlxApi& glx_api() {
    static const GlxApi api = [] {
        GlxApi a;
        a.get_current_display =
            reinterpret_cast<decltype(a.get_current_display)>(
                loaded_symbol("libGLX.so.0", "glXGetCurrentDisplay"));
        a.get_current_context =
            reinterpret_cast<decltype(a.get_current_context)>(
                loaded_symbol("libGLX.so.0", "glXGetCurrentContext"));
        a.get_current_drawable =
            reinterpret_cast<decltype(a.get_current_drawable)>(
                loaded_symbol("libGLX.so.0", "glXGetCurrentDrawable"));
        a.get_current_read_drawable =
            reinterpret_cast<decltype(a.get_current_read_drawable)>(
                loaded_symbol("libGLX.so.0", "glXGetCurrentReadDrawable"));
        a.make_context_current =
            reinterpret_cast<decltype(a.make_context_current)>(
                loaded_symbol("libGLX.so.0", "glXMakeContextCurrent"));
        return a;
    }();
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
#else
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
#if defined(_WIN32)
    const WglApi& wgl = wgl_api();
    if (wgl_context && wgl.usable() &&
        wgl.get_current_context() != wgl_context) {
        wgl.make_current(wgl_device_context, wgl_context);
    }
#else
    const EglApi& egl = egl_api();
    if (egl_context && egl.usable() &&
        egl.get_current_context() != egl_context) {
        egl.make_current(egl_display, egl_draw, egl_read, egl_context);
    }

    const GlxApi& glx = glx_api();
    if (glx_context && glx.usable() &&
        glx.get_current_context() != glx_context) {
        glx.make_context_current(glx_display, glx_draw, glx_read, glx_context);
    }
#endif
}

}  // namespace mbgl_slint
