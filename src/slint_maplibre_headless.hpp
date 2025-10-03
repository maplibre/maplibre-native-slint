#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <slint.h>
#include <string>

// All required MapLibre headers
#include <mbgl/gfx/headless_frontend.hpp>
#include <mbgl/map/map.hpp>
#include <mbgl/map/map_observer.hpp>
#include <mbgl/map/map_options.hpp>
#include <mbgl/renderer/renderer_observer.hpp>
#include <mbgl/storage/resource_options.hpp>
#include <mbgl/util/run_loop.hpp>

// Custom file source is implemented, but not required for core rendering
// paths used here. We avoid constructing it eagerly to reduce startup
// complexity.

// --- No-op Observer for safe shutdown ---
class NoopRendererObserver final : public mbgl::RendererObserver {
public:
    void onInvalidate() override {
    }
    void onDidFinishRenderingFrame(RenderMode, bool, bool) override {
    }
};

// --- Renderer Observer ---
// Receives repaint requests from MapLibre and forwards them to the Slint event
// loop. This class is now fully defined here to resolve compilation issues.
class SlintRendererObserver : public mbgl::RendererObserver {
public:
    explicit SlintRendererObserver(std::function<void()> notifyRepaint)
        : m_notifyRepaint(std::move(notifyRepaint)) {
    }

    void onInvalidate() override {
        if (m_notifyRepaint)
            m_notifyRepaint();
    }

    void onDidFinishRenderingFrame(RenderMode mode, bool needsRepaint,
                                   bool placementChanged) override {
        if (needsRepaint || placementChanged) {
            onInvalidate();
        }
    }

private:
    std::function<void()> m_notifyRepaint;
};

// --- Main MapLibre Integration Class ---
class SlintMapLibre : public mbgl::MapObserver {
public:
    SlintMapLibre();
    ~SlintMapLibre();

    void initialize(int width, int height);
    void setRenderCallback(std::function<void()> callback);
    slint::Image render_map();
    void resize(int width, int height);
    void handle_mouse_press(float x, float y);
    void handle_mouse_release(float x, float y);
    void handle_mouse_move(float x, float y, bool pressed);
    void handle_double_click(float x, float y, bool shift);
    void handle_wheel_zoom(float x, float y, float dy);
    void set_pitch(int pitch_value);
    void set_bearing(float bearing_value);
    void setStyleUrl(const std::string& url);
    void fly_to(const std::string& location);

    // Manually drive the map's run loop
    void run_map_loop();
    void tick_animation();

    // Repaint signaling consumed by UI thread (timer)
    bool take_repaint_request();
    void request_repaint();
    bool consume_forced_repaint();
    void arm_forced_repaint_ms(int ms);

    // MapObserver implementation
    void onWillStartLoadingMap() override;
    void onDidFinishLoadingStyle() override;
    void onDidBecomeIdle() override;
    void onDidFailLoadingMap(mbgl::MapLoadError error,
                             const std::string& what) override;
    void onCameraDidChange(CameraChangeMode) override;
    void onSourceChanged(mbgl::style::Source&) override;
    void onDidFinishRenderingFrame(const RenderFrameStatus&) override;

private:
    // Declaration order matters for destruction order (bottom-up).
    // The observer must outlive the frontend.
    std::unique_ptr<mbgl::util::RunLoop> run_loop;  // created in initialize()
    std::function<void()> m_renderCallback;

    // Observer and frontend must be declared before the map.
    // The observer must be declared before the frontend to ensure it's
    // destroyed after.
    std::unique_ptr<SlintRendererObserver> m_renderer_observer;
    NoopRendererObserver m_noop_observer;  // For safe shutdown
    std::unique_ptr<mbgl::HeadlessFrontend> frontend;
    std::unique_ptr<mbgl::Map> map;

    int width = 0;
    int height = 0;

    mbgl::Point<double> last_pos;
    double min_zoom = 0.0;
    double max_zoom = 22.0;

    // Style/loading state management
    std::atomic<bool> style_loaded{false};
    std::atomic<bool> map_idle{false};
    std::atomic<bool> repaint_needed{false};

    bool fallback_style_applied{false};
    std::atomic<int> forced_repaint_frames{0};

    struct CustomAnim {
        bool active = false;
        mbgl::LatLng start_center{};
        mbgl::LatLng target_center{};
        double start_zoom = 0.0;
        double target_zoom = 0.0;
        double mid_zoom = 0.0;
        double mid_ratio = 0.35;  // fraction of duration for zoom-out phase
        double center_hold_ratio =
            0.2;  // hold center early to emphasize zoom-out
        std::chrono::steady_clock::time_point start_time{};
        int duration_ms = 0;
    } custom_anim;
};
