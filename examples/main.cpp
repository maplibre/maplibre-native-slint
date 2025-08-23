#include <iostream>
#include <memory>
#include <vector>

#include "map_window.h"
#include "slint_maplibre.hpp"

int main(int argc, char** argv) {
    auto main_window = MapWindow::create();
    auto slint_map_libre = std::make_shared<SlintMapLibre>();

    // Delay initialization until a non-zero window size is known
    auto initialized = std::make_shared<bool>(false);
    const auto size = main_window->get_window_size();
    std::cout << "Initial Window Size: " << static_cast<int>(size.width) << "x"
              << static_cast<int>(size.height) << std::endl;

    // Create a lambda for rendering logic
    auto render_function = [=]() {
        slint_map_libre->run_map_loop();  // Drive the MapLibre event loop
        std::cout << "Rendering map..." << std::endl;
        auto image = slint_map_libre->render_map();
        main_window->global<MapAdapter>().set_map_texture(image);
    };

    // Set the callback for asynchronous rendering requests from MapLibre
    slint_map_libre->setRenderCallback(render_function);

    // The timer in .slint file will trigger this callback periodically for
    // continuous rendering
    main_window->global<MapAdapter>().on_render_map(render_function);

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

    main_window->run();

    return 0;
}
