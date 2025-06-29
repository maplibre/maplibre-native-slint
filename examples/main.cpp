#include "slint_maplibre.hpp"
#include "map_window.h"

#include <memory>
#include <iostream>

// Rendering notifier for MapLibre integration
class MapRenderer {
public:
    MapRenderer(slint::ComponentHandle<MapWindow> app, std::shared_ptr<SlintMapLibre> maplibre)
        : app_weak(app), maplibre(maplibre) {}

    void operator()(slint::RenderingState state, slint::GraphicsAPI) {
        switch (state) {
        case slint::RenderingState::RenderingSetup:
            std::cout << "Rendering setup" << std::endl;
            break;
        case slint::RenderingState::BeforeRendering:
            if (auto app = app_weak.lock()) {
                // Render MapLibre to texture and update Slint
                auto texture = maplibre->render_map();
                (*app)->global<MapAdapter>().set_map_texture(texture);
                (*app)->window().request_redraw();
            }
            break;
        case slint::RenderingState::AfterRendering:
            break;
        case slint::RenderingState::RenderingTeardown:
            std::cout << "Rendering teardown" << std::endl;
            break;
        }
    }

private:
    slint::ComponentWeakHandle<MapWindow> app_weak;
    std::shared_ptr<SlintMapLibre> maplibre;
};

int main(int argc, char** argv) {
    auto main_window = MapWindow::create();
    auto slint_map_libre = std::make_shared<SlintMapLibre>();

    // Initialize MapLibre with window size
    slint_map_libre->initialize(800, 600);

    // Set up rendering notifier
    auto renderer = std::make_shared<MapRenderer>(main_window, slint_map_libre);
    main_window->window().set_rendering_notifier(
        [renderer](slint::RenderingState state, slint::GraphicsAPI api) {
            (*renderer)(state, api);
        }
    );

    // Connect mouse events
    main_window->global<MapAdapter>().on_render_map([=]() {
        main_window->window().request_redraw();
    });

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
