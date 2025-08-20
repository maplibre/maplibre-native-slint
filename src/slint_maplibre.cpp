#include "slint_maplibre.hpp"

#include "mbgl/gfx/backend_scope.hpp"
#include "mbgl/style/style.hpp"
#include "mbgl/util/logging.hpp"
#include "mbgl/util/geo.hpp"
#include "mbgl/util/premultiply.hpp"

#include <iostream>
#include <memory>

SlintMapLibre::SlintMapLibre() {
    // Note: The API key is managed through CustomFileSource
    file_source = std::make_unique<mbgl::CustomFileSource>();
}

SlintMapLibre::~SlintMapLibre() = default;

void SlintMapLibre::initialize(int w, int h) {
    width = w;
    height = h;

    frontend = std::make_unique<mbgl::HeadlessFrontend>(
        mbgl::Size{static_cast<uint32_t>(width), static_cast<uint32_t>(height)},
        1.0f);

    mbgl::ResourceOptions resourceOptions;
    
    map = std::make_unique<mbgl::Map>(
        *frontend,
        mbgl::MapObserver::nullObserver(),
        mbgl::MapOptions()
            .withMapMode(mbgl::MapMode::Static) // Use Static mode for image rendering
            .withSize(frontend->getSize())
            .withPixelRatio(1.0f),
        resourceOptions);

    map->getStyle().loadURL("https://demotiles.maplibre.org/style.json");
}

void SlintMapLibre::setStyleUrl(const std::string& url) {
    if (map) {
        map->getStyle().loadURL(url);
    }
}

slint::Image SlintMapLibre::render_map() {
    if (!map || !frontend) {
        return {};
    }

    frontend->render(*map);
    mbgl::PremultipliedImage rendered_image = frontend->readStillImage();

    if (rendered_image.data == nullptr || rendered_image.size.isEmpty()) {
        return {};
    }

    auto pixel_buffer = slint::SharedPixelBuffer<slint::Rgba8Pixel>(
        rendered_image.size.width, rendered_image.size.height);

    // Copy pixel data from MapLibre's buffer to Slint's pixel buffer
    memcpy(pixel_buffer.begin(), rendered_image.data.get(), rendered_image.size.width * rendered_image.size.height * sizeof(slint::Rgba8Pixel));

    return slint::Image(pixel_buffer);
}

void SlintMapLibre::resize(int w, int h) {
    width = w;
    height = h;

    if (frontend && map) {
        frontend->setSize({static_cast<uint32_t>(width), static_cast<uint32_t>(height)});
        map->setSize({static_cast<uint32_t>(width), static_cast<uint32_t>(height)});
    }
}

void SlintMapLibre::handle_mouse_press(float x, float y) {
    last_pos = {x, y};
}

void SlintMapLibre::handle_mouse_release(float x, float y) {
    // No action needed for release
}

void SlintMapLibre::handle_mouse_move(float x, float y, bool pressed) {
    if (pressed) {
        mbgl::Point<double> current_pos = {x, y};
        map->moveBy(last_pos - current_pos);
        last_pos = current_pos;
    }
}