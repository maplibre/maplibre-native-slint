#include "gl/slint_gl_maplibre.hpp"

#include <cassert>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <mbgl/gfx/backend_scope.hpp>
#include <mbgl/gfx/renderable.hpp>
#include <sstream>
#ifdef _MSC_VER
#include <eh.h>
#endif

// Forward declare guard for use before its definition in some compilers
static bool is_executable_ptr(const void* p);
static bool looks_like_vftable(const void* vft_ptr);
#ifdef _WIN32
static void log_vftable(const char* tag, const void* vft_ptr);
#endif
#ifdef _WIN32
enum VptrGuardMode { GUARD_OFF = 0, GUARD_WARN = 1, GUARD_STRICT = 2 };
static int get_vptr_guard_mode() {
    static int mode = []() {
        const char* v = std::getenv("MLNS_GL_VPTR_GUARD");
        if (!v)
            return GUARD_WARN;  // default warn-only
        std::string s(v);
        for (auto& c : s)
            c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
        if (s == "0" || s == "false" || s == "off" || s == "no")
            return GUARD_OFF;
        if (s == "2" || s == "strict")
            return GUARD_STRICT;
        return GUARD_WARN;  // 1/on/true/warn
    }();
    return mode;
}
#endif

// was early misplaced; definitions moved below within namespace mlns where GL
// helpers are available
#ifdef _WIN32
#include <GL/gl.h>
#include <windows.h>
// Provide missing calling convention macro used in extension typedefs
#ifndef APIENTRYP
#ifndef APIENTRY
#define APIENTRY __stdcall
#endif
#define APIENTRYP APIENTRY*
#endif
// Older GL headers on Windows may not define these pointer-sized types
#include <cstddef>
#ifndef GLintptr
typedef ptrdiff_t GLintptr;
#endif
#ifndef GLsizeiptr
typedef ptrdiff_t GLsizeiptr;
#endif
#ifndef GLbitfield
typedef unsigned int GLbitfield;
#endif
#ifndef GLchar
typedef char GLchar;
#endif
// Define missing enums for GL 1.1 headers on Windows
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_DRAW_FRAMEBUFFER
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#endif
#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER 0x8CA8
#endif
#ifndef GL_RENDERBUFFER
#define GL_RENDERBUFFER 0x8D41
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#endif
#ifndef GL_DEPTH_STENCIL_ATTACHMENT
#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif
#ifndef GL_FRAMEBUFFER_BINDING
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#endif
#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_TEXTURE_BINDING_2D
#define GL_TEXTURE_BINDING_2D 0x8069
#endif
// Common enums not present in GL 1.1 headers
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#endif
#ifndef GL_VERTEX_ARRAY_BINDING
#define GL_VERTEX_ARRAY_BINDING 0x85B5
#endif
#ifndef GL_ACTIVE_TEXTURE
#define GL_ACTIVE_TEXTURE 0x84E0
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_SHADING_LANGUAGE_VERSION
#define GL_SHADING_LANGUAGE_VERSION 0x8B8C
#endif
#ifndef GL_DEPTH24_STENCIL8
#define GL_DEPTH24_STENCIL8 0x88F0
#endif
#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif

// Declare function pointer types for FBO entry points
typedef void(APIENTRYP PFNGLGENFRAMEBUFFERSPROC)(GLsizei, GLuint*);
typedef void(APIENTRYP PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei, const GLuint*);
typedef void(APIENTRYP PFNGLBINDFRAMEBUFFERPROC)(GLenum, GLuint);
typedef void(APIENTRYP PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum, GLenum, GLenum,
                                                      GLuint, GLint);
typedef GLenum(APIENTRYP PFNGLCHECKFRAMEBUFFERSTATUSPROC)(GLenum);
typedef void(APIENTRYP PFNGLGENRENDERBUFFERSPROC)(GLsizei, GLuint*);
typedef void(APIENTRYP PFNGLBINDRENDERBUFFERPROC)(GLenum, GLuint);
typedef void(APIENTRYP PFNGLRENDERBUFFERSTORAGEPROC)(GLenum, GLenum, GLsizei,
                                                     GLsizei);
typedef void(APIENTRYP PFNGLFRAMEBUFFERRENDERBUFFERPROC)(GLenum, GLenum, GLenum,
                                                         GLuint);
typedef void(APIENTRYP PFNGLDELETERENDERBUFFERSPROC)(GLsizei, const GLuint*);

// Additional GL entry points we may need to stub in ultra-safe mode
typedef const GLubyte*(APIENTRYP PFNGLGETSTRINGIPROC)(GLenum, GLuint);
typedef void(APIENTRYP PFNGLBINDFRAGDATALOCATIONPROC)(GLuint, GLuint,
                                                      const GLchar*);
typedef void*(APIENTRYP PFNGLMAPBUFFERRANGEPROC)(GLenum, GLintptr, GLsizeiptr,
                                                 GLbitfield);
typedef void(APIENTRYP PFNGLFLUSHMAPPEDBUFFERRANGEPROC)(GLenum, GLintptr,
                                                        GLsizeiptr);
typedef void(APIENTRYP PFNGLINVALIDATEFRAMEBUFFERPROC)(GLenum, GLsizei,
                                                       const GLenum*);
typedef void(APIENTRYP PFNGLINVALIDATESUBFRAMEBUFFERPROC)(GLenum, GLsizei,
                                                          const GLenum*, GLint,
                                                          GLint, GLsizei,
                                                          GLsizei);

// Forward declarations for ultra-safe stubs (defined at file end)
static const GLubyte* APIENTRY stub_glGetStringi(GLenum, GLuint);
static void APIENTRY stub_glBindFragDataLocation(GLuint, GLuint, const GLchar*);
static void* APIENTRY stub_glMapBufferRange(GLenum, GLintptr, GLsizeiptr,
                                            GLbitfield);
static void APIENTRY stub_glFlushMappedBufferRange(GLenum, GLintptr,
                                                   GLsizeiptr);
static void APIENTRY stub_glInvalidateFramebuffer(GLenum, GLsizei,
                                                  const GLenum*);
static void APIENTRY stub_glInvalidateSubFramebuffer(GLenum, GLsizei,
                                                     const GLenum*, GLint,
                                                     GLint, GLsizei, GLsizei);

static PFNGLGENFRAMEBUFFERSPROC p_glGenFramebuffers = nullptr;
static PFNGLDELETEFRAMEBUFFERSPROC p_glDeleteFramebuffers = nullptr;
static PFNGLBINDFRAMEBUFFERPROC p_glBindFramebuffer = nullptr;
static PFNGLFRAMEBUFFERTEXTURE2DPROC p_glFramebufferTexture2D = nullptr;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC p_glCheckFramebufferStatus = nullptr;
static PFNGLGENRENDERBUFFERSPROC p_glGenRenderbuffers = nullptr;
static PFNGLBINDRENDERBUFFERPROC p_glBindRenderbuffer = nullptr;
static PFNGLRENDERBUFFERSTORAGEPROC p_glRenderbufferStorage = nullptr;
static PFNGLFRAMEBUFFERRENDERBUFFERPROC p_glFramebufferRenderbuffer = nullptr;
static PFNGLDELETERENDERBUFFERSPROC p_glDeleteRenderbuffers = nullptr;

// wgl current context
typedef HGLRC(WINAPI* PFNWGLGETCURRENTCONTEXTPROC)();
static PFNWGLGETCURRENTCONTEXTPROC p_wglGetCurrentContext = nullptr;
typedef HDC(WINAPI* PFNWGLGETCURRENTDCPROC)();
static PFNWGLGETCURRENTDCPROC p_wglGetCurrentDC = nullptr;
typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTPROC)(HDC);
static PFNWGLCREATECONTEXTPROC p_wglCreateContext = nullptr;
typedef BOOL(WINAPI* PFNWGLDELETECONTEXTPROC)(HGLRC);
static PFNWGLDELETECONTEXTPROC p_wglDeleteContext = nullptr;
typedef BOOL(WINAPI* PFNWGLMAKECURRENTPROC)(HDC, HGLRC);
static PFNWGLMAKECURRENTPROC p_wglMakeCurrent = nullptr;
typedef BOOL(WINAPI* PFNWGLSHARELISTSPROC)(HGLRC, HGLRC);
static PFNWGLSHARELISTSPROC p_wglShareLists = nullptr;

// A few modern helpers we may use to leave GL in a clean state after
// MapLibre renders. These are loaded dynamically when available and skipped
// otherwise so we can still run against legacy headers/contexts.
typedef void(APIENTRYP PFNGLUSEPROGRAMPROC)(GLuint);
typedef void(APIENTRYP PFNGLBINDBUFFERPROC)(GLenum, GLuint);
typedef void(APIENTRYP PFNGLBINDVERTEXARRAYPROC)(GLuint);
typedef void(APIENTRYP PFNGLGENVERTEXARRAYSPROC)(GLsizei, GLuint*);
typedef void(APIENTRYP PFNGLDELETEVERTEXARRAYSPROC)(GLsizei, const GLuint*);
typedef void(APIENTRYP PFNGLACTIVETEXTUREPROC)(GLenum);
typedef void(APIENTRYP PFNGLDRAWBUFFERSPROC)(GLsizei, const GLenum*);

static PFNGLUSEPROGRAMPROC p_glUseProgram = nullptr;
static PFNGLBINDBUFFERPROC p_glBindBuffer = nullptr;
static PFNGLBINDVERTEXARRAYPROC p_glBindVertexArray = nullptr;
static PFNGLACTIVETEXTUREPROC p_glActiveTexture = nullptr;
static PFNGLDRAWBUFFERSPROC p_glDrawBuffers = nullptr;
static PFNGLGENVERTEXARRAYSPROC p_glGenVertexArrays = nullptr;
static PFNGLDELETEVERTEXARRAYSPROC p_glDeleteVertexArrays = nullptr;

// Minimal cross-loader for GL/ES symbols on Windows. Prefer the active API:
//  - If a WGL context is current, use wglGetProcAddress/opengl32.dll
//  - Otherwise try ANGLE/EGL: eglGetProcAddress + libGLESv2.dll
static bool env_truthy(const char* v) {
    if (!v)
        return false;
    std::string s(v);
    for (auto& c : s)
        c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
    return (s == "1" || s == "true" || s == "on" || s == "yes");
}

static FARPROC load_gl_proc(const char* name) {
    static const bool force_desktop_gl =
        env_truthy(std::getenv("MLNS_FORCE_DESKTOP_GL"));
    // Bootstrap: resolve WGL entry points directly from opengl32.dll
    if (std::strncmp(name, "wgl", 3) == 0) {
        HMODULE lib = GetModuleHandleA("opengl32.dll");
        if (lib) {
            FARPROC p = GetProcAddress(lib, name);
            if (p)
                return p;
        }
    }
    // Ensure we can query WGL current context
    if (!p_wglGetCurrentContext) {
        HMODULE lib = GetModuleHandleA("opengl32.dll");
        if (lib) {
            p_wglGetCurrentContext =
                reinterpret_cast<PFNWGLGETCURRENTCONTEXTPROC>(
                    GetProcAddress(lib, "wglGetCurrentContext"));
        }
    }
    const bool have_wgl =
        p_wglGetCurrentContext && p_wglGetCurrentContext() != nullptr;
    if (have_wgl) {
        // When a WGL context is current, NEVER mix in EGL/GLES function
        // pointers.
        FARPROC p = wglGetProcAddress(name);
        if (!p) {
            HMODULE lib = GetModuleHandleA("opengl32.dll");
            if (lib)
                p = GetProcAddress(lib, name);
        }
        if (p && p != reinterpret_cast<FARPROC>(1) &&
            p != reinterpret_cast<FARPROC>(2) &&
            p != reinterpret_cast<FARPROC>(3) &&
            p != reinterpret_cast<FARPROC>(-1)) {
            return p;
        }
        // No fallback to EGL here by design; return null so callers can
        // stub/deny.
        return nullptr;
    }
    if (force_desktop_gl) {
        // Avoid falling back to EGL/ANGLE if explicitly requested.
        return nullptr;
    }
    // ANGLE / EGL path (only when no WGL context is current)
    using PFNEGLGETPROCADDRESSPROC = void*(WINAPI*)(const char*);
    static PFNEGLGETPROCADDRESSPROC p_eglGetProcAddress = nullptr;
    static bool egl_loaded = false;
    if (!egl_loaded) {
        HMODULE egl = GetModuleHandleA("libEGL.dll");
        if (!egl)
            egl = GetModuleHandleA("EGL.dll");
        if (egl)
            p_eglGetProcAddress = reinterpret_cast<PFNEGLGETPROCADDRESSPROC>(
                GetProcAddress(egl, "eglGetProcAddress"));
        egl_loaded = true;
    }
    // Prefer core symbols from libGLESv2.dll for ES core functions
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
    // If still not found, try eglGetProcAddress (extensions)
    if (!vp && p_eglGetProcAddress)
        vp = p_eglGetProcAddress(name);
    return reinterpret_cast<FARPROC>(vp);
}

static bool ensure_gl_functions_loaded() {
    if (p_glGenFramebuffers)
        return true;  // already loaded
                      // Ensure we can query WGL current status in this process
#ifdef _WIN32
    if (!p_wglGetCurrentContext) {
        HMODULE lib = GetModuleHandleA("opengl32.dll");
        if (lib)
            p_wglGetCurrentContext =
                reinterpret_cast<PFNWGLGETCURRENTCONTEXTPROC>(
                    GetProcAddress(lib, "wglGetCurrentContext"));
        if (lib) {
            p_wglGetCurrentDC = reinterpret_cast<PFNWGLGETCURRENTDCPROC>(
                GetProcAddress(lib, "wglGetCurrentDC"));
            p_wglCreateContext = reinterpret_cast<PFNWGLCREATECONTEXTPROC>(
                GetProcAddress(lib, "wglCreateContext"));
            p_wglDeleteContext = reinterpret_cast<PFNWGLDELETECONTEXTPROC>(
                GetProcAddress(lib, "wglDeleteContext"));
            p_wglMakeCurrent = reinterpret_cast<PFNWGLMAKECURRENTPROC>(
                GetProcAddress(lib, "wglMakeCurrent"));
            p_wglShareLists = reinterpret_cast<PFNWGLSHARELISTSPROC>(
                GetProcAddress(lib, "wglShareLists"));
        }
    }
#endif
    auto load_any = [](const char* const* names) -> FARPROC {
        for (size_t i = 0; names[i]; ++i) {
            FARPROC p = load_gl_proc(names[i]);
            if (p)
                return p;
        }
        return nullptr;
    };
    {
        const char* names[] = {"glGenFramebuffers", "glGenFramebuffersEXT",
                               "glGenFramebuffersOES", nullptr};
        p_glGenFramebuffers =
            reinterpret_cast<PFNGLGENFRAMEBUFFERSPROC>(load_any(names));
    }
    {
        const char* names[] = {"glDeleteFramebuffers",
                               "glDeleteFramebuffersEXT",
                               "glDeleteFramebuffersOES", nullptr};
        p_glDeleteFramebuffers =
            reinterpret_cast<PFNGLDELETEFRAMEBUFFERSPROC>(load_any(names));
    }
    {
        const char* names[] = {"glBindFramebuffer", "glBindFramebufferEXT",
                               "glBindFramebufferOES", nullptr};
        p_glBindFramebuffer =
            reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>(load_any(names));
    }
    {
        const char* names[] = {"glFramebufferTexture2D",
                               "glFramebufferTexture2DEXT",
                               "glFramebufferTexture2DOES", nullptr};
        p_glFramebufferTexture2D =
            reinterpret_cast<PFNGLFRAMEBUFFERTEXTURE2DPROC>(load_any(names));
    }
    {
        const char* names[] = {"glCheckFramebufferStatus",
                               "glCheckFramebufferStatusEXT",
                               "glCheckFramebufferStatusOES", nullptr};
        p_glCheckFramebufferStatus =
            reinterpret_cast<PFNGLCHECKFRAMEBUFFERSTATUSPROC>(load_any(names));
    }
    // Renderbuffer functions (depth-stencil)
    {
        const char* names[] = {"glGenRenderbuffers", "glGenRenderbuffersEXT",
                               "glGenRenderbuffersOES", nullptr};
        p_glGenRenderbuffers =
            reinterpret_cast<PFNGLGENRENDERBUFFERSPROC>(load_any(names));
    }
    {
        const char* names[] = {"glBindRenderbuffer", "glBindRenderbufferEXT",
                               "glBindRenderbufferOES", nullptr};
        p_glBindRenderbuffer =
            reinterpret_cast<PFNGLBINDRENDERBUFFERPROC>(load_any(names));
    }
    {
        const char* names[] = {"glRenderbufferStorage",
                               "glRenderbufferStorageEXT",
                               "glRenderbufferStorageOES", nullptr};
        p_glRenderbufferStorage =
            reinterpret_cast<PFNGLRENDERBUFFERSTORAGEPROC>(load_any(names));
    }
    {
        const char* names[] = {"glFramebufferRenderbuffer",
                               "glFramebufferRenderbufferEXT",
                               "glFramebufferRenderbufferOES", nullptr};
        p_glFramebufferRenderbuffer =
            reinterpret_cast<PFNGLFRAMEBUFFERRENDERBUFFERPROC>(load_any(names));
    }
    {
        const char* names[] = {"glDeleteRenderbuffers",
                               "glDeleteRenderbuffersEXT",
                               "glDeleteRenderbuffersOES", nullptr};
        p_glDeleteRenderbuffers =
            reinterpret_cast<PFNGLDELETERENDERBUFFERSPROC>(load_any(names));
    }
    if (!p_wglGetCurrentContext) {
        p_wglGetCurrentContext = reinterpret_cast<PFNWGLGETCURRENTCONTEXTPROC>(
            load_gl_proc("wglGetCurrentContext"));
    }
    bool ok = p_glGenFramebuffers && p_glDeleteFramebuffers &&
              p_glBindFramebuffer && p_glFramebufferTexture2D &&
              p_glCheckFramebufferStatus;
    if (!ok) {
        std::cerr << "[GL] Missing FBO symbols:" << " gen="
                  << (p_glGenFramebuffers ? "ok" : "null")
                  << " del=" << (p_glDeleteFramebuffers ? "ok" : "null")
                  << " bind=" << (p_glBindFramebuffer ? "ok" : "null")
                  << " tex2D=" << (p_glFramebufferTexture2D ? "ok" : "null")
                  << " status=" << (p_glCheckFramebufferStatus ? "ok" : "null")
                  << " rbo.gen=" << (p_glGenRenderbuffers ? "ok" : "null")
                  << " rbo.bind=" << (p_glBindRenderbuffer ? "ok" : "null")
                  << " rbo.store=" << (p_glRenderbufferStorage ? "ok" : "null")
                  << " rbo.attach="
                  << (p_glFramebufferRenderbuffer ? "ok" : "null")
                  << " rbo.del=" << (p_glDeleteRenderbuffers ? "ok" : "null")
                  << std::endl;
    }
    return ok;
}

#define glGenFramebuffers p_glGenFramebuffers
#define glDeleteFramebuffers p_glDeleteFramebuffers
#define glBindFramebuffer p_glBindFramebuffer
#define glFramebufferTexture2D p_glFramebufferTexture2D
#define glCheckFramebufferStatus p_glCheckFramebufferStatus
#define glGenRenderbuffers p_glGenRenderbuffers
#define glBindRenderbuffer p_glBindRenderbuffer
#define glRenderbufferStorage p_glRenderbufferStorage
#define glFramebufferRenderbuffer p_glFramebufferRenderbuffer
#define glDeleteRenderbuffers p_glDeleteRenderbuffers

static void ensure_optional_gl_functions_loaded() {
    if (!p_glUseProgram)
        p_glUseProgram =
            reinterpret_cast<PFNGLUSEPROGRAMPROC>(load_gl_proc("glUseProgram"));
    if (!p_glBindBuffer)
        p_glBindBuffer =
            reinterpret_cast<PFNGLBINDBUFFERPROC>(load_gl_proc("glBindBuffer"));
    if (!p_glBindVertexArray)
        p_glBindVertexArray = reinterpret_cast<PFNGLBINDVERTEXARRAYPROC>(
            load_gl_proc("glBindVertexArray"));
    if (!p_glGenVertexArrays)
        p_glGenVertexArrays = reinterpret_cast<PFNGLGENVERTEXARRAYSPROC>(
            load_gl_proc("glGenVertexArrays"));
    if (!p_glDeleteVertexArrays)
        p_glDeleteVertexArrays = reinterpret_cast<PFNGLDELETEVERTEXARRAYSPROC>(
            load_gl_proc("glDeleteVertexArrays"));
    if (!p_glActiveTexture)
        p_glActiveTexture = reinterpret_cast<PFNGLACTIVETEXTUREPROC>(
            load_gl_proc("glActiveTexture"));
    if (!p_glDrawBuffers) {
        p_glDrawBuffers = reinterpret_cast<PFNGLDRAWBUFFERSPROC>(
            load_gl_proc("glDrawBuffers"));
        if (!p_glDrawBuffers)
            p_glDrawBuffers = reinterpret_cast<PFNGLDRAWBUFFERSPROC>(
                load_gl_proc("glDrawBuffersEXT"));
    }
}
#endif

using namespace std;

namespace mlns {

static std::string now_ts() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto t = system_clock::to_time_t(now);
    std::tm lt{};
#ifdef _WIN32
    localtime_s(&lt, &t);
#else
    localtime_r(&t, &lt);
#endif
    const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    std::ostringstream oss;
    oss << std::put_time(&lt, "%Y-%m-%d %H:%M:%S") << "." << std::setw(3)
        << std::setfill('0') << ms.count();
    return oss.str();
}

#define LOGI(expr)                                               \
    do {                                                         \
        std::ostringstream _oss;                                 \
        _oss << expr;                                            \
        std::cout << now_ts() << " " << _oss.str() << std::endl; \
    } while (0)
#define LOGE(expr)                                               \
    do {                                                         \
        std::ostringstream _oss;                                 \
        _oss << expr;                                            \
        std::cerr << now_ts() << " " << _oss.str() << std::endl; \
    } while (0)

static void gl_check(const char* where) {
#ifdef _WIN32
    // On Windows' legacy headers, GL errors are defined; use them to log.
    GLenum err = glGetError();
    if (err != 0) {
        LOGE("[GL] error 0x" << std::hex << err << std::dec << " at " << where);
    }
#endif
}

// Returns false if GL calls look invalid (e.g. context not current)
static bool gl_context_seems_current(const char* where) {
#ifdef _WIN32
    if (p_wglGetCurrentContext && p_wglGetCurrentContext() == nullptr) {
        LOGE("[ZeroCopyGL] GL not current (wglGetCurrentContext=null) at "
             << where);
        return false;
    }
    GLint fbo_probe = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo_probe);
    GLenum err = glGetError();
    if (err == GL_INVALID_OPERATION) {
        LOGE("[ZeroCopyGL] GL not current at " << where
                                               << " (GL_INVALID_OPERATION)");
        return false;
    }
#endif
    (void)where;
    return true;
}

static void delete_rt(GLRenderTarget& rt) {
    LOGI("[ZeroCopyGL] delete_rt: fbo=" << rt.fbo << " tex=" << rt.color_tex
                                        << " size=" << rt.width << "x"
                                        << rt.height);
    if (rt.fbo) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &rt.fbo);
        rt.fbo = 0;
    }
    if (rt.depth_stencil_rb) {
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glDeleteRenderbuffers(1, &rt.depth_stencil_rb);
        rt.depth_stencil_rb = 0;
    }
    if (rt.color_tex) {
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &rt.color_tex);
        rt.color_tex = 0;
    }
    rt.width = rt.height = 0;
}

SlintZeroCopyGL::~SlintZeroCopyGL() {
    teardown();
}

void SlintZeroCopyGL::teardown() {
    std::cout << "[ZeroCopyGL] teardown()" << std::endl;
    delete_rt(rt_);
    borrowed_.reset();
}

void SlintZeroCopyGL::setup_if_needed(int w, int h) {
    LOGI("[ZeroCopyGL] setup_if_needed(w=" << w << ", h=" << h << ")");
    if (rt_.valid() && rt_.width == w && rt_.height == h) {
        std::cout << "[ZeroCopyGL] Already valid and size unchanged"
                  << std::endl;
        return;
    }

    // Recreate on size change
    delete_rt(rt_);

    rt_.width = w;
    rt_.height = h;

#ifdef _WIN32
    if (!ensure_gl_functions_loaded()) {
        LOGE("[SlintZeroCopyGL] Failed to load OpenGL FBO functions on "
             "Windows.");
        return;
    }
#endif

    // Preserve Slint bindings during resource creation
    GLint prevTex = 0;
    GLint prevFbo = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);

    glGenTextures(1, &rt_.color_tex);
    glBindTexture(GL_TEXTURE_2D, rt_.color_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 nullptr);

    glGenFramebuffers(1, &rt_.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, rt_.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           rt_.color_tex, 0);

    // Create a depth-stencil renderbuffer; many pipelines assume depth testing.
    glGenRenderbuffers(1, &rt_.depth_stencil_rb);
    glBindRenderbuffer(GL_RENDERBUFFER, rt_.depth_stencil_rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, rt_.depth_stencil_rb);

    const auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    LOGI("[ZeroCopyGL] Created FBO=" << rt_.fbo << " tex=" << rt_.color_tex
                                     << " status=0x" << std::hex << status
                                     << std::dec);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("[SlintZeroCopyGL] FBO incomplete: 0x" << std::hex << status
                                                    << std::dec);
        delete_rt(rt_);
        return;
    }

    // Restore previous bindings for Slint's renderer
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFbo));
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(prevTex));

    // Create/refresh the borrowed Slint image for this texture id/size.
    borrowed_ = slint::Image::create_from_borrowed_gl_2d_rgba_texture(
        rt_.color_tex,
        slint::Size<uint32_t>{static_cast<uint32_t>(w),
                              static_cast<uint32_t>(h)},
        slint::Image::BorrowedOpenGLTextureOrigin::BottomLeft);
    if (on_ready_ && borrowed_.has_value()) {
        LOGI("[ZeroCopyGL] on_ready: borrowed tex=" << rt_.color_tex << " size="
                                                    << w << "x" << h);
        // Defer UI property update to the event loop to avoid mutating Slint
        // state from within the rendering notifier callback.
        auto img = *borrowed_;
        auto cb = on_ready_;
        slint::invoke_from_event_loop([img, cb]() { cb(img); });
    }
}

void SlintZeroCopyGL::resize(int w, int h) {
    setup_if_needed(w, h);
}

void SlintZeroCopyGL::attach(slint::Window& window,
                             RenderHook on_before_render) {
    on_before_render_ = std::move(on_before_render);
    if (attached_)
        return;
    attached_ = true;

    window.set_rendering_notifier([this, &window](slint::RenderingState s,
                                                  slint::GraphicsAPI) {
        switch (s) {
        case slint::RenderingState::RenderingSetup: {
            int target_w = desired_w_ > 0
                               ? desired_w_
                               : static_cast<int>(window.size().width);
            int target_h = desired_h_ > 0
                               ? desired_h_
                               : static_cast<int>(window.size().height);
            pending_w_ = target_w;
            pending_h_ = target_h;
            resources_ready_ = rt_.valid();
            LOGI("[ZeroCopyGL] RenderingSetup: window.size="
                 << window.size().width << "x" << window.size().height
                 << ", target=" << pending_w_ << "x" << pending_h_
                 << ", resources_ready="
                 << (resources_ready_ ? "true" : "false"));
            break;
        }
        case slint::RenderingState::BeforeRendering: {
            // Refresh desired size each frame. If an explicit surface size was
            // set (map image area), prefer that; otherwise use window size.
            int target_w = desired_w_ > 0
                               ? desired_w_
                               : static_cast<int>(window.size().width);
            int target_h = desired_h_ > 0
                               ? desired_h_
                               : static_cast<int>(window.size().height);
            pending_w_ = target_w;
            pending_h_ = target_h;
            frame_counter_++;
            LOGI("[ZeroCopyGL] BeforeRendering: frame="
                 << frame_counter_
                 << " resources_ready=" << (resources_ready_ ? "true" : "false")
                 << ", rt_valid=" << (rt_.valid() ? "true" : "false")
                 << " pending=" << pending_w_ << "x" << pending_h_);
            // (GL info log omitted on Windows MSVC to avoid macro parsing
            // quirks)

            if ((!resources_ready_ || !rt_.valid()) && pending_w_ > 0 &&
                pending_h_ > 0) {
                setup_if_needed(pending_w_, pending_h_);
                resources_ready_ = rt_.valid();
                LOGI("[ZeroCopyGL] BeforeRendering: setup_if_needed done, "
                     "rt_valid="
                     << (rt_.valid() ? "true" : "false"));
                //  :
                warmup_remaining_ = 2;
                return;
            } else if (rt_.valid() &&
                       (rt_.width != pending_w_ || rt_.height != pending_h_) &&
                       pending_w_ > 0 && pending_h_ > 0) {
                setup_if_needed(pending_w_, pending_h_);
                resources_ready_ = rt_.valid();
                LOGI("[ZeroCopyGL] BeforeRendering: resized RT, rt_valid="
                     << (rt_.valid() ? "true" : "false"));
                warmup_remaining_ = 2;
                return;
            }
            if (!rt_.valid()) {
                LOGI("[ZeroCopyGL] BeforeRendering: rt not valid, skip render");
                return;
            }
            if (warmup_remaining_ > 0) {
                LOGI("[ZeroCopyGL] BeforeRendering: warmup skip");
                warmup_remaining_ -= 1;
                return;
            }
            // Perform actual render here (BeforeRendering). Avoid touching swap
            // path.
            if (!gl_context_seems_current("BeforeRendering-entry") ||
                in_render_) {
                break;
            }
            in_render_ = true;
            GLint prevFbo = 0;
            GLint prevViewport[4] = {0, 0, 0, 0};
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
            glGetIntegerv(GL_VIEWPORT, prevViewport);
            glBindFramebuffer(GL_FRAMEBUFFER, rt_.fbo);
            glViewport(0, 0, rt_.width, rt_.height);
            // Ultra-minimal path: do not touch masks, sRGB state, VAO, draw
            // buffers, etc.
            if (on_before_render_) {
                LOGI("[ZeroCopyGL] BeforeRendering: calling render hook");
                try {
                    on_before_render_(rt_);
                } catch (const std::exception& e) {
                    LOGE("[ZeroCopyGL] render hook threw: " << e.what());
                } catch (...) {
                    LOGE("[ZeroCopyGL] render hook threw unknown exception");
                }
            }
            glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFbo));
            glViewport(prevViewport[0], prevViewport[1], prevViewport[2],
                       prevViewport[3]);
            in_render_ = false;
            break;
        }
        case slint::RenderingState::AfterRendering: {
            // Avoid GL calls here to keep driver swap path stable.
            window.request_redraw();
            break;
        }
        case slint::RenderingState::RenderingTeardown: {
            std::cout << "[ZeroCopyGL] RenderingTeardown" << std::endl;
            teardown();
            break;
        }
        }
    });
}

}  // namespace mlns

// Controller
namespace mlns {
SlintGLMapLibre::SlintGLMapLibre() = default;
SlintGLMapLibre::~SlintGLMapLibre() {
#ifdef _WIN32
    {
        std::lock_guard<std::mutex> lk(iso_mu_);
        iso_stop_ = true;
    }
    iso_cv_.notify_all();
    if (iso_thread_.joinable())
        iso_thread_.join();
#endif
}

void SlintGLMapLibre::initialize(int w, int h) {
    std::cout << "[SlintGLMapLibre] initialize " << w << "x" << h << std::endl;
    width = w;
    height = h;
    // Record style URL (used by thread-owned path or immediate init)
    if (const char* env_style = std::getenv("MLNS_STYLE_URL");
        env_style && *env_style) {
        style_url_ = env_style;
        std::cout << "[SlintGLMapLibre] Using MLNS_STYLE_URL=" << env_style
                  << std::endl;
    } else {
        style_url_ = "https://demotiles.maplibre.org/style.json";
    }

#ifdef _WIN32
    // Read isolate flag
    isolate_ctx_ = []() {
        const char* v = std::getenv("MLNS_GL_ISOLATE_CONTEXT");
        if (!v)
            return true;  // default ON on Windows
        std::string s(v);
        for (auto& c : s)
            c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
        return !(s == "0" || s == "false" || s == "off" || s == "no");
    }();
    if (isolate_ctx_) {
        // Defer MapLibre object creation to the isolated render thread
        thread_owns_map_ = true;
        return;
    }
#endif
    if (!run_loop)
        run_loop = std::make_unique<mbgl::util::RunLoop>();
    backend = std::make_unique<SlintGLBackend>(mbgl::gfx::ContextMode::Unique);
    backend->setSize(
        mbgl::Size{static_cast<uint32_t>(w), static_cast<uint32_t>(h)});
    auto rnd = std::make_unique<mbgl::Renderer>(*backend, 1.0f);
    frontend =
        std::make_unique<SlintRendererFrontend>(std::move(rnd), *backend);
    mbgl::ResourceOptions resourceOptions;
    resourceOptions.withCachePath("cache.sqlite").withAssetPath(".");
    map =
        std::make_unique<mbgl::Map>(*frontend, *this,
                                    mbgl::MapOptions()
                                        .withMapMode(mbgl::MapMode::Continuous)
                                        .withSize(backend->getSize())
                                        .withPixelRatio(1.0f),
                                    resourceOptions);
    map->getStyle().loadURL(style_url_);
}

void SlintGLMapLibre::resize(int w, int h) {
    std::cout << "[SlintGLMapLibre] resize " << w << "x" << h << std::endl;
    width = w;
    height = h;
    if (backend)
        backend->setSize({static_cast<uint32_t>(w), static_cast<uint32_t>(h)});
    if (map)
        map->setSize({static_cast<uint32_t>(w), static_cast<uint32_t>(h)});
}

void SlintGLMapLibre::run_map_loop() {
    // NOTE: Avoid pumping MapLibre's RunLoop from Slint's timer callback to
    // prevent re-entrant event processing inside the UI thread. We instead
    // drive the run loop just-in-time from render_to_fbo() where the GL
    // context is current, similar to the Metal path's deferred rendering.
    (void)run_loop;
}

void SlintGLMapLibre::render_to_fbo(uint32_t fbo, int w, int h) {
#ifdef _WIN32
    if (isolate_ctx_) {
        // Ignore FBO; render via isolated context into shared texture path
        // only.
        return;
    }
#endif
    std::cout << "[SlintGLMapLibre] render_to_fbo fbo=" << fbo << " size=" << w
              << "x" << h << std::endl;
    if (!backend || !frontend)
        return;
    // Log GL strings once to confirm context profile
    static bool info_logged = false;
    if (!info_logged) {
        const GLubyte* ver = glGetString(GL_VERSION);
        const GLubyte* ven = glGetString(GL_VENDOR);
        const GLubyte* ren = glGetString(GL_RENDERER);
        const GLubyte* sl = glGetString(GL_SHADING_LANGUAGE_VERSION);
        LOGI("[GL] version="
             << (ver ? reinterpret_cast<const char*>(ver) : "?")
             << " vendor=" << (ven ? reinterpret_cast<const char*>(ven) : "?")
             << " renderer=" << (ren ? reinterpret_cast<const char*>(ren) : "?")
             << " sl=" << (sl ? reinterpret_cast<const char*>(sl) : "?"));
        info_logged = true;
    }
    const auto desired =
        mbgl::Size{static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
    if (backend->getSize().width != desired.width ||
        backend->getSize().height != desired.height) {
        backend->setSize(desired);
        if (map)
            map->setSize(desired);
    }
    backend->updateFramebuffer(
        fbo, {static_cast<uint32_t>(w), static_cast<uint32_t>(h)});
    backend->setViewport(0, 0, backend->getSize());

    // Make GL draw to our FBO with an explicit draw buffer selection (core
    // profile safety)
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    ensure_optional_gl_functions_loaded();
    if (p_glDrawBuffers) {
        const GLenum bufs[1] = {GL_COLOR_ATTACHMENT0};
        p_glDrawBuffers(1, bufs);
    }
    // Core profile safety: ensure a VAO is bound
    static GLuint s_dummy_vao = 0;
    if (p_glGenVertexArrays && p_glBindVertexArray && s_dummy_vao == 0) {
        p_glGenVertexArrays(1, &s_dummy_vao);
    }
    if (p_glBindVertexArray && s_dummy_vao) {
        p_glBindVertexArray(s_dummy_vao);
    }

    // Process MapLibre updates outside Slint's event dispatch to avoid
    // re-entrancy. Generate update parameters and then render.
    if (run_loop) {
        try {
            run_loop->runOnce();
        } catch (...) {
            // Best-effort: keep rendering even if no pending tasks
        }
    }
    if (map) {
        map->triggerRepaint();
    }
    if (run_loop) {
        try {
            run_loop->runOnce();
        } catch (...) {
        }
    }

    // Extra safety: verify vtable pointers point to executable memory
#ifdef _WIN32
    const int vptr_guard_mode = get_vptr_guard_mode();
    if (vptr_guard_mode != GUARD_OFF) {
        void* fe_vptr =
            frontend ? *reinterpret_cast<void**>(frontend.get()) : nullptr;
        void* be_vptr =
            backend ? *reinterpret_cast<void**>(backend.get()) : nullptr;
        mbgl::Renderer* ren_ptr = frontend ? frontend->getRenderer() : nullptr;
        void* ren_vptr = ren_ptr ? *reinterpret_cast<void**>(ren_ptr) : nullptr;
#ifdef _WIN32
        static bool vft_once = false;
        if (!vft_once) {
            log_vftable("frontend", fe_vptr);
            log_vftable("backend", be_vptr);
            log_vftable("renderer", ren_vptr);
            vft_once = true;
        }
#endif
#ifdef _WIN32
        const bool ok = looks_like_vftable(fe_vptr) &&
                        looks_like_vftable(be_vptr) &&
                        looks_like_vftable(ren_vptr);
#else
        const bool ok = fe_vptr && be_vptr && ren_vptr;
#endif
        if (!ok) {
            if (vptr_guard_mode == GUARD_STRICT) {
                LOGE("[GLGuard] vtable sanity failed: frontend="
                     << fe_vptr << " backend=" << be_vptr
                     << " renderer=" << ren_vptr << "; skip render");
                return;  // 厳格モードのみスキップ
            } else {
                LOGE("[GLGuard] vtable sanity failed: frontend="
                     << fe_vptr << " backend=" << be_vptr
                     << " renderer=" << ren_vptr << "; warn only");
            }
        }
    }
#endif

    // Clear any pre-existing GL error to avoid tripping our check
    while (glGetError() != 0) {
    }
    gl_check("before MapLibre render");
#ifdef _MSC_VER
    auto prev_seh =
        _set_se_translator([](unsigned int code, _EXCEPTION_POINTERS*) {
            char msg[64];
            std::snprintf(msg, sizeof(msg), "SEH 0x%08X", code);
            throw std::runtime_error(msg);
        });
#endif
    try {
        frontend->render();
    } catch (const std::exception& e) {
        LOGE("[SlintGLMapLibre] frontend->render threw: " << e.what());
        return;
    } catch (...) {
        LOGE("[SlintGLMapLibre] frontend->render threw unknown exception");
        return;
    }
#ifdef _MSC_VER
    _set_se_translator(prev_seh);
#endif
    LOGI("[SlintGLMapLibre] MapLibre render returned");
    gl_check("after MapLibre render");
    if (first_render_) {
        first_render_ = false;
        // Flush first frame once to avoid driver hiccups at startup
        glFinish();
        std::cout << "[SlintGLMapLibre] glFinish after first render"
                  << std::endl;
    }
}

void SlintGLMapLibre::render_to_texture(uint32_t texture, int w, int h) {
#ifdef _WIN32
    if (isolate_ctx_) {
        ensure_isolated_context_created();
        if (map_hglrc_) {
            request_isolated_render(texture, w, h);
            return;
        }
        if (!map_hglrc_ || !p_wglGetCurrentContext || !p_wglGetCurrentDC ||
            !p_wglMakeCurrent) {
            return;
        }
        HGLRC prevRC = p_wglGetCurrentContext();
        HDC prevDC = p_wglGetCurrentDC();
        HDC hdc = static_cast<HDC>(slint_hdc_);
        if (!p_wglMakeCurrent(hdc, static_cast<HGLRC>(map_hglrc_)))
            return;

        ::ensure_gl_functions_loaded();
        // (Re)create iso FBO as needed
        if (iso_fbo_ == 0 || iso_w_ != w || iso_h_ != h) {
            if (iso_fbo_) {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glDeleteFramebuffers(1, &iso_fbo_);
                iso_fbo_ = 0;
            }
            glGenFramebuffers(1, &iso_fbo_);
            iso_w_ = w;
            iso_h_ = h;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, iso_fbo_);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, texture, 0);
        // Ensure depth-stencil attachment (MapLibre relies on depth test)
        if (iso_depth_rb_ == 0) {
            glGenRenderbuffers(1, &iso_depth_rb_);
        }
        glBindRenderbuffer(GL_RENDERBUFFER, iso_depth_rb_);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER, iso_depth_rb_);
        // Viewport for our render area
        glViewport(0, 0, w, h);
        // Core profile safety: select draw buffer and bind a dummy VAO
        ensure_optional_gl_functions_loaded();
        if (p_glDrawBuffers) {
            const GLenum bufs[1] = {GL_COLOR_ATTACHMENT0};
            p_glDrawBuffers(1, bufs);
        }
        static GLuint s_dummy_vao = 0;
        if (p_glGenVertexArrays && p_glBindVertexArray && s_dummy_vao == 0) {
            p_glGenVertexArrays(1, &s_dummy_vao);
        }
        if (p_glBindVertexArray && s_dummy_vao) {
            p_glBindVertexArray(s_dummy_vao);
        }
        glDisable(GL_SCISSOR_TEST);

        // FBO completeness check (once per size)
        static int last_chk_w = 0, last_chk_h = 0;
        if (last_chk_w != w || last_chk_h != h) {
            GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            LOGI("[Iso] FBO=" << iso_fbo_ << " status=0x" << std::hex << st
                              << std::dec << " size=" << w << "x" << h);
            last_chk_w = w;
            last_chk_h = h;
        }
        backend->updateFramebuffer(
            iso_fbo_,
            mbgl::Size{static_cast<uint32_t>(w), static_cast<uint32_t>(h)});
        backend->setSize(
            mbgl::Size{static_cast<uint32_t>(w), static_cast<uint32_t>(h)});

        // Debug clear (optional)
        static bool dbg_clear = []() {
            const char* v = std::getenv("MLNS_GL_DEBUG_CLEAR");
            if (!v)
                return false;
            std::string s(v);
            for (auto& c : s)
                c = (char)tolower((unsigned char)c);
            return (s == "1" || s == "true" || s == "on" || s == "yes");
        }();
        if (dbg_clear) {
            glClearColor(0.0f, 0.6f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                    GL_STENCIL_BUFFER_BIT);
        }

        // Drive MapLibre updates similar to legacy path
        if (run_loop) {
            try {
                run_loop->runOnce();
            } catch (...) {
            }
        }
        if (map) {
            map->triggerRepaint();
        }
        if (run_loop) {
            try {
                run_loop->runOnce();
            } catch (...) {
            }
        }

        {
#ifdef _MSC_VER
            auto prev_seh =
                _set_se_translator([](unsigned int code, _EXCEPTION_POINTERS*) {
                    char msg[64];
                    std::snprintf(msg, sizeof(msg), "SEH 0x%08X", code);
                    throw std::runtime_error(msg);
                });
#endif
            try {
                // One-time GL info log
                static bool info_logged_iso = false;
                if (!info_logged_iso) {
                    const GLubyte* ver = glGetString(GL_VERSION);
                    const GLubyte* ven = glGetString(GL_VENDOR);
                    const GLubyte* ren = glGetString(GL_RENDERER);
                    LOGI("[IsoGL] version="
                         << (ver ? reinterpret_cast<const char*>(ver) : "?")
                         << " vendor="
                         << (ven ? reinterpret_cast<const char*>(ven) : "?")
                         << " renderer="
                         << (ren ? reinterpret_cast<const char*>(ren) : "?"));
                    info_logged_iso = true;
                }
                // Sync backend assumed state and render in scoped backend
                backend->updateAssumedState();
                {
                    // Validate vptrs again in isolated path (renderer included)
#ifdef _WIN32
                    const int vptr_guard_mode = get_vptr_guard_mode();
                    if (vptr_guard_mode != GUARD_OFF) {
                        void* fe_vptr =
                            frontend ? *reinterpret_cast<void**>(frontend.get())
                                     : nullptr;
                        void* be_vptr =
                            backend ? *reinterpret_cast<void**>(backend.get())
                                    : nullptr;
                        mbgl::Renderer* ren_ptr =
                            frontend ? frontend->getRenderer() : nullptr;
                        void* ren_vptr =
                            ren_ptr ? *reinterpret_cast<void**>(ren_ptr)
                                    : nullptr;
#ifdef _WIN32
                        const bool ok = looks_like_vftable(fe_vptr) &&
                                        looks_like_vftable(be_vptr) &&
                                        looks_like_vftable(ren_vptr);
#else
                        const bool ok = fe_vptr && be_vptr && ren_vptr;
#endif
                        if (!ok) {
#ifdef _WIN32
                            log_vftable("iso.frontend", fe_vptr);
                            log_vftable("iso.backend", be_vptr);
                            log_vftable("iso.renderer", ren_vptr);
#endif
                            if (vptr_guard_mode == GUARD_STRICT) {
                                LOGE("[IsoGLGuard] vtable sanity failed: "
                                     "frontend="
                                     << fe_vptr << " backend=" << be_vptr
                                     << " renderer=" << ren_vptr
                                     << "; skip render");
                                goto after_render_scope;  // 厳格モードのみスキップ
                            } else {
                                LOGE("[IsoGLGuard] vtable sanity failed: "
                                     "frontend="
                                     << fe_vptr << " backend=" << be_vptr
                                     << " renderer=" << ren_vptr
                                     << "; warn only");
                            }
                        }
                    }
#endif
                    mbgl::gfx::BackendScope guard{
                        *backend, mbgl::gfx::BackendScope::ScopeType::Implicit};
                    try {
                        frontend->render();
                    } catch (const std::exception& e) {
                        LOGE(
                            "[IsoRender] frontend->render threw: " << e.what());
                        goto after_render_scope;
                    } catch (...) {
                        LOGE("[IsoRender] frontend->render threw unknown "
                             "exception");
                        goto after_render_scope;
                    }
                }
            after_render_scope:
                glFlush();
            } catch (const std::exception& e) {
                LOGE("[IsoRender] frontend->render threw: " << e.what());
            } catch (...) {
                LOGE("[IsoRender] frontend->render threw unknown exception");
            }
#ifdef _MSC_VER
            _set_se_translator(prev_seh);
#endif
        }
        // Restore Slint context
        p_wglMakeCurrent(prevDC, prevRC);
        return;
    }
#endif
    // Fallback: bind a temporary FBO in the current context and attach texture
    GLuint tempFbo = 0;
    glGenFramebuffers(1, &tempFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, tempFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           texture, 0);
    render_to_fbo(tempFbo, w, h);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &tempFbo);
}

#ifdef _WIN32
void SlintGLMapLibre::ensure_isolated_context_created() {
    if (map_hglrc_)
        return;
    if (!p_wglGetCurrentContext || !p_wglGetCurrentDC || !p_wglCreateContext ||
        !p_wglShareLists || !p_wglMakeCurrent)
        return;
    slint_hglrc_ = p_wglGetCurrentContext();
    slint_hdc_ = p_wglGetCurrentDC();
    if (!slint_hglrc_ || !slint_hdc_)
        return;
    if (!iso_thread_.joinable()) {
        iso_stop_ = false;
        iso_ready_ = false;
        iso_thread_ =
            std::thread([this]() { this->isolated_render_thread_main(); });
        // wait up to 1s for ready
        std::unique_lock<std::mutex> lk(iso_mu_);
        iso_cv_.wait_for(lk, std::chrono::milliseconds(1000),
                         [this] { return iso_ready_; });
    }
}

void SlintGLMapLibre::request_isolated_render(uint32_t tex, int w, int h) {
    std::lock_guard<std::mutex> lk(iso_mu_);
    iso_req_.tex = tex;
    iso_req_.w = w;
    iso_req_.h = h;
    iso_req_.seq++;
    iso_cv_.notify_one();
}

void SlintGLMapLibre::isolated_render_thread_main() {
    if (!p_wglMakeCurrent || !p_wglCreateContext || !p_wglShareLists)
        return;
    // Create hidden window and its HDC for this thread
    WNDCLASSW wc{};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"MLNS_IsoWin";
    RegisterClassW(&wc);
    HWND hwnd = CreateWindowExW(
        0, wc.lpszClassName, L"mlns_iso", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
        CW_USEDEFAULT, 16, 16, nullptr, nullptr, wc.hInstance, nullptr);
    if (!hwnd) {
        LOGE("[IsoInit] CreateWindowExW failed");
        return;
    }
    HDC hdc = GetDC(hwnd);
    if (!hdc) {
        LOGE("[IsoInit] GetDC failed");
        return;
    }
    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    int pf = ChoosePixelFormat(hdc, &pfd);
    if (pf == 0 || !SetPixelFormat(hdc, pf, &pfd)) {
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        LOGE("[IsoInit] SetPixelFormat failed");
        return;
    }
    // Create RC and share with UI RC
    HGLRC rc = p_wglCreateContext(hdc);
    if (!rc) {
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        LOGE("[IsoInit] wglCreateContext failed");
        return;
    }
    if (!p_wglShareLists(static_cast<HGLRC>(slint_hglrc_), rc)) {
        p_wglDeleteContext(rc);
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        LOGE("[IsoInit] wglShareLists failed");
        return;
    }
    if (!p_wglMakeCurrent(hdc, rc)) {
        p_wglDeleteContext(rc);
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        LOGE("[IsoInit] wglMakeCurrent failed");
        return;
    }
    {
        std::lock_guard<std::mutex> lk(iso_mu_);
        iso_hwnd_ = hwnd;
        iso_hdc_ = hdc;
        map_hglrc_ = rc;
        iso_ready_ = true;
    }
    iso_cv_.notify_all();
    ::ensure_gl_functions_loaded();
    ensure_optional_gl_functions_loaded();
    LOGI("[IsoInit] ready hwnd=" << hwnd << " hdc=" << hdc << " rc=" << rc);
    while (true) {
        IsoReq req;
        {
            std::unique_lock<std::mutex> lk(iso_mu_);
            iso_cv_.wait(lk, [this] {
                return iso_stop_ || iso_done_seq_ != iso_req_.seq;
            });
            if (iso_stop_)
                break;
            req = iso_req_;
        }
        LOGI("[IsoRender] begin seq=" << req.seq << " tex=" << req.tex
                                      << " size=" << req.w << "x" << req.h);
        // Prepare FBO
        if (iso_fbo_ == 0 || iso_w_ != req.w || iso_h_ != req.h) {
            if (iso_fbo_) {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glDeleteFramebuffers(1, &iso_fbo_);
                iso_fbo_ = 0;
            }
            glGenFramebuffers(1, &iso_fbo_);
            iso_w_ = req.w;
            iso_h_ = req.h;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, iso_fbo_);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, req.tex, 0);
        if (iso_depth_rb_ == 0)
            glGenRenderbuffers(1, &iso_depth_rb_);
        glBindRenderbuffer(GL_RENDERBUFFER, iso_depth_rb_);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, req.w,
                              req.h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER, iso_depth_rb_);
        glViewport(0, 0, req.w, req.h);
        if (p_glDrawBuffers) {
            const GLenum bufs[1] = {GL_COLOR_ATTACHMENT0};
            p_glDrawBuffers(1, bufs);
        }
        static GLuint s_dummy_vao = 0;
        if (p_glGenVertexArrays && p_glBindVertexArray && s_dummy_vao == 0)
            p_glGenVertexArrays(1, &s_dummy_vao);
        if (p_glBindVertexArray && s_dummy_vao)
            p_glBindVertexArray(s_dummy_vao);

        // Update backend state
        backend->updateFramebuffer(iso_fbo_,
                                   mbgl::Size{static_cast<uint32_t>(req.w),
                                              static_cast<uint32_t>(req.h)});
        backend->setSize(mbgl::Size{static_cast<uint32_t>(req.w),
                                    static_cast<uint32_t>(req.h)});

        // Drive MapLibre once and render
        if (run_loop) {
            try {
                run_loop->runOnce();
            } catch (...) {
            }
        }
        if (map) {
            map->triggerRepaint();
        }
        if (run_loop) {
            try {
                run_loop->runOnce();
            } catch (...) {
            }
        }
        try {
            mbgl::gfx::BackendScope guard{
                *backend, mbgl::gfx::BackendScope::ScopeType::Implicit};
            frontend->render();
        } catch (...) {
            // skip frame
        }
        glFlush();
        {
            std::lock_guard<std::mutex> lk(iso_mu_);
            iso_done_seq_ = req.seq;
        }
        LOGI("[IsoRender] end seq=" << req.seq);
    }
    p_wglMakeCurrent(nullptr, nullptr);
    if (map_hglrc_) {
        p_wglDeleteContext(static_cast<HGLRC>(map_hglrc_));
        map_hglrc_ = nullptr;
    }
    if (iso_hdc_ && iso_hwnd_) {
        ReleaseDC(static_cast<HWND>(iso_hwnd_), static_cast<HDC>(iso_hdc_));
        iso_hdc_ = nullptr;
    }
    if (iso_hwnd_) {
        DestroyWindow(static_cast<HWND>(iso_hwnd_));
        iso_hwnd_ = nullptr;
    }
}
#endif

void SlintGLMapLibre::setStyleUrl(const std::string& url) {
    if (map)
        map->getStyle().loadURL(url);
}

void SlintGLMapLibre::handle_mouse_press(float x, float y) {
    last_pos = {x, y};
}

void SlintGLMapLibre::handle_mouse_move(float x, float y, bool pressed) {
    if (pressed && map) {
        mbgl::Point<double> current_pos = {x, y};
        map->moveBy(current_pos - last_pos);
        last_pos = current_pos;
    }
}

void SlintGLMapLibre::handle_double_click(float x, float y, bool shift) {
    if (!map)
        return;
    const mbgl::LatLng ll = map->latLngForPixel(mbgl::ScreenCoordinate{x, y});
    const auto cam = map->getCameraOptions();
    const double currentZoom = cam.zoom.value_or(0.0);
    const double delta = shift ? -1.0 : 1.0;
    const double targetZoom =
        std::min(max_zoom, std::max(min_zoom, currentZoom + delta));
    mbgl::CameraOptions next;
    next.withCenter(std::optional<mbgl::LatLng>(ll));
    next.withZoom(std::optional<double>(targetZoom));
    map->jumpTo(next);
}

void SlintGLMapLibre::handle_wheel_zoom(float x, float y, float dy) {
    if (!map)
        return;
    constexpr double step = 1.2;
    // Make scroll-up zoom in (align with Metal path/user expectation)
    double scale = (dy > 0.0) ? step : (1.0 / step);
    map->scaleBy(scale, mbgl::ScreenCoordinate{x, y});
}

void SlintGLMapLibre::fly_to(const std::string& location) {
    if (!map)
        return;
    mbgl::LatLng target;
    if (location == "paris")
        target = {48.8566, 2.3522};
    else if (location == "new_york")
        target = {40.7128, -74.0060};
    else
        target = {35.6895, 139.6917};
    mbgl::CameraOptions next;
    next.center = target;
    next.zoom = 10.0;
    map->jumpTo(next);
}

}  // namespace mlns
// Ultra-safe mode stubs (Windows)
// Windows helpers: memory protection checks for pointers
static bool is_executable_ptr(const void* p) {
#ifdef _WIN32
    if (!p)
        return false;
    MEMORY_BASIC_INFORMATION mbi{};
    if (VirtualQuery(p, &mbi, sizeof(mbi)) != sizeof(mbi))
        return false;
    const DWORD prot = mbi.Protect & 0xFF;
    switch (prot) {
    case PAGE_EXECUTE:
    case PAGE_EXECUTE_READ:
    case PAGE_EXECUTE_READWRITE:
    case PAGE_EXECUTE_WRITECOPY:
        return true;
    default:
        return false;
    }
#else
    (void)p;
    return true;
#endif
}

#ifdef _WIN32
static bool is_readable_ptr(const void* p) {
    if (!p)
        return false;
    MEMORY_BASIC_INFORMATION mbi{};
    if (VirtualQuery(p, &mbi, sizeof(mbi)) != sizeof(mbi))
        return false;
    const DWORD prot = mbi.Protect & 0xFF;
    switch (prot) {
    case PAGE_READONLY:
    case PAGE_READWRITE:
    case PAGE_WRITECOPY:
    case PAGE_EXECUTE_READ:
    case PAGE_EXECUTE_READWRITE:
    case PAGE_EXECUTE_WRITECOPY:
        return true;
    default:
        return false;
    }
}

// Heuristic: Validate vtable by checking first few entries are executable
static bool looks_like_vftable(const void* vft_ptr) {
    if (!is_readable_ptr(vft_ptr))
        return false;
    const void* const* vfptr_table =
        reinterpret_cast<const void* const*>(vft_ptr);
    for (int i = 0; i < 4; ++i) {
        const void* entry = nullptr;
#if defined(_MSC_VER)
        __try {
            entry = vfptr_table[i];
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
#else
        entry = vfptr_table[i];
#endif
        if (!is_executable_ptr(entry))
            return false;
    }
    return true;
}
#endif

#ifdef _WIN32
static void log_vftable(const char* tag, const void* vft_ptr) {
    if (!vft_ptr) {
        std::cout << "[VFT] " << tag << " vptr=null" << std::endl;
        return;
    }
    const void* const* table = reinterpret_cast<const void* const*>(vft_ptr);
    const void* e0 = nullptr;
    const void* e1 = nullptr;
    __try {
        e0 = table[0];
        e1 = table[1];
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        std::cout << "[VFT] " << tag << " vptr=" << vft_ptr
                  << " entries=ACCESS_VIOLATION" << std::endl;
        return;
    }
    std::cout << "[VFT] " << tag << " vptr=" << vft_ptr << " e0=" << e0
              << " e1=" << e1 << std::endl;
}
#endif
static const GLubyte* APIENTRY stub_glGetStringi(GLenum, GLuint) {
    static const GLubyte empty[] = "";
    return empty;  // report no extensions
}
static void APIENTRY stub_glBindFragDataLocation(GLuint, GLuint,
                                                 const GLchar*) {
    // no-op
}
static void* APIENTRY stub_glMapBufferRange(GLenum, GLintptr, GLsizeiptr,
                                            GLbitfield) {
    return nullptr;  // force fallback path
}
static void APIENTRY stub_glFlushMappedBufferRange(GLenum, GLintptr,
                                                   GLsizeiptr) {
    // no-op
}
static void APIENTRY stub_glInvalidateFramebuffer(GLenum, GLsizei,
                                                  const GLenum*) {
    // no-op
}
static void APIENTRY stub_glInvalidateSubFramebuffer(GLenum, GLsizei,
                                                     const GLenum*, GLint,
                                                     GLint, GLsizei, GLsizei) {
    // no-op
}
// (no forward declarations needed; static stubs are defined above)
