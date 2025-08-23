#include <iostream>
#include <memory>

#include "map_window.h"
#include "slint_maplibre.hpp"

int main(int argc, char** argv) {
    auto main_window = MapWindow::create();
    auto slint_map_libre = std::make_shared<SlintMapLibre>();

    std::cout << "Initializing MapLibre..." << std::endl;
    slint_map_libre->initialize(800, 600);
    std::cout << "MapLibre initialized." << std::endl;

    // Create a lambda for rendering logic
    auto render_function = [=]() {
        slint_map_libre->run_map_loop();  // Drive the MapLibre event loop
        std::cout << "Rendering map..." << std::endl;
        auto image = slint_map_libre->render_map();
        main_window->global<MapAdapter>().set_map_texture(image);
    };

    // Set the callback for asynchronous rendering requests from MapLibre
    slint_map_libre->setRenderCallback(render_function);

    // The timer in .slint file will trigger this callback periodically for continuous rendering
    main_window->global<MapAdapter>().on_render_map(render_function);

    // Connect mouse events
    main_window->global<MapAdapter>().on_mouse_press(
        [=](float x, float y) { slint_map_libre->handle_mouse_press(x, y); });

    main_window->global<MapAdapter>().on_mouse_release(
        [=](float x, float y) { slint_map_libre->handle_mouse_release(x, y); });

    main_window->global<MapAdapter>().on_mouse_move(
        [=](float x, float y, bool pressed) {
            slint_map_libre->handle_mouse_move(x, y, pressed);
        });

    std::cout << "Starting Slint event loop..." << std::endl;
    main_window->run();
    std::cout << "Slint event loop finished." << std::endl;

    return 0;
}