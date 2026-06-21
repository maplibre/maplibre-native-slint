#include "slint_map_gl.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <mbgl/map/camera.hpp>
#include <mbgl/map/map_options.hpp>
#include <mbgl/renderer/renderer.hpp>
#include <mbgl/storage/resource_options.hpp>
#include <mbgl/style/style.hpp>
#include <mbgl/util/chrono.hpp>
#include <mbgl/util/geo.hpp>

SlintMapGL::~SlintMapGL() {
    // Orderly shutdown: detach observer, then drop map before frontend/backend.
    if (frontend) {
        frontend->setObserver(noop_observer);
    }
    map.reset();
    frontend.reset();
    backend.reset();
    run_loop.reset();
}

void SlintMapGL::setup(uint32_t fbo, int w, int h,
                       const std::string& styleUrl) {
    if (!run_loop) {
        run_loop = std::make_unique<mbgl::util::RunLoop>();
    }

    backend = std::make_unique<SlintGLBackend>(
        mbgl::Size{static_cast<uint32_t>(w), static_cast<uint32_t>(h)});
    backend->setFbo(fbo);

    auto renderer = std::make_unique<mbgl::Renderer>(*backend, 1.0f);
    frontend = std::make_unique<SlintGLFrontend>(std::move(renderer), *backend);

    observer =
        std::make_unique<SlintGLRendererObserver>([this]() { repaint = true; });
    frontend->setObserver(*observer);

    mbgl::ResourceOptions ro;
    ro.withCachePath("cache.sqlite").withAssetPath(".");

    map = std::make_unique<mbgl::Map>(
        *frontend, *this,
        mbgl::MapOptions()
            .withMapMode(mbgl::MapMode::Continuous)
            .withSize({static_cast<uint32_t>(w), static_cast<uint32_t>(h)})
            .withPixelRatio(1.0f),
        ro);

    std::cout << "[SlintMapGL] setup fbo=" << fbo << " size=" << w << "x" << h
              << " style=" << styleUrl << std::endl;

    if (const char* e = std::getenv("MAPLIBRE_FLY_MS")) {
        int v = std::atoi(e);
        if (v > 0)
            fly_ms_ = v;
    }

    map->getStyle().loadURL(styleUrl);
    map->jumpTo(mbgl::CameraOptions()
                    .withCenter(mbgl::LatLng{35.681, 139.767})
                    .withZoom(10.0));
}

void SlintMapGL::render() {
    if (run_loop) {
        run_loop->runOnce();
    }
    if (frontend) {
        frontend->render();
    }
    if ((frame_count_++ % 300) == 0) {
        std::cout << "[SlintMapGL] render frame=" << frame_count_
                  << " style_loaded=" << style_loaded.load() << std::endl;
    }
}

// --- Pointer / touch interaction ---
void SlintMapGL::handle_mouse_press(float x, float y) {
    // Detect a double-tap (two quick taps close together) ourselves, since
    // touchscreens do not reliably produce Slint's double-clicked event.
    // Kernel event timestamps are accurate, so this is robust to input lag.
    auto now = std::chrono::steady_clock::now();
    double dt =
        std::chrono::duration<double, std::milli>(now - last_tap_).count();
    double dist = std::hypot(x - last_tap_x_, y - last_tap_y_);
    if (dt < 350.0 && dist < 30.0) {
        last_tap_ = {};  // reset so a third tap does not re-trigger
        handle_double_click(x, y, false);
        return;
    }
    last_tap_ = now;
    last_tap_x_ = x;
    last_tap_y_ = y;
    last_pos = {x, y};
}

void SlintMapGL::handle_mouse_release() {
}

void SlintMapGL::handle_mouse_move(float x, float y, bool pressed) {
    if (!pressed || !map)
        return;
    mbgl::Point<double> cur{x, y};
    map->moveBy(cur - last_pos);
    last_pos = cur;
    map->triggerRepaint();
    repaint = true;
}

void SlintMapGL::handle_wheel_zoom(float x, float y, float dy) {
    if (!map)
        return;
    constexpr double step = 1.2;
    double scale = (dy < 0.0) ? step : (1.0 / step);
    map->scaleBy(scale, mbgl::ScreenCoordinate{x, y});
    map->triggerRepaint();
    repaint = true;
}

void SlintMapGL::handle_double_click(float x, float y, bool shift) {
    if (!map)
        return;
    const mbgl::LatLng ll = map->latLngForPixel(mbgl::ScreenCoordinate{x, y});
    const auto cam = map->getCameraOptions();
    double z = cam.zoom.value_or(0.0) + (shift ? -1.0 : 1.0);
    z = std::min(max_zoom_, std::max(min_zoom_, z));
    map->jumpTo(mbgl::CameraOptions().withCenter(ll).withZoom(z));
    map->triggerRepaint();
    repaint = true;
}

// --- Toolbar commands ---
void SlintMapGL::setStyleUrl(const std::string& url) {
    if (map) {
        std::cout << "[SlintMapGL] style change: " << url << std::endl;
        map->getStyle().loadURL(url);
        repaint = true;
    }
}

void SlintMapGL::fly_to(double lat, double lon, double zoom) {
    if (!map)
        return;
    mbgl::AnimationOptions anim;
    anim.duration = mbgl::Duration(std::chrono::milliseconds(fly_ms_));
    map->flyTo(
        mbgl::CameraOptions().withCenter(mbgl::LatLng{lat, lon}).withZoom(zoom),
        anim);
    map->triggerRepaint();
    repaint = true;
}

void SlintMapGL::set_zoom(double zoom) {
    if (!map)
        return;
    map->jumpTo(mbgl::CameraOptions().withZoom(zoom));
    map->triggerRepaint();
    repaint = true;
}

void SlintMapGL::set_pitch(double pitch) {
    if (!map)
        return;
    map->jumpTo(mbgl::CameraOptions().withPitch(pitch));
    map->triggerRepaint();
    repaint = true;
}

void SlintMapGL::set_bearing(double bearing) {
    if (!map)
        return;
    map->jumpTo(mbgl::CameraOptions().withBearing(bearing));
    map->triggerRepaint();
    repaint = true;
}

void SlintMapGL::onWillStartLoadingMap() {
    std::cout << "[MapObserver] Will start loading map" << std::endl;
    style_loaded = false;
    map_idle = false;
}

void SlintMapGL::onDidFinishLoadingStyle() {
    std::cout << "[MapObserver] Did finish loading style" << std::endl;
    style_loaded = true;
}

void SlintMapGL::onDidBecomeIdle() {
    std::cout << "[MapObserver] Did become idle" << std::endl;
    map_idle = true;
}

void SlintMapGL::onDidFailLoadingMap(mbgl::MapLoadError error,
                                     const std::string& what) {
    std::cout << "[MapObserver] FAILED loading map. type="
              << static_cast<int>(error) << " what=" << what << std::endl;
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

void SlintMapGL::onCameraDidChange(CameraChangeMode) {
    repaint = true;
}

void SlintMapGL::onSourceChanged(mbgl::style::Source&) {
    repaint = true;
}

void SlintMapGL::onDidFinishRenderingFrame(const RenderFrameStatus& status) {
    if (status.needsRepaint)
        repaint = true;
}
