#include <iostream>
#include <memory>
#include <vector>

#include "map_window.h"
#include "slint_maplibre.hpp"

int main(int argc, char** argv) {
    std::cout << "[main] Starting application" << std::endl;
    auto main_window = MapWindow::create();
    auto slint_map_libre = std::make_shared<SlintMapLibre>();

    // Delay initialization until a non-zero window size is known
    auto initialized = std::make_shared<bool>(false);
    const auto size = main_window->get_window_size();
    std::cout << "Initial Window Size: " << static_cast<int>(size.width) << "x"
              << static_cast<int>(size.height) << std::endl;

    // This function is ONLY for rendering. It will be called by the observer.
    auto render_function = [=]() {
        std::cout << "Rendering map..." << std::endl;
        auto image = slint_map_libre->render_map();
        main_window->global<MapAdapter>().set_map_texture(image);
    };

    // Pass the render function to SlintMapLibre, which will pass it to the
    // observer.
    slint_map_libre->setRenderCallback(render_function);

    // The timer in .slint file will trigger this callback periodically.
    // This callback drives the MapLibre run loop and, if needed, performs
    // rendering on the UI thread.
    main_window->global<MapAdapter>().on_tick_map_loop([=]() {
        slint_map_libre->run_map_loop();
        if (slint_map_libre->take_repaint_request() ||
            slint_map_libre->consume_forced_repaint()) {
            render_function();
        }
    });

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
        const auto s = main_window->get_map_size();
        const int w = static_cast<int>(s.width);
        const int h = static_cast<int>(s.height);
        std::cout << "Map Area Size Changed: " << w << "x" << h << std::endl;
        if (w > 0 && h > 0) {
            if (!*initialized) {
                slint_map_libre->initialize(w, h);
                *initialized = true;
            } else {
                slint_map_libre->resize(w, h);
            }
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
