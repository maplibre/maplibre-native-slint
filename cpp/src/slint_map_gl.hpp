#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mbgl/map/map.hpp>
#include <mbgl/map/map_observer.hpp>
#include <mbgl/renderer/renderer_observer.hpp>
#include <mbgl/util/run_loop.hpp>
#include <memory>
#include <string>

#include "slint_gl_backend.hpp"

// No-op observer used during orderly shutdown.
class NoopGLRendererObserver final : public mbgl::RendererObserver {
public:
    void onInvalidate() override {
    }
    void onDidFinishRenderingFrame(RenderMode, bool, bool) override {
    }
};

// Renderer observer that flags a repaint when maplibre invalidates.
class SlintGLRendererObserver final : public mbgl::RendererObserver {
public:
    explicit SlintGLRendererObserver(std::function<void()> notify)
        : notify_(std::move(notify)) {
    }

    void onInvalidate() override {
        if (notify_)
            notify_();
    }
    void onDidFinishRenderingFrame(RenderMode, bool needsRepaint,
                                   bool placementChanged) override {
        if (needsRepaint || placementChanged)
            onInvalidate();
    }

private:
    std::function<void()> notify_;
};

class SlintMapGL : public mbgl::MapObserver {
public:
    SlintMapGL() = default;
    ~SlintMapGL() override;

    // Called from Slint's RenderingSetup (GL context current).
    void setup(uint32_t fbo, int w, int h, const std::string& styleUrl);

    // Called from Slint's BeforeRendering (GL context current).
    void render();

    bool style_is_loaded() const {
        return style_loaded.load();
    }

    // Pointer / touch interaction (wired from the Slint UI callbacks).
    void handle_mouse_press(float x, float y);
    void handle_mouse_release();
    void handle_mouse_move(float x, float y, bool pressed);
    void handle_wheel_zoom(float x, float y, float dy);
    void handle_double_click(float x, float y, bool shift);

    // Commands from the toolbar (dropdown / buttons / sliders).
    void setStyleUrl(const std::string& url);
    void fly_to(double lat, double lon, double zoom);
    void set_zoom(double zoom);
    void set_pitch(double pitch);
    void set_bearing(double bearing);

    // MapObserver overrides
    void onWillStartLoadingMap() override;
    void onDidFinishLoadingStyle() override;
    void onDidBecomeIdle() override;
    void onDidFailLoadingMap(mbgl::MapLoadError error,
                             const std::string& what) override;
    void onCameraDidChange(CameraChangeMode) override;
    void onSourceChanged(mbgl::style::Source&) override;
    void onDidFinishRenderingFrame(const RenderFrameStatus&) override;

private:
    std::unique_ptr<mbgl::util::RunLoop> run_loop;
    std::unique_ptr<SlintGLBackend> backend;
    std::unique_ptr<SlintGLFrontend> frontend;
    std::unique_ptr<SlintGLRendererObserver> observer;
    NoopGLRendererObserver noop_observer;
    std::unique_ptr<mbgl::Map> map;

    std::atomic<bool> style_loaded{false};
    std::atomic<bool> map_idle{false};
    std::atomic<bool> repaint{false};
    bool fallback_style_applied{false};

    mbgl::Point<double> last_pos{};
    double min_zoom_ = 0.0;
    double max_zoom_ = 22.0;
    int frame_count_ = 0;
    int fly_ms_ = 2500;  // flyTo duration; override with MAPLIBRE_FLY_MS

    // Manual double-tap detection (touchscreens rarely emit Slint
    // double-clicked).
    std::chrono::steady_clock::time_point last_tap_{};
    float last_tap_x_ = 0.0f;
    float last_tap_y_ = 0.0f;
};
