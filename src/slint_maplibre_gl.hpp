// OpenGL zero-copy scaffold for Slint integration
#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <slint.h>
#include <string>

// Cross‑platform minimal GL includes (no loader). We only need basic types
// and a handful of functions; the final link is provided by OpenGL.
#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#include <TargetConditionals.h>
#else
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#endif

namespace mlns {

// Simple RAII holder for a single‑sample color texture attached to an FBO.
struct GLRenderTarget {
    GLuint fbo = 0;
    GLuint color_tex = 0;
    int width = 0;
    int height = 0;
    bool valid() const {
        return fbo != 0 && color_tex != 0 && width > 0 && height > 0;
    }
};

// Minimal OpenGL zero‑copy pipeline manager.
// Responsibilities:
//  - Create/resize a GL texture + FBO in Slint's GL context
//  - Expose a borrowed Slint Image referencing the texture (BottomLeft origin)
//  - Install a rendering notifier on the Slint window to drive per‑frame hooks
class SlintZeroCopyGL final {
public:
    using RenderHook =
        std::function<void(const GLRenderTarget&)>;  // caller renders into fbo

    SlintZeroCopyGL() = default;
    ~SlintZeroCopyGL();

    // Attach to a Slint window and install a rendering notifier. The provided
    // render hook is invoked every frame with a bound FBO of the current size.
    void attach(slint::Window& window, RenderHook on_before_render);

    // Returns the borrowed image. Recreated only on (re)size.
    std::optional<slint::Image> borrowed_image() const {
        return borrowed_;
    }

    // Notify client when a new borrowed texture becomes available (created or
    // resized). Callback is invoked from Slint's rendering notifier on the UI
    // thread while the GL context is current.
    void set_on_ready(std::function<void(const slint::Image&)> cb) {
        on_ready_ = std::move(cb);
    }

    // Explicitly set the desired render target size (map image area)
    void set_surface_size(int w, int h) {
        desired_w_ = w;
        desired_h_ = h;
        resources_ready_ = false;  // force (re)create on next frame
    }

    // Force teardown now (otherwise handled by notifier).
    void teardown();

private:
    void setup_if_needed(int w, int h);
    void resize(int w, int h);

    GLRenderTarget rt_{};
    std::optional<slint::Image> borrowed_{};
    RenderHook on_before_render_{};
    bool attached_ = false;
    std::function<void(const slint::Image&)> on_ready_{};

    // Defer creation until first AfterRendering to avoid touching GL before
    // Slint's femtovg renderer is fully initialized.
    bool resources_ready_ = false;
    int pending_w_ = 0;
    int pending_h_ = 0;
    bool warmup_done_ =
        false;  // skip rendering on the same frame we created RT
    uint64_t frame_counter_ = 0;
    bool in_render_ = false;  // guard against re-entrancy

    // Desired surface size (map image area). If zero, fall back to window size.
    int desired_w_ = 0;
    int desired_h_ = 0;
};

}  // namespace mlns

// ---- MapLibre OpenGL Backend + Frontend + Controller (zero-copy) ----
// These are placed in the same header for simplicity of this phase.

#include <mbgl/gfx/renderable.hpp>
#include <mbgl/gl/renderer_backend.hpp>
#include <mbgl/map/camera.hpp>
#include <mbgl/map/map.hpp>
#include <mbgl/map/map_observer.hpp>
#include <mbgl/map/map_options.hpp>
#include <mbgl/renderer/renderer.hpp>
#include <mbgl/renderer/renderer_frontend.hpp>
#include <mbgl/storage/resource_options.hpp>
#include <mbgl/style/style.hpp>
#include <mbgl/util/run_loop.hpp>

namespace mlns {

class SlintGLBackend;

class SlintRendererFrontend : public mbgl::RendererFrontend {
public:
    SlintRendererFrontend(std::unique_ptr<mbgl::Renderer> renderer,
                          mbgl::gfx::RendererBackend& backend);
    ~SlintRendererFrontend() override;

    void reset() override;
    void setObserver(mbgl::RendererObserver&) override;
    void update(std::shared_ptr<mbgl::UpdateParameters>) override;
    const mbgl::TaggedScheduler& getThreadPool() const override;
    void render();

    mbgl::Renderer* getRenderer();

private:
    mbgl::gfx::RendererBackend& backend;
    std::unique_ptr<mbgl::Renderer> renderer;
    std::shared_ptr<mbgl::UpdateParameters> updateParameters;
};

class SlintGLBackend final : public mbgl::gl::RendererBackend,
                             public mbgl::gfx::Renderable {
public:
    explicit SlintGLBackend(mbgl::gfx::ContextMode mode);
    ~SlintGLBackend() override;

    mbgl::gfx::Renderable& getDefaultRenderable() override {
        return *this;
    }

    void activate() override {
    }
    void deactivate() override {
    }

    mbgl::gl::ProcAddress getExtensionFunctionPointer(
        const char* name) override;
    void updateAssumedState() override;

    void updateFramebuffer(uint32_t fbo, const mbgl::Size& newSize);
    void setSize(const mbgl::Size& newSize) {
        size = newSize;
    }
    uint32_t currentFBO() const {
        return fbo_;
    }

private:
    uint32_t fbo_ = 0;
};

class SlintMapLibreGL : public mbgl::MapObserver {
public:
    SlintMapLibreGL();
    ~SlintMapLibreGL();

    void initialize(int width, int height);
    void resize(int width, int height);
    void run_map_loop();
    void render_to_fbo(uint32_t fbo, int width, int height);

    void setStyleUrl(const std::string& url);
    void handle_mouse_press(float x, float y);
    void handle_mouse_move(float x, float y, bool pressed);
    void handle_mouse_release(float, float) {
    }
    void handle_double_click(float x, float y, bool shift);
    void handle_wheel_zoom(float x, float y, float dy);
    void fly_to(const std::string& location);

    // MapObserver (no-op for now)
    void onWillStartLoadingMap() override {
    }
    void onDidFinishLoadingStyle() override {
    }
    void onDidBecomeIdle() override {
    }
    void onDidFailLoadingMap(mbgl::MapLoadError, const std::string&) override {
    }

private:
    std::unique_ptr<mbgl::util::RunLoop> run_loop;
    std::unique_ptr<SlintGLBackend> backend;
    std::unique_ptr<SlintRendererFrontend> frontend;
    std::unique_ptr<mbgl::Map> map;

    int width = 0;
    int height = 0;
    mbgl::Point<double> last_pos;
    double min_zoom = 0.0;
    double max_zoom = 22.0;

    bool first_render_ = true;
};

}  // namespace mlns
