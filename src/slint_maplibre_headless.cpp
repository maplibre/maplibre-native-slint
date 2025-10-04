#include "slint_maplibre_headless.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>

#include "mbgl/gfx/backend_scope.hpp"
#include "mbgl/map/bound_options.hpp"
#include "mbgl/map/camera.hpp"
#include "mbgl/style/style.hpp"
#include "mbgl/util/geo.hpp"
#include "mbgl/util/logging.hpp"
#include "mbgl/util/premultiply.hpp"

SlintMapLibre::SlintMapLibre() {
    // Defer RunLoop creation until initialize() when we know sizes and
    // the UI is set up. This reduces the chance of early event-loop
    // interactions before the window exists.
}

SlintMapLibre::~SlintMapLibre() {
    // Orderly shutdown: first, unregister the observer to prevent dangling
    // references.
    if (frontend) {
        frontend->setObserver(m_noop_observer);
    }
    // Next, destroy the map explicitly.
    map.reset();
    // Finally, the rest of the members (frontend, observer, etc.) will be
    // destroyed automatically by their unique_ptrs in the correct order.
}

void SlintMapLibre::initialize(int w, int h) {
    width = w;
    height = h;

    std::cout << "[SlintMapLibre] initialize(" << w << "," << h << ")"
              << std::endl;

    // Initialize RunLoop similar to mbgl-render (one per thread)
    if (!run_loop) {
        std::cout << "[SlintMapLibre] Creating mbgl::util::RunLoop"
                  << std::endl;
        run_loop = std::make_unique<mbgl::util::RunLoop>();
    }

    // Create HeadlessFrontend with the exact same parameters as mbgl-render
    frontend = std::make_unique<mbgl::HeadlessFrontend>(
        mbgl::Size{static_cast<uint32_t>(width), static_cast<uint32_t>(height)},
        1.0f);

    // Set the observer to receive repaint requests (flag-based, UI-safe)
    m_renderer_observer = std::make_unique<SlintRendererObserver>([this]() {
        request_repaint();
        // Keep a short burst of frames to drain the queue
        arm_forced_repaint_ms(100);
    });
    frontend->setObserver(*m_renderer_observer);

    // Set ResourceOptions same as mbgl-render
    mbgl::ResourceOptions resourceOptions;
    resourceOptions.withCachePath("cache.sqlite").withAssetPath(".");

    // Set MapOptions same as mbgl-render
    map = std::make_unique<mbgl::Map>(
        *frontend,
        *this,  // Use this instance as MapObserver
        mbgl::MapOptions()
            .withMapMode(
                mbgl::MapMode::Continuous)  // Use Continuous for animations
            .withSize(frontend->getSize())
            .withPixelRatio(1.0f),
        resourceOptions);

    // Constrain zoom range
    map->setBounds(
        mbgl::BoundOptions().withMinZoom(min_zoom).withMaxZoom(max_zoom));

    // Set a more reliable background color style
    std::cout << "Setting solid background color style..." << std::endl;
    std::string simple_style = R"JSON({
        "version": 8,
        "name": "solid-background",
        "sources": {},
        "layers": [
            {
                "id": "background",
                "type": "background",
                "paint": {
                    "background-color": "rgb(255, 0, 0)",
                    "background-opacity": 1.0
                }
            }
        ]
    })JSON";
    // Try remote MapLibre demo style first; fall back to local JSON on error
    std::cout << "Loading remote MapLibre style..." << std::endl;
    map->getStyle().loadURL("https://demotiles.maplibre.org/style.json");

    // Set initial display position (around Tokyo)
    // std::cout << "Setting initial map position..." << std::endl;
    // map->jumpTo(mbgl::CameraOptions()
    //     .withCenter(mbgl::LatLng{35.6762, 139.6503}) // Tokyo
    //    .withZoom(10.0));

    std::cout << "[SlintMapLibre] Map initialization completed" << std::endl;
}

void SlintMapLibre::setRenderCallback(std::function<void()> callback) {
    m_renderCallback = std::move(callback);
}

// MapObserver implementation
void SlintMapLibre::onWillStartLoadingMap() {
    std::cout << "[MapObserver] Will start loading map" << std::endl;
    style_loaded = false;
    map_idle = false;
}

void SlintMapLibre::onDidFinishLoadingStyle() {
    std::cout << "[MapObserver] Did finish loading style" << std::endl;
    style_loaded = true;
}

void SlintMapLibre::onDidBecomeIdle() {
    std::cout << "[MapObserver] Did become idle" << std::endl;
    map_idle = true;
}

void SlintMapLibre::onDidFailLoadingMap(mbgl::MapLoadError error,
                                        const std::string& what) {
    std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
    std::cout << "[MapObserver] FAILED loading map." << std::endl;
    std::cout << "    Error type: " << static_cast<int>(error) << std::endl;
    std::cout << "    What: " << what << std::endl;
    std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
    if (!fallback_style_applied && map) {
        fallback_style_applied = true;
        std::cout << "[MapObserver] Applying fallback local JSON style"
                  << std::endl;
        const std::string fallback_json = R"JSON({
            "version": 8,
            "name": "solid-background",
            "sources": {},
            "layers": [
                {"id": "background", "type": "background",
                 "paint": {"background-color": "rgb(255, 0, 0)",
                            "background-opacity": 1.0}}]
        })JSON";
        map->getStyle().loadJSON(fallback_json);
    }
}

void SlintMapLibre::onCameraDidChange(CameraChangeMode) {
    request_repaint();
    arm_forced_repaint_ms(100);
}

void SlintMapLibre::onSourceChanged(mbgl::style::Source&) {
    request_repaint();
    arm_forced_repaint_ms(100);
}

void SlintMapLibre::onDidFinishRenderingFrame(const RenderFrameStatus& status) {
    if (status.needsRepaint) {
        request_repaint();
        arm_forced_repaint_ms(100);
    }
}

void SlintMapLibre::setStyleUrl(const std::string& url) {
    if (map) {
        map->getStyle().loadURL(url);
    }
}

slint::Image SlintMapLibre::render_map() {
    std::cout << "render_map() called" << std::endl;

    if (!map || !frontend) {
        std::cout << "ERROR: map or frontend is null" << std::endl;
        return {};
    }

    // Wait for style to finish loading
    if (!style_loaded.load()) {
        std::cout << "Style not loaded yet, returning empty image" << std::endl;
        return {};  // Return an empty image
    }

    std::cout << "Style loaded, proceeding with rendering..." << std::endl;

    // Use the exact same rendering method as mbgl-render
    std::cout << "Using frontend.render(map) like mbgl-render..." << std::endl;
    // Ensure a valid backend scope is active for rendering (required on some
    // platforms/drivers, notably Windows) to make the GL context current.
    if (auto* backend = frontend->getBackend()) {
        mbgl::gfx::BackendScope scope{*backend};
        frontend->renderOnce(*map);
        std::cout << "Rendered one frame, reading still image..." << std::endl;
        mbgl::PremultipliedImage rendered_image = frontend->readStillImage();
        std::cout << "Image size: " << rendered_image.size.width << "x"
                  << rendered_image.size.height << std::endl;
        std::cout << "Image data pointer: "
                  << (rendered_image.data.get() ? "valid" : "null")
                  << std::endl;

        if (rendered_image.data == nullptr || rendered_image.size.isEmpty()) {
            std::cout << "ERROR: frontend->render() returned empty data"
                      << std::endl;
            return {};
        }

        std::cout << "Converting from premultiplied to unpremultiplied..."
                  << std::endl;
        mbgl::UnassociatedImage unpremult_image =
            mbgl::util::unpremultiply(std::move(rendered_image));

        std::cout << "Creating Slint pixel buffer..." << std::endl;
        auto pixel_buffer = slint::SharedPixelBuffer<slint::Rgba8Pixel>(
            unpremult_image.size.width, unpremult_image.size.height);

        std::cout << "Copying unpremultiplied pixel data..." << std::endl;
        memcpy(pixel_buffer.begin(), unpremult_image.data.get(),
               unpremult_image.size.width * unpremult_image.size.height *
                   sizeof(slint::Rgba8Pixel));

        const uint8_t* raw_data = unpremult_image.data.get();
        std::cout << "Pixel samples (RGBA): ";
        for (int i = 0; i < 20 && i < unpremult_image.size.width *
                                          unpremult_image.size.height;
             i += 5) {
            int offset = i * 4;
            std::cout << "(" << (int)raw_data[offset] << ","
                      << (int)raw_data[offset + 1] << ","
                      << (int)raw_data[offset + 2] << ","
                      << (int)raw_data[offset + 3] << ") ";
        }
        std::cout << std::endl;

        int non_transparent_count = 0;
        for (int i = 0;
             i < unpremult_image.size.width * unpremult_image.size.height;
             i++) {
            if (raw_data[i * 4 + 3] > 0) {
                non_transparent_count++;
            }
        }
        std::cout << "Non-transparent pixels: " << non_transparent_count
                  << " / "
                  << (unpremult_image.size.width * unpremult_image.size.height)
                  << std::endl;

        std::cout << "Image created successfully" << std::endl;
        return slint::Image(pixel_buffer);
    } else {
        std::cout << "ERROR: frontend->getBackend() returned null" << std::endl;
        return {};
    }
}

void SlintMapLibre::resize(int w, int h) {
    width = w;
    height = h;

    if (frontend && map) {
        frontend->setSize(
            {static_cast<uint32_t>(width), static_cast<uint32_t>(height)});
        map->setSize(
            {static_cast<uint32_t>(width), static_cast<uint32_t>(height)});
    }
}

void SlintMapLibre::handle_mouse_press(float x, float y) {
    last_pos = {x, y};
    // Trigger a redraw after interaction starts for responsiveness
    request_repaint();
    arm_forced_repaint_ms(120);
}

void SlintMapLibre::handle_mouse_release(float x, float y) {
    // No action needed for release
}

void SlintMapLibre::handle_mouse_move(float x, float y, bool pressed) {
    if (pressed) {
        mbgl::Point<double> current_pos = {x, y};
        mbgl::Point<double> delta = current_pos - last_pos;
        // Move the map along with the pointer movement (dragging behavior)
        map->moveBy(delta);
        last_pos = current_pos;
        map->triggerRepaint();
    }
}

void SlintMapLibre::handle_double_click(float x, float y, bool shift) {
    if (!map)
        return;
    // Center the map on the clicked location and zoom by one level (+/- with
    // Shift)
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
    map->triggerRepaint();
}

void SlintMapLibre::handle_wheel_zoom(float x, float y, float dy) {
    if (!map)
        return;
    // Lower sensitivity: dy < 0 => zoom in, dy > 0 => zoom out
    constexpr double step = 1.2;  // smoother than 2.0
    double scale = (dy < 0.0) ? step : (1.0 / step);
    map->scaleBy(scale, mbgl::ScreenCoordinate{x, y});
    map->triggerRepaint();
}

void SlintMapLibre::set_pitch(int pitch_value) {
    if (!map)
        return;
    // Convert slider value (0-100) to pitch in degrees (0-60)
    double pitch = (pitch_value / 100.0) * 60.0;

    // Get current camera state and update pitch
    const auto cam = map->getCameraOptions();
    mbgl::CameraOptions next;
    next.withCenter(cam.center)
        .withZoom(cam.zoom)
        .withBearing(cam.bearing)
        .withPitch(std::optional<double>(pitch));

    map->jumpTo(next);
    map->triggerRepaint();
}

void SlintMapLibre::set_bearing(float bearing_value) {
    if (!map)
        return;
    // Convert slider value (0-100) to bearing in degrees (0-360)
    double bearing = (bearing_value / 100.0) * 360.0;

    // Get current camera state and update bearing
    const auto cam = map->getCameraOptions();
    mbgl::CameraOptions next;
    next.withCenter(cam.center)
        .withZoom(cam.zoom)
        .withPitch(cam.pitch)
        .withBearing(std::optional<double>(bearing));

    map->jumpTo(next);
    map->triggerRepaint();
}

void SlintMapLibre::run_map_loop() {
    if (run_loop) {
        run_loop->runOnce();
    } else {
        // Not initialized yet; nothing to pump.
    }
    // Drive custom animation if active
    tick_animation();
}

bool SlintMapLibre::take_repaint_request() {
    bool expected = true;
    return repaint_needed.compare_exchange_strong(expected, false,
                                                  std::memory_order_seq_cst);
}

void SlintMapLibre::request_repaint() {
    repaint_needed.store(true, std::memory_order_relaxed);
}

bool SlintMapLibre::consume_forced_repaint() {
    int v = forced_repaint_frames.load(std::memory_order_relaxed);
    if (v > 0) {
        forced_repaint_frames.store(v - 1, std::memory_order_relaxed);
        return true;
    }
    return false;
}

void SlintMapLibre::arm_forced_repaint_ms(int ms) {
    int frames = std::max(1, ms / 16);
    int cur = forced_repaint_frames.load(std::memory_order_relaxed);
    if (frames > cur) {
        forced_repaint_frames.store(frames, std::memory_order_relaxed);
    }
}

void SlintMapLibre::fly_to(const std::string& location) {
    if (!map)
        return;

    // Determine target
    mbgl::LatLng target;
    if (location == "paris") {
        target = mbgl::LatLng{48.8566, 2.3522};
    } else if (location == "new_york") {
        target = mbgl::LatLng{40.7128, -74.0060};
    } else {  // tokyo or default
        target = mbgl::LatLng{35.6895, 139.6917};
    }

    // Capture start camera
    const auto cam = map->getCameraOptions();
    mbgl::LatLng start_center = cam.center.value_or(target);
    double start_zoom = cam.zoom.value_or(10.0);
    double target_zoom = 10.0;
    // Dynamic zoom-out amount based on approximate great-circle distance
    auto deg2rad = [](double d) { return d * M_PI / 180.0; };
    auto approx_distance_deg = [&](const mbgl::LatLng& a,
                                   const mbgl::LatLng& b) {
        double lat1 = deg2rad(a.latitude());
        double lat2 = deg2rad(b.latitude());
        double dlat = lat2 - lat1;
        double dlon = deg2rad(b.longitude() - a.longitude());
        double x = dlon * std::cos((lat1 + lat2) * 0.5);
        double y = dlat;
        return std::sqrt(x * x + y * y) * 180.0 / M_PI;
    };
    double dist = approx_distance_deg(start_center, target);
    // Bolder zoom-out: higher base, stronger distance gain, higher cap
    double zoom_out_delta = 8.0 + std::min(3.0, dist / 8.0);
    // Ensure at least a meaningful pull-back even for short hops
    zoom_out_delta = std::max(2.0, zoom_out_delta);
    // Plan zoom-out then zoom-in: first zoom to a lower mid level, then in
    double mid_zoom =
        std::max(min_zoom, std::min(max_zoom, start_zoom - zoom_out_delta));

    // Setup custom animation state (2.5s)
    custom_anim.active = true;
    custom_anim.start_center = start_center;
    custom_anim.target_center = target;
    custom_anim.start_zoom = start_zoom;
    custom_anim.target_zoom = target_zoom;
    custom_anim.mid_zoom = mid_zoom;
    custom_anim.mid_ratio =
        0.60;  // 60% zoom-out, 40% zoom-in (emphasize pull-back)
    custom_anim.center_hold_ratio = 0.20;  // keep center almost still at first
    custom_anim.start_time = std::chrono::steady_clock::now();
    custom_anim.duration_ms = 2500;
    request_repaint();
    arm_forced_repaint_ms(custom_anim.duration_ms + 600);
}

static inline double ease_in_out(double t) {
    // Smoothstep-like cubic easing
    return t < 0.5 ? 4 * t * t * t : 1 - std::pow(-2 * t + 2, 3) / 2;
}

void SlintMapLibre::tick_animation() {
    if (!custom_anim.active || !map)
        return;
    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                             now - custom_anim.start_time)
                             .count();
    double t = std::clamp(
        elapsed / static_cast<double>(custom_anim.duration_ms), 0.0, 1.0);
    double k = ease_in_out(t);

    auto lerp = [](double a, double b, double k) { return a + (b - a) * k; };
    // Delay center movement at the beginning to accentuate zoom-out
    double k_center;
    if (t <= custom_anim.center_hold_ratio) {
        double t_hold = (custom_anim.center_hold_ratio > 0.0)
                            ? t / custom_anim.center_hold_ratio
                            : 1.0;
        k_center = 0.10 * ease_in_out(t_hold);  // only 10% move during hold
    } else {
        double t_rest = (t - custom_anim.center_hold_ratio) /
                        std::max(1e-6, 1.0 - custom_anim.center_hold_ratio);
        k_center = 0.10 + 0.90 * ease_in_out(t_rest);
    }
    mbgl::LatLng c{lerp(custom_anim.start_center.latitude(),
                        custom_anim.target_center.latitude(), k_center),
                   lerp(custom_anim.start_center.longitude(),
                        custom_anim.target_center.longitude(), k_center)};
    // Two-phase zoom: out then in
    double z;
    if (t <= custom_anim.mid_ratio) {
        double t0 =
            (custom_anim.mid_ratio > 0.0) ? t / custom_anim.mid_ratio : 1.0;
        double k0 = ease_in_out(t0);
        z = lerp(custom_anim.start_zoom, custom_anim.mid_zoom, k0);
    } else {
        double t1 = (t - custom_anim.mid_ratio) /
                    std::max(1e-6, 1.0 - custom_anim.mid_ratio);
        double k1 = ease_in_out(t1);
        z = lerp(custom_anim.mid_zoom, custom_anim.target_zoom, k1);
    }

    mbgl::CameraOptions next;
    next.center = c;
    next.zoom = z;
    map->jumpTo(next);
    request_repaint();

    if (t >= 1.0) {
        custom_anim.active = false;
    }
}
