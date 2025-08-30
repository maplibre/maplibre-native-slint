#include "slint_maplibre_gl.hpp"

#include <cassert>
#include <iostream>
#include <mbgl/gfx/backend_scope.hpp>
#include <mbgl/gfx/renderable.hpp>

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
// Define missing enums for GL 1.1 headers on Windows
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
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

// Declare function pointer types for FBO entry points
typedef void(APIENTRYP PFNGLGENFRAMEBUFFERSPROC)(GLsizei, GLuint*);
typedef void(APIENTRYP PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei, const GLuint*);
typedef void(APIENTRYP PFNGLBINDFRAMEBUFFERPROC)(GLenum, GLuint);
typedef void(APIENTRYP PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum, GLenum, GLenum,
                                                      GLuint, GLint);
typedef GLenum(APIENTRYP PFNGLCHECKFRAMEBUFFERSTATUSPROC)(GLenum);

static PFNGLGENFRAMEBUFFERSPROC p_glGenFramebuffers = nullptr;
static PFNGLDELETEFRAMEBUFFERSPROC p_glDeleteFramebuffers = nullptr;
static PFNGLBINDFRAMEBUFFERPROC p_glBindFramebuffer = nullptr;
static PFNGLFRAMEBUFFERTEXTURE2DPROC p_glFramebufferTexture2D = nullptr;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC p_glCheckFramebufferStatus = nullptr;

// A few modern helpers we may use to leave GL in a clean state after
// MapLibre renders. These are loaded dynamically when available and skipped
// otherwise so we can still run against legacy headers/contexts.
typedef void(APIENTRYP PFNGLUSEPROGRAMPROC)(GLuint);
typedef void(APIENTRYP PFNGLBINDBUFFERPROC)(GLenum, GLuint);
typedef void(APIENTRYP PFNGLBINDVERTEXARRAYPROC)(GLuint);
typedef void(APIENTRYP PFNGLACTIVETEXTUREPROC)(GLenum);

static PFNGLUSEPROGRAMPROC p_glUseProgram = nullptr;
static PFNGLBINDBUFFERPROC p_glBindBuffer = nullptr;
static PFNGLBINDVERTEXARRAYPROC p_glBindVertexArray = nullptr;
static PFNGLACTIVETEXTUREPROC p_glActiveTexture = nullptr;

static FARPROC load_gl_proc(const char* name) {
    FARPROC p = wglGetProcAddress(name);
    if (!p) {
        HMODULE lib = GetModuleHandleA("opengl32.dll");
        if (lib)
            p = GetProcAddress(lib, name);
    }
    return p;
}

static bool ensure_gl_functions_loaded() {
    if (p_glGenFramebuffers)
        return true;  // already loaded
    p_glGenFramebuffers = reinterpret_cast<PFNGLGENFRAMEBUFFERSPROC>(
        load_gl_proc("glGenFramebuffers"));
    p_glDeleteFramebuffers = reinterpret_cast<PFNGLDELETEFRAMEBUFFERSPROC>(
        load_gl_proc("glDeleteFramebuffers"));
    p_glBindFramebuffer = reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>(
        load_gl_proc("glBindFramebuffer"));
    p_glFramebufferTexture2D = reinterpret_cast<PFNGLFRAMEBUFFERTEXTURE2DPROC>(
        load_gl_proc("glFramebufferTexture2D"));
    p_glCheckFramebufferStatus =
        reinterpret_cast<PFNGLCHECKFRAMEBUFFERSTATUSPROC>(
            load_gl_proc("glCheckFramebufferStatus"));
    return p_glGenFramebuffers && p_glDeleteFramebuffers &&
           p_glBindFramebuffer && p_glFramebufferTexture2D &&
           p_glCheckFramebufferStatus;
}

#define glGenFramebuffers p_glGenFramebuffers
#define glDeleteFramebuffers p_glDeleteFramebuffers
#define glBindFramebuffer p_glBindFramebuffer
#define glFramebufferTexture2D p_glFramebufferTexture2D
#define glCheckFramebufferStatus p_glCheckFramebufferStatus

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
    if (!p_glActiveTexture)
        p_glActiveTexture = reinterpret_cast<PFNGLACTIVETEXTUREPROC>(
            load_gl_proc("glActiveTexture"));
}
#endif

using namespace std;

namespace mlns {

static void gl_check(const char* where) {
#ifdef _WIN32
    // On Windows' legacy headers, GL errors are defined; use them to log.
    GLenum err = glGetError();
    if (err != 0) {
        std::cout << "[GL] error 0x" << std::hex << err << std::dec << " at "
                  << where << std::endl;
    }
#endif
}

static void delete_rt(GLRenderTarget& rt) {
    std::cout << "[ZeroCopyGL] delete_rt: fbo=" << rt.fbo
              << " tex=" << rt.color_tex << " size=" << rt.width << "x"
              << rt.height << std::endl;
    if (rt.fbo) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &rt.fbo);
        rt.fbo = 0;
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
    std::cout << "[ZeroCopyGL] setup_if_needed(w=" << w << ", h=" << h << ")"
              << std::endl;
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
        cerr << "[SlintZeroCopyGL] Failed to load OpenGL FBO functions on "
                "Windows."
             << endl;
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

    const auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    std::cout << "[ZeroCopyGL] Created FBO=" << rt_.fbo
              << " tex=" << rt_.color_tex << " status=0x" << std::hex << status
              << std::dec << std::endl;
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        cerr << "[SlintZeroCopyGL] FBO incomplete: 0x" << hex << status << dec
             << endl;
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
        std::cout << "[ZeroCopyGL] on_ready: borrowed tex=" << rt_.color_tex
                  << " size=" << w << "x" << h << std::endl;
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
            std::cout << "[ZeroCopyGL] RenderingSetup: window.size="
                      << window.size().width << "x" << window.size().height
                      << ", target=" << pending_w_ << "x" << pending_h_
                      << ", resources_ready="
                      << (resources_ready_ ? "true" : "false") << std::endl;
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
            std::cout << "[ZeroCopyGL] BeforeRendering: frame="
                      << frame_counter_ << " resources_ready="
                      << (resources_ready_ ? "true" : "false")
                      << ", rt_valid=" << (rt_.valid() ? "true" : "false")
                      << " pending=" << pending_w_ << "x" << pending_h_
                      << std::endl;
            if ((!resources_ready_ || !rt_.valid()) && pending_w_ > 0 &&
                pending_h_ > 0) {
                setup_if_needed(pending_w_, pending_h_);
                resources_ready_ = rt_.valid();
                std::cout << "[ZeroCopyGL] BeforeRendering: setup_if_needed "
                             "done, rt_valid="
                          << (rt_.valid() ? "true" : "false") << std::endl;
                // Warmup: skip rendering on the same frame we created RT
                warmup_done_ = true;
                return;
            } else if (rt_.valid() &&
                       (rt_.width != pending_w_ || rt_.height != pending_h_) &&
                       pending_w_ > 0 && pending_h_ > 0) {
                setup_if_needed(pending_w_, pending_h_);
                resources_ready_ = rt_.valid();
                std::cout
                    << "[ZeroCopyGL] BeforeRendering: resized RT, rt_valid="
                    << (rt_.valid() ? "true" : "false") << std::endl;
                warmup_done_ = true;
                return;
            }
            if (!rt_.valid()) {
                std::cout
                    << "[ZeroCopyGL] BeforeRendering: rt not valid, skip render"
                    << std::endl;
                return;
            }
            if (warmup_done_) {
                warmup_done_ = false;
                std::cout << "[ZeroCopyGL] BeforeRendering: warmup skip"
                          << std::endl;
                return;
            }
            // Render our offscreen content now, before Slint draws this frame.
            // Restore state so Slint/Skia can proceed normally.
            if (in_render_) {
                std::cout
                    << "[ZeroCopyGL] BeforeRendering: re-entrant call, skip"
                    << std::endl;
                break;
            }
            in_render_ = true;
            GLint prevFbo = 0;
            GLint prevViewport[4] = {0, 0, 0, 0};
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
            glGetIntegerv(GL_VIEWPORT, prevViewport);

            std::cout << "[ZeroCopyGL] BeforeRendering: prevFBO=" << prevFbo
                      << " rt.fbo=" << rt_.fbo
                      << " viewport=" << prevViewport[0] << ","
                      << prevViewport[1] << "," << prevViewport[2] << ","
                      << prevViewport[3] << std::endl;

            glBindFramebuffer(GL_FRAMEBUFFER, rt_.fbo);
            gl_check("bind RT fbo");
            glViewport(0, 0, rt_.width, rt_.height);
            gl_check("viewport RT");
            if (on_before_render_) {
                std::cout << "[ZeroCopyGL] BeforeRendering: calling render hook"
                          << std::endl;
                on_before_render_(rt_);
                gl_check("render hook done");
            } else {
                glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                gl_check("clear RT");
            }

            // Leave the GL state in a conservative baseline to avoid
            // interfering with Slint/Skia's presentation path which might run
            // immediately after this callback. On Windows headers many of the
            // functions are legacy; optional modern entry points are used when
            // available and skipped otherwise.
#ifdef _WIN32
            ensure_optional_gl_functions_loaded();
            if (p_glBindVertexArray)
                p_glBindVertexArray(0);
            if (p_glUseProgram)
                p_glUseProgram(0);
            if (p_glBindBuffer) {
                p_glBindBuffer(GL_ARRAY_BUFFER, 0);
                p_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            }
            if (p_glActiveTexture)
                p_glActiveTexture(GL_TEXTURE0);
#endif
            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_SCISSOR_TEST);
            glDisable(GL_STENCIL_TEST);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glDisable(GL_BLEND);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glPixelStorei(GL_PACK_ALIGNMENT, 4);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

            glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFbo));
            glViewport(prevViewport[0], prevViewport[1], prevViewport[2],
                       prevViewport[3]);
            std::cout
                << "[ZeroCopyGL] BeforeRendering: restored prevFBO/viewport"
                << std::endl;
            gl_check("restored prev fbo");
            in_render_ = false;
            break;
        }
        case slint::RenderingState::AfterRendering: {
            // No-op: map texture is already up-to-date for this frame.
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

// ------------------- MapLibre GL Backend + Frontend + Controller
// -------------------
namespace mlns {

// RenderableResource that binds our FBO on draw
class SlintGLRenderableResource final : public mbgl::gfx::RenderableResource {
public:
    explicit SlintGLRenderableResource(class SlintGLBackend& backend_)
        : backend(backend_) {
    }
    void bind() override;

private:
    class SlintGLBackend& backend;
};

// Backend
SlintGLBackend::SlintGLBackend(const mbgl::gfx::ContextMode mode)
    : mbgl::gl::RendererBackend(mode),
      mbgl::gfx::Renderable(
          {0, 0}, std::make_unique<SlintGLRenderableResource>(*this)) {
}

SlintGLBackend::~SlintGLBackend() = default;

void SlintGLRenderableResource::bind() {
    // Slint made the context current already. Ensure FBO + viewport are set.
    assert(mbgl::gfx::BackendScope::exists());
    auto& b = backend;
    b.setFramebufferBinding(static_cast<unsigned int>(b.currentFBO()));
    b.setViewport(0, 0, b.getSize());
}

void SlintGLBackend::updateAssumedState() {
    assumeFramebufferBinding(ImplicitFramebufferBinding);
    assumeViewport(0, 0, size);
}

void SlintGLBackend::updateFramebuffer(uint32_t fbo,
                                       const mbgl::Size& newSize) {
    fbo_ = fbo;
    size = newSize;
}

mbgl::gl::ProcAddress SlintGLBackend::getExtensionFunctionPointer(
    const char* name) {
#ifdef _WIN32
    // Use WGL
    auto p = wglGetProcAddress(name);
    if (!p) {
        HMODULE lib = GetModuleHandleA("opengl32.dll");
        if (lib)
            p = GetProcAddress(lib, name);
    }
    return reinterpret_cast<mbgl::gl::ProcAddress>(p);
#else
    // Fallback: rely on core symbols; return nullptr when unavailable.
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

// Controller
SlintMapLibreGL::SlintMapLibreGL() = default;
SlintMapLibreGL::~SlintMapLibreGL() = default;

void SlintMapLibreGL::initialize(int w, int h) {
    std::cout << "[SlintMapLibreGL] initialize " << w << "x" << h << std::endl;
    width = w;
    height = h;
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

    // Default style
    map->getStyle().loadURL("https://demotiles.maplibre.org/style.json");
}

void SlintMapLibreGL::resize(int w, int h) {
    std::cout << "[SlintMapLibreGL] resize " << w << "x" << h << std::endl;
    width = w;
    height = h;
    if (backend)
        backend->setSize({static_cast<uint32_t>(w), static_cast<uint32_t>(h)});
    if (map)
        map->setSize({static_cast<uint32_t>(w), static_cast<uint32_t>(h)});
}

void SlintMapLibreGL::run_map_loop() {
    if (run_loop)
        run_loop->runOnce();
}

void SlintMapLibreGL::render_to_fbo(uint32_t fbo, int w, int h) {
    std::cout << "[SlintMapLibreGL] render_to_fbo fbo=" << fbo << " size=" << w
              << "x" << h << std::endl;
    if (!backend || !frontend)
        return;
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
    gl_check("before MapLibre render");
    frontend->render();
    gl_check("after MapLibre render");
    if (first_render_) {
        first_render_ = false;
        // Flush first frame once to avoid driver hiccups at startup
        glFinish();
        std::cout << "[SlintMapLibreGL] glFinish after first render"
                  << std::endl;
    }
}

void SlintMapLibreGL::setStyleUrl(const std::string& url) {
    if (map)
        map->getStyle().loadURL(url);
}

void SlintMapLibreGL::handle_mouse_press(float x, float y) {
    last_pos = {x, y};
}

void SlintMapLibreGL::handle_mouse_move(float x, float y, bool pressed) {
    if (pressed && map) {
        mbgl::Point<double> current_pos = {x, y};
        map->moveBy(current_pos - last_pos);
        last_pos = current_pos;
    }
}

void SlintMapLibreGL::handle_double_click(float x, float y, bool shift) {
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

void SlintMapLibreGL::handle_wheel_zoom(float x, float y, float dy) {
    if (!map)
        return;
    constexpr double step = 1.2;
    double scale = (dy < 0.0) ? step : (1.0 / step);
    map->scaleBy(scale, mbgl::ScreenCoordinate{x, y});
}

void SlintMapLibreGL::fly_to(const std::string& location) {
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
