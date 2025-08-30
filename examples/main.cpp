// Metal-only example entry point (CPU fallback removed)
#include <iostream>
#include <memory>
#include <vector>
#include <atomic>
#include <optional>
#include <CoreFoundation/CoreFoundation.h>
#include <mach/mach_time.h>
#include <dispatch/dispatch.h>

#include "map_window.h"
#include "metal/slint_metal_maplibre.hpp"

int main(int argc, char** argv) {
    std::cout << "[main] Starting application" << std::endl;
    auto main_window = MapWindow::create();
    std::cout << "[main] Using Metal zero-copy backend (CPU headless path removed)" << std::endl;
    auto slint_map_libre = std::make_shared<slint_metal::SlintMetalMapLibre>();

    // Delay initialization until a non-zero window size is known
    auto initialized = std::make_shared<bool>(false);
    // Initial size (not printing due to potential macOS MacTypes::Size name clash)
    (void)main_window->get_window_size();

    // No CPU screenshot path remains.

    // The timer in .slint file will trigger this callback periodically.
    // This callback drives the MapLibre run loop and, if needed, performs
    // rendering on the UI thread.
    // --- Frame Scheduler (Metal path) ---------------------------------------------------------
    struct MetalFrameSchedulerCFRunLoop {
        static std::atomic<bool>& needFrame() { static std::atomic<bool> v{false}; return v; }
        static std::atomic<bool>& rendering() { static std::atomic<bool> v{false}; return v; }
        static std::weak_ptr<slint_metal::SlintMetalMapLibre>& weakMap() { static std::weak_ptr<slint_metal::SlintMetalMapLibre> w; return w; }
        static CFRunLoopObserverRef& observer() { static CFRunLoopObserverRef o = nullptr; return o; }
        static std::atomic<uint64_t>& lastFrameNS() { static std::atomic<uint64_t> v{0}; return v; }
        static constexpr uint64_t targetIntervalNS = 16'666'666ull; // 60Hz

        static void setup(const std::shared_ptr<slint_metal::SlintMetalMapLibre>& map) {
            weakMap() = map;
            if (observer() == nullptr) {
                CFRunLoopObserverContext ctx{}; // no custom info needed
                observer() = CFRunLoopObserverCreate(nullptr,
                                                     kCFRunLoopBeforeWaiting | kCFRunLoopAfterWaiting,
                                                     true,
                                                     INT_MAX, // low priority
                                                     [](CFRunLoopObserverRef, CFRunLoopActivity, void*) {
                                                         auto sp = weakMap().lock();
                                                         if (!sp) return;
                                                         // Only render if a frame was requested and we are not already rendering.
                                                         if (!needFrame().exchange(false, std::memory_order_acq_rel)) return;
                                                         bool expected = false;
                                                         if (!rendering().compare_exchange_strong(expected, true)) {
                                                             // Someone else rendering; request another.
                                                             needFrame().store(true, std::memory_order_relaxed);
                                                             return;
                                                         }
                                                         // Frame pacing (simple): enforce minimum interval.
                                                         uint64_t now = mach_absolute_time();
                                                         uint64_t last = lastFrameNS().load(std::memory_order_relaxed);
                                                         if (last != 0 && (now - last) < targetIntervalNS / 2) {
                                                             // Too soon; defer to next loop.
                                                             needFrame().store(true, std::memory_order_relaxed);
                                                             rendering().store(false, std::memory_order_relaxed);
                                                             return;
                                                         }
                                                         sp->render_once();
                                                         lastFrameNS().store(now, std::memory_order_relaxed);
                                                         rendering().store(false, std::memory_order_relaxed);
                                                     },
                                                     &ctx);
                CFRunLoopAddObserver(CFRunLoopGetMain(), observer(), kCFRunLoopCommonModes);
            }
        }

        static void request(const std::shared_ptr<slint_metal::SlintMetalMapLibre>& map) {
            setup(map);
            needFrame().store(true, std::memory_order_relaxed);
        }
    };

    // Tick only signals that a frame is desired; actual render happens after current event processing finishes.
    main_window->global<MapAdapter>().on_tick_map_loop([=]() { MetalFrameSchedulerCFRunLoop::request(slint_map_libre); });

    main_window->global<MapAdapter>().on_style_changed(
        [=](const slint::SharedString& url) {
            slint_map_libre->setStyleUrl(std::string(url.data(), url.size()));
        });

    // Connect mouse events
    main_window->global<MapAdapter>().on_mouse_press(
        [=](float x, float y) { slint_map_libre->handle_mouse_press(x, y); });

    main_window->global<MapAdapter>().on_mouse_release(
        [=](float x, float y) { slint_map_libre->handle_mouse_release(x, y); });

    main_window->global<MapAdapter>().on_mouse_move(
        [=](float x, float y, bool pressed) {
            slint_map_libre->handle_mouse_move(x, y, pressed);
        });

    // Double click zoom with Shift for zoom-out
    main_window->global<MapAdapter>().on_double_click_with_shift(
        [=](float x, float y, bool shift) {
            slint_map_libre->handle_double_click(x, y, shift);
        });

    // Wheel zoom
    main_window->global<MapAdapter>().on_wheel_zoom(
        [=](float x, float y, float dy) {
            slint_map_libre->handle_wheel_zoom(x, y, dy);
        });

    main_window->global<MapAdapter>().on_fly_to(
        [=](const slint::SharedString& location) {
            slint_map_libre->fly_to(
                std::string(location.data(), location.size()));
        });

    // Initialize/resize MapLibre to match the map image area
    main_window->on_map_size_changed([=]() {
    auto s = main_window->get_map_size();
    int w = static_cast<int>(s.width);
    int h = static_cast<int>(s.height);
        std::cout << "Map Area Size Changed: " << w << "x" << h << std::endl;
        if (w > 0 && h > 0) {
            if (!*initialized) { slint_map_libre->initialize(w, h, 1.0f); *initialized = true; }
            else { slint_map_libre->resize(w, h, 1.0f); }
        }
    });

    std::cout << "[main] Entering UI event loop" << std::endl;
    try {
        main_window->run();
        std::cout << "[main] UI event loop exited normally" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[main] Unhandled exception: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[main] Unhandled unknown exception" << std::endl;
    }
    return 1;
}
