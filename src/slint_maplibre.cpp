#include "slint_maplibre.hpp"

#include "mbgl/gfx/backend_scope.hpp"
#include "mbgl/gl/renderer_backend.hpp"
#include "mbgl/style/style.hpp"
#include "mbgl/util/logging.hpp"
#include "mbgl/util/geo.hpp"

#include <iostream>
#include <GLES3/gl3.h>

SlintMapLibre::SlintMapLibre() {
    run_loop = std::make_unique<mbgl::CustomRunLoop>();
    file_source = std::make_unique<mbgl::CustomFileSource>();
}

SlintMapLibre::~SlintMapLibre() {
    if (fbo_id) {
        glDeleteFramebuffers(1, &fbo_id);
    }
    if (texture_id) {
        glDeleteTextures(1, &texture_id);
    }
    if (rbo_id) {
        glDeleteRenderbuffers(1, &rbo_id);
    }
}

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
            .withMapMode(mbgl::MapMode::Continuous)
            .withSize(frontend->getSize())
            .withPixelRatio(1.0f),
        resourceOptions);

    map->getStyle().loadURL("https://demotiles.maplibre.org/style.json");

    // Create FBO
    glGenFramebuffers(1, &fbo_id);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);

    // Create texture
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_id, 0);

    // Create renderbuffer for depth and stencil
    glGenRenderbuffers(1, &rbo_id);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_id);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo_id);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        mbgl::Log::Error(mbgl::Event::OpenGL, "Framebuffer is not complete!");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

slint::Image SlintMapLibre::render_map() {
    if (!map || !frontend) {
        return {};
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);
    glViewport(0, 0, width, height);
    
    frontend->render(*map);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Create Slint image from OpenGL texture
    return slint::Image::create_from_borrowed_gl_2d_rgba_texture(
        texture_id, 
        {static_cast<uint32_t>(width), static_cast<uint32_t>(height)},
        slint::Image::BorrowedOpenGLTextureOrigin::TopLeft
    );
}

void SlintMapLibre::resize(int w, int h) {
    width = w;
    height = h;

    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_id);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

    frontend->setSize({static_cast<uint32_t>(width), static_cast<uint32_t>(height)});
    map->setSize({static_cast<uint32_t>(width), static_cast<uint32_t>(height)});
}

void SlintMapLibre::handle_mouse_press(float x, float y) {
    last_pos = {x, y};
}

void SlintMapLibre::handle_mouse_release(float x, float y) {
    // No action needed for release in this simple example
}

void SlintMapLibre::handle_mouse_move(float x, float y, bool pressed) {
    if (pressed) {
        mbgl::Point<double> current_pos = {x, y};
        map->moveBy(last_pos - current_pos);
        last_pos = current_pos;
    }
}
