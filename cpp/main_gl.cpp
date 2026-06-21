#include <GLES3/gl3.h>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <slint.h>
#include <string>

#include "gl_map_window.h"
#include "slint_map_gl.hpp"

int main(int /*argc*/, char** /*argv*/) {
    std::cout << "[main_gl] Starting zero-copy GL application" << std::endl;

    auto win = MapWindow::create();
    auto smap = std::make_shared<SlintMapGL>();

    std::string styleUrl = "https://demotiles.maplibre.org/style.json";
    if (const char* env = std::getenv("MAPLIBRE_STYLE_URL")) {
        if (env[0] != '\0')
            styleUrl = env;
    }

    // Optional explicit render size; otherwise the display's native size is
    // used (so it fills the screen at any resolution, including small panels).
    int envW = 0, envH = 0;
    if (const char* e = std::getenv("MAPLIBRE_WIDTH"))
        envW = std::atoi(e);
    if (const char* e = std::getenv("MAPLIBRE_HEIGHT"))
        envH = std::atoi(e);

    auto gl_ready = std::make_shared<bool>(false);
    auto tex = std::make_shared<GLuint>(0);
    auto fbo = std::make_shared<GLuint>(0);
    auto rbo = std::make_shared<GLuint>(0);
    auto Wp = std::make_shared<int>(0);
    auto Hp = std::make_shared<int>(0);

    win->window().set_rendering_notifier([=](slint::RenderingState state,
                                             slint::GraphicsAPI api) {
        switch (state) {
        case slint::RenderingState::RenderingSetup: {
            if (api != slint::GraphicsAPI::NativeOpenGL) {
                std::cout << "[main_gl] WARNING: GraphicsAPI is not "
                             "NativeOpenGL; zero-copy GL path unavailable"
                          << std::endl;
                return;
            }

            auto ps = win->window().size();
            int w = envW > 0
                        ? envW
                        : (ps.width > 0 ? static_cast<int>(ps.width) : 1280);
            int h = envH > 0
                        ? envH
                        : (ps.height > 0 ? static_cast<int>(ps.height) : 720);
            *Wp = w;
            *Hp = h;
            std::cout << "[main_gl] RenderingSetup: NativeOpenGL acquired, "
                         "render size "
                      << w << "x" << h << std::endl;

            glGenTextures(1, tex.get());
            glBindTexture(GL_TEXTURE_2D, *tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, nullptr);

            glGenRenderbuffers(1, rbo.get());
            glBindRenderbuffer(GL_RENDERBUFFER, *rbo);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);

            glGenFramebuffers(1, fbo.get());
            glBindFramebuffer(GL_FRAMEBUFFER, *fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D, *tex, 0);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                      GL_DEPTH_STENCIL_ATTACHMENT,
                                      GL_RENDERBUFFER, *rbo);

            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            std::cout << "[main_gl] FBO status="
                      << (status == GL_FRAMEBUFFER_COMPLETE
                              ? "GL_FRAMEBUFFER_COMPLETE"
                              : std::to_string(status))
                      << " fbo=" << *fbo << " tex=" << *tex << std::endl;

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            smap->setup(*fbo, w, h, styleUrl);
            *gl_ready = true;
            break;
        }
        case slint::RenderingState::BeforeRendering: {
            if (!*gl_ready)
                return;

            GLint pf = 0;
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &pf);
            GLint vp[4] = {0, 0, 0, 0};
            glGetIntegerv(GL_VIEWPORT, vp);
            GLint prog = 0;
            glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
            GLint arrayBuf = 0;
            glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBuf);
            GLint elemBuf = 0;
            glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &elemBuf);
            GLint activeTex = 0;
            glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTex);
            GLboolean blend = glIsEnabled(GL_BLEND);
            GLboolean depth = glIsEnabled(GL_DEPTH_TEST);
            GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
            GLboolean cull = glIsEnabled(GL_CULL_FACE);

            smap->render();

            glBindFramebuffer(GL_FRAMEBUFFER, pf);
            glViewport(vp[0], vp[1], vp[2], vp[3]);
            glUseProgram(prog);
            glBindBuffer(GL_ARRAY_BUFFER, arrayBuf);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBuf);
            glActiveTexture(activeTex);
            if (blend)
                glEnable(GL_BLEND);
            else
                glDisable(GL_BLEND);
            if (depth)
                glEnable(GL_DEPTH_TEST);
            else
                glDisable(GL_DEPTH_TEST);
            if (scissor)
                glEnable(GL_SCISSOR_TEST);
            else
                glDisable(GL_SCISSOR_TEST);
            if (cull)
                glEnable(GL_CULL_FACE);
            else
                glDisable(GL_CULL_FACE);

            win->global<MMapAdapter>().set_frame(
                slint::Image::create_from_borrowed_gl_2d_rgba_texture(
                    *tex,
                    {static_cast<uint32_t>(*Wp), static_cast<uint32_t>(*Hp)},
                    slint::Image::BorrowedOpenGLTextureOrigin::BottomLeft));
            win->window().request_redraw();
            break;
        }
        case slint::RenderingState::AfterRendering:
            break;
        case slint::RenderingState::RenderingTeardown: {
            std::cout << "[main_gl] RenderingTeardown" << std::endl;
            if (*fbo)
                glDeleteFramebuffers(1, fbo.get());
            if (*rbo)
                glDeleteRenderbuffers(1, rbo.get());
            if (*tex)
                glDeleteTextures(1, tex.get());
            *fbo = *rbo = *tex = 0;
            *gl_ready = false;
            break;
        }
        }
    });

    // Touch / pointer interaction (Slint delivers touch via libinput as pointer
    // events; the MMapView forwards them through these MMapAdapter callbacks).
    win->global<MMapAdapter>().on_mouse_pressed(
        [=](float x, float y) { smap->handle_mouse_press(x, y); });
    win->global<MMapAdapter>().on_mouse_released(
        [=](float, float) { smap->handle_mouse_release(); });
    win->global<MMapAdapter>().on_mouse_moved(
        [=](float x, float y) { smap->handle_mouse_move(x, y, true); });
    // Double-tap is detected inside handle_mouse_press (touchscreens do not
    // reliably emit Slint's double-clicked), so on_double_clicked is left
    // unwired to avoid double-triggering with a real mouse.
    win->global<MMapAdapter>().on_wheel_zoomed(
        [=](float x, float y, float dy) { smap->handle_wheel_zoom(x, y, dy); });

    // Toolbar commands (dropdown / buttons / sliders).
    win->global<MMapAdapter>().on_request_style_change(
        [=](const slint::SharedString& u) {
            smap->setStyleUrl(std::string(u.data(), u.size()));
        });
    win->global<MMapAdapter>().on_request_fly_to(
        [=](float lat, float lon, float z) { smap->fly_to(lat, lon, z); });
    win->global<MMapAdapter>().on_request_zoom_change(
        [=](float z) { smap->set_zoom(z); });
    win->global<MMapAdapter>().on_request_pitch_change(
        [=](float p) { smap->set_pitch(p); });
    win->global<MMapAdapter>().on_request_bearing_change(
        [=](float b) { smap->set_bearing(b); });

    win->on_map_size_changed([=]() {});

    std::cout << "[main_gl] Entering UI event loop" << std::endl;
    win->run();
    return 0;
}
