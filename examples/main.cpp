#include "slint_maplibre.hpp"
#include "map_window.h"

#include <memory>
#include <iostream>
#include <vector>

int main(int argc, char** argv) {
    auto main_window = MapWindow::create();
    auto slint_map_libre = std::make_shared<SlintMapLibre>();

    std::vector<slint::SharedString> style_urls_vector = {
        "https://demotiles.maplibre.org/style.json",
        "https://tile.openstreetmap.jp/styles/osm-bright/style.json"
    };
    auto style_urls_model = std::make_shared<slint::VectorModel<slint::SharedString>>(style_urls_vector);
    main_window->global<MapAdapter>().set_style_urls(style_urls_model);

    slint_map_libre->initialize(800, 600);

    // The timer in .slint file will trigger this callback periodically
    main_window->global<MapAdapter>().on_render_map([=]() {
        auto image = slint_map_libre->render_map();
        main_window->global<MapAdapter>().set_map_texture(image);
    });

    main_window->global<MapAdapter>().on_style_changed([=](const slint::SharedString& url) {
        slint_map_libre->setStyleUrl(std::string(url.data(), url.size()));
    });

    // Connect mouse events
    main_window->global<MapAdapter>().on_mouse_press([=](float x, float y) {
        slint_map_libre->handle_mouse_press(x, y);
    });

    main_window->global<MapAdapter>().on_mouse_release([=](float x, float y) {
        slint_map_libre->handle_mouse_release(x, y);
    });

    main_window->global<MapAdapter>().on_mouse_move([=](float x, float y, bool pressed) {
        slint_map_libre->handle_mouse_move(x, y, pressed);
    });

    main_window->run();

    return 0;
}