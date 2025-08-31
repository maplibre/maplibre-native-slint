// Windows OpenGL backend and renderer frontend split from slint_maplibre_gl.cpp
#include "gl/slint_gl_maplibre.hpp"

#ifdef _WIN32
#include <GL/gl.h>
#include <windows.h>
// Legacy Windows GL headers may miss some enums; provide fallbacks.
#ifndef APIENTRYP
#ifndef APIENTRY
#define APIENTRY __stdcall
#endif
#define APIENTRYP APIENTRY*
#endif
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_FRAMEBUFFER_BINDING
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#endif
#endif

#include <mbgl/gfx/backend_scope.hpp>
#include <mbgl/renderer/renderer.hpp>

using namespace std;

namespace mlns {

#ifdef _WIN32
// Minimal loader for GL/ES symbols with ANGLE/EGL fallback
static FARPROC load_gl_proc(const char* name) {
    using PFNWGLGETCURRENTCONTEXTPROC = HGLRC(WINAPI*)();
    static PFNWGLGETCURRENTCONTEXTPROC p_wglGetCurrentContext = nullptr;
    if (!p_wglGetCurrentContext) {
        HMODULE gl = GetModuleHandleA("opengl32.dll");
        if (gl) {
            p_wglGetCurrentContext =
                reinterpret_cast<PFNWGLGETCURRENTCONTEXTPROC>(
                    GetProcAddress(gl, "wglGetCurrentContext"));
        }
    }
    const bool have_wgl =
        p_wglGetCurrentContext && p_wglGetCurrentContext() != nullptr;
    auto bad_ptr = [](FARPROC p) {
        return p == reinterpret_cast<FARPROC>(1) ||
               p == reinterpret_cast<FARPROC>(2) ||
               p == reinterpret_cast<FARPROC>(3) ||
               p == reinterpret_cast<FARPROC>(-1);
    };
    if (have_wgl) {
        HMODULE lib = GetModuleHandleA("opengl32.dll");
        FARPROC p = nullptr;
        if (lib) {
            using PFNWGLGETPROCADDRESSPROC = PROC(WINAPI*)(LPCSTR);
            auto wglGetProcAddress_fn =
                reinterpret_cast<PFNWGLGETPROCADDRESSPROC>(
                    GetProcAddress(lib, "wglGetProcAddress"));
            if (wglGetProcAddress_fn) {
                p = reinterpret_cast<FARPROC>(wglGetProcAddress_fn(name));
                if (bad_ptr(p))
                    p = nullptr;
            }
            if (!p)
                p = GetProcAddress(lib, name);
        }
        return p;
    }
    auto env_truthy = [](const char* v) {
        if (!v)
            return false;
        std::string s(v);
        for (auto& c : s)
            c = (char)tolower((unsigned char)c);
        return (s == "1" || s == "true" || s == "on" || s == "yes");
    };
    if (env_truthy(std::getenv("MLNS_FORCE_DESKTOP_GL"))) {
        return nullptr;
    }
    using PFNEGLGETPROCADDRESSPROC = void*(WINAPI*)(const char*);
    static PFNEGLGETPROCADDRESSPROC p_eglGetProcAddress = nullptr;
    static bool egl_loaded = false;
    if (!egl_loaded) {
        HMODULE egl = GetModuleHandleA("libEGL.dll");
        if (!egl)
            egl = GetModuleHandleA("EGL.dll");
        if (!egl)
            egl = LoadLibraryA("libEGL.dll");
        if (!egl)
            egl = LoadLibraryA("EGL.dll");
        if (egl)
            p_eglGetProcAddress = reinterpret_cast<PFNEGLGETPROCADDRESSPROC>(
                GetProcAddress(egl, "eglGetProcAddress"));
        egl_loaded = true;
    }
    void* vp = nullptr;
    HMODULE gles = GetModuleHandleA("libGLESv2.dll");
    if (!gles)
        gles = GetModuleHandleA("GLESv2.dll");
    if (!gles)
        gles = LoadLibraryA("libGLESv2.dll");
    if (!gles)
        gles = LoadLibraryA("GLESv2.dll");
    if (gles)
        vp = reinterpret_cast<void*>(GetProcAddress(gles, name));
    if (!vp && p_eglGetProcAddress)
        vp = p_eglGetProcAddress(name);
    return reinterpret_cast<FARPROC>(vp);
}

// Ultra-safe mode stubs (duplicated with internal linkage)
typedef unsigned int GLbitfield;
typedef ptrdiff_t GLintptr;
typedef ptrdiff_t GLsizeiptr;
typedef char GLchar;
static const GLubyte* APIENTRY stub_glGetStringi(GLenum, GLuint) {
    static const GLubyte empty[] = "";
    return empty;
}
static void APIENTRY stub_glBindFragDataLocation(GLuint, GLuint,
                                                 const GLchar*) {
}
static void* APIENTRY stub_glMapBufferRange(GLenum, GLintptr, GLsizeiptr,
                                            GLbitfield) {
    return nullptr;
}
static void APIENTRY stub_glFlushMappedBufferRange(GLenum, GLintptr,
                                                   GLsizeiptr) {
}
static void APIENTRY stub_glInvalidateFramebuffer(GLenum, GLsizei,
                                                  const GLenum*) {
}
static void APIENTRY stub_glInvalidateSubFramebuffer(GLenum, GLsizei,
                                                     const GLenum*, GLint,
                                                     GLint, GLsizei, GLsizei) {
}
#endif

// Renderable that ensures our FBO/viewport are bound when MapLibre draws
class SlintGLRenderableResource final : public mbgl::gfx::RenderableResource {
public:
    explicit SlintGLRenderableResource(class SlintGLBackend& backend_)
        : backend(backend_) {
    }
    void bind() override;

private:
    class SlintGLBackend& backend;
};

SlintGLBackend::SlintGLBackend(const mbgl::gfx::ContextMode mode)
    : mbgl::gl::RendererBackend(mode),
      mbgl::gfx::Renderable(
          {0, 0}, std::make_unique<SlintGLRenderableResource>(*this)) {
}

SlintGLBackend::~SlintGLBackend() = default;

void SlintGLRenderableResource::bind() {
    assert(mbgl::gfx::BackendScope::exists());
    auto& b = backend;
#ifdef _WIN32
    // Bind our FBO if the function is available; legacy headers don't declare
    // it.
    using PFNGLBINDFRAMEBUFFERPROC = void(APIENTRYP)(GLenum, GLuint);
    static PFNGLBINDFRAMEBUFFERPROC p_glBindFramebuffer = nullptr;
    if (!p_glBindFramebuffer) {
        p_glBindFramebuffer = reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>(
            load_gl_proc("glBindFramebuffer"));
        if (!p_glBindFramebuffer) {
            p_glBindFramebuffer = reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>(
                load_gl_proc("glBindFramebufferEXT"));
        }
    }
    if (p_glBindFramebuffer) {
        p_glBindFramebuffer(GL_FRAMEBUFFER, b.currentFBO());
    }
#else
    glBindFramebuffer(GL_FRAMEBUFFER, b.currentFBO());
#endif
    glViewport(0, 0, static_cast<GLsizei>(b.getSize().width),
               static_cast<GLsizei>(b.getSize().height));
    b.setFramebufferBinding(static_cast<unsigned int>(b.currentFBO()));
    b.setViewport(0, 0, b.getSize());
}

void SlintGLBackend::updateAssumedState() {
#ifdef _WIN32
    GLint fbo = 0;
    GLint vp[4] = {0, 0, static_cast<GLint>(size.width),
                   static_cast<GLint>(size.height)};
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
    glGetIntegerv(GL_VIEWPORT, vp);
    assumeFramebufferBinding(static_cast<unsigned int>(fbo));
    assumeViewport(
        vp[0], vp[1],
        {static_cast<uint32_t>(vp[2]), static_cast<uint32_t>(vp[3])});
#else
    assumeFramebufferBinding(ImplicitFramebufferBinding);
    assumeViewport(0, 0, size);
#endif
}

void SlintGLBackend::updateFramebuffer(uint32_t fbo,
                                       const mbgl::Size& newSize) {
    fbo_ = fbo;
    size = newSize;
}

mbgl::gl::ProcAddress SlintGLBackend::getExtensionFunctionPointer(
    const char* name) {
#ifdef _WIN32
    static int safe_mode_level = []() {
        const char* v = std::getenv("MLNS_GL_SAFE_MODE");
        if (!v)
            return 0;
        std::string s(v);
        for (auto& c : s)
            c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
        if (s == "2")
            return 2;
        if (s == "1" || s == "true" || s == "on" || s == "yes")
            return 1;
        return 0;
    }();
    if (safe_mode_level >= 1) {
        // Level 1: Disable only debug output related APIs
        static const char* deny[] = {"glDebugMessageControl",
                                     "glDebugMessageCallback",
                                     "glDebugMessageControlARB",
                                     "glDebugMessageCallbackARB",
                                     "glPushDebugGroup",
                                     "glPopDebugGroup",
                                     nullptr};
        for (int i = 0; deny[i]; ++i) {
            if (std::strcmp(name, deny[i]) == 0) {
                std::cout << "[SlintGLBackend] getProcAddress('" << name
                          << "') => forced null (safe mode)" << std::endl;
                return nullptr;
            }
        }
    }
    if (safe_mode_level >= 2) {
        if (std::strcmp(name, "glGetStringi") == 0)
            return reinterpret_cast<mbgl::gl::ProcAddress>(stub_glGetStringi);
        if (std::strcmp(name, "glBindFragDataLocation") == 0)
            return reinterpret_cast<mbgl::gl::ProcAddress>(
                stub_glBindFragDataLocation);
        if (std::strcmp(name, "glInvalidateFramebuffer") == 0)
            return reinterpret_cast<mbgl::gl::ProcAddress>(
                stub_glInvalidateFramebuffer);
        if (std::strcmp(name, "glInvalidateSubFramebuffer") == 0)
            return reinterpret_cast<mbgl::gl::ProcAddress>(
                stub_glInvalidateSubFramebuffer);
        if (std::strcmp(name, "glMapBufferRange") == 0)
            return reinterpret_cast<mbgl::gl::ProcAddress>(
                stub_glMapBufferRange);
        if (std::strcmp(name, "glFlushMappedBufferRange") == 0)
            return reinterpret_cast<mbgl::gl::ProcAddress>(
                stub_glFlushMappedBufferRange);
    }
    // No unconditional deny beyond safe-mode handling
    auto p = load_gl_proc(name);
    return reinterpret_cast<mbgl::gl::ProcAddress>(p);
#else
    return nullptr;
#endif
}

// Frontend
SlintRendererFrontend::SlintRendererFrontend(
    std::unique_ptr<mbgl::Renderer> renderer_,
    mbgl::gfx::RendererBackend& backend_)
    : backend(backend_), renderer(std::move(renderer_)) {
}

SlintRendererFrontend::~SlintRendererFrontend() = default;

void SlintRendererFrontend::reset() {
    renderer.reset();
}

void SlintRendererFrontend::setObserver(mbgl::RendererObserver& observer) {
    if (renderer)
        renderer->setObserver(&observer);
}

void SlintRendererFrontend::update(
    std::shared_ptr<mbgl::UpdateParameters> params) {
    updateParameters = std::move(params);
}

const mbgl::TaggedScheduler& SlintRendererFrontend::getThreadPool() const {
    return backend.getThreadPool();
}

void SlintRendererFrontend::render() {
    if (!renderer || !updateParameters)
        return;
    mbgl::gfx::BackendScope guard{backend,
                                  mbgl::gfx::BackendScope::ScopeType::Implicit};
    auto params = updateParameters;
    renderer->render(params);
}

mbgl::Renderer* SlintRendererFrontend::getRenderer() {
    return renderer.get();
}

}  // namespace mlns
