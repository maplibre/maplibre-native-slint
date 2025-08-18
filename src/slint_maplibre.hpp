#pragma once

#include <slint.h>
#include <mbgl/map/map.hpp>
#include <mbgl/gfx/headless_frontend.hpp>
#include <mbgl/storage/resource_options.hpp>
#include <mbgl/map/map_options.hpp>

#include "../platform/custom_file_source.hpp"
#include "../platform/custom_run_loop.hpp"

#include <memory>

#ifdef __APPLE__
#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION
#endif
#include <OpenGL/gl3.h>
#else
#include <GLES3/gl3.h>
#endif

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

private:
    void setup_opengl_resources();
    void cleanup_opengl_resources();
    
    std::unique_ptr<mbgl::Map> map;
    std::unique_ptr<mbgl::HeadlessFrontend> frontend;
    std::unique_ptr<mbgl::CustomFileSource> file_source;
    std::unique_ptr<mbgl::CustomRunLoop> run_loop;

    GLuint fbo_id = 0;
    GLuint texture_id = 0;
    GLuint rbo_id = 0;

    int width = 0;
    int height = 0;

    mbgl::Point<double> last_pos;
};
