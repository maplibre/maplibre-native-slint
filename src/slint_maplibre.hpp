#pragma once

#include <slint.h>
#include <mbgl/map/map.hpp>
#include <mbgl/gfx/headless_frontend.hpp>
#include <mbgl/storage/resource_options.hpp>
#include <mbgl/map/map_options.hpp>

#include "../platform/custom_file_source.hpp"

#include <memory>
#include <string>

class SlintMapLibre {
public:
    SlintMapLibre();
    ~SlintMapLibre();

    void initialize(int width, int height);
    slint::Image render_map();
    void resize(int width, int height);
    void handle_mouse_press(float x, float y);
    void handle_mouse_release(float x, float y);
    void handle_mouse_move(float x, float y, bool pressed);
    void setStyleUrl(const std::string& url);

private:
    std::unique_ptr<mbgl::Map> map;
    std::unique_ptr<mbgl::HeadlessFrontend> frontend;
    std::unique_ptr<mbgl::CustomFileSource> file_source;

    int width = 0;
    int height = 0;

    mbgl::Point<double> last_pos;
};