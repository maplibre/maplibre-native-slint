#include <iostream>
#include <memory>
#include <vector>

#include "map_window.h"
#include "slint_maplibre_headless.hpp"
#ifdef MLNS_WITH_SLINT_GL
#include "slint_maplibre_gl.hpp"
#endif

int main(int argc, char** argv) {
    std::cout << "[main] Starting application" << std::endl;
#ifdef MLNS_WITH_SLINT_GL
    auto use_gl = std::getenv("MLNS_USE_GL");
    bool kUseGL = false;
    if (use_gl) {
        // Treat any non-empty value other than 0/false/off as ON
        std::string val(use_gl);
        for (auto& c : val)
            c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
        if (val == "0" || val == "false" || val == "off" || val.empty())
            kUseGL = false;
        else
            kUseGL = true;
    }
    // Prefer Skia(OpenGL) renderer on Windows when GL path is requested
#ifdef _WIN32
    if (kUseGL) {
        // Clear any previous renderer override and prefer Skia(OpenGL)
        _putenv_s("SLINT_RENDERER", "");
        _putenv_s("SLINT_BACKEND", "winit-skia");
        _putenv_s("SLINT_SKIA_BACKEND", "opengl");
    }
#endif
    std::cout << "[main] GL feature available. MLNS_USE_GL="
              << (use_gl ? use_gl : "<unset>") << " => GL path "
              << (kUseGL ? "ON" : "OFF") << std::endl;
    std::shared_ptr<mlns::SlintZeroCopyGL> zero_copy_gl;
    std::shared_ptr<mlns::SlintMapLibreGL> gl_map;
#else
    const bool kUseGL = false;
    std::cout << "[main] GL feature disabled at build. Using CPU path."
              << std::endl;
#endif
    auto main_window = MapWindow::create();
#ifdef MLNS_WITH_SLINT_GL
    // Debug: print the renderer env we set
    auto env_backend = std::getenv("SLINT_BACKEND");
    auto env_skia = std::getenv("SLINT_SKIA_BACKEND");
    std::cout << "[main] SLINT_BACKEND="
              << (env_backend ? env_backend : "<unset>")
              << " SLINT_SKIA_BACKEND=" << (env_skia ? env_skia : "<unset>")
              << std::endl;
#endif
    std::shared_ptr<SlintMapLibre> slint_map_libre =
        kUseGL ? nullptr : std::make_shared<SlintMapLibre>();

    // Delay initialization until a non-zero window size is known
    auto initialized = std::make_shared<bool>(false);
    const auto size = main_window->get_window_size();
    std::cout << "Initial Window Size: " << static_cast<int>(size.width) << "x"
              << static_cast<int>(size.height) << std::endl;

    // This function is ONLY for rendering in the CPU path.
    auto render_function = [=]() {
        if (kUseGL)
            return;  // GL path is driven by Slint's render notifier
        std::cout << "Rendering map..." << std::endl;
        auto image = slint_map_libre->render_map();
        main_window->global<MapAdapter>().set_map_texture(image);
    };

    // Pass the render function to SlintMapLibre, which will pass it to the
    // observer.
    if (!kUseGL) {
        slint_map_libre->setRenderCallback(render_function);
    }

    // The timer in .slint file will trigger this callback periodically.
    // This callback drives the MapLibre run loop and, if needed, performs
    // rendering on the UI thread.
    main_window->global<MapAdapter>().on_tick_map_loop(
        [=, &zero_copy_gl, &gl_map]() {
            if (kUseGL) {
                if (gl_map)
                    gl_map->run_map_loop();
            } else {
                slint_map_libre->run_map_loop();
                if (slint_map_libre->take_repaint_request() ||
                    slint_map_libre->consume_forced_repaint()) {
                    render_function();
                }
            }
        });

    main_window->global<MapAdapter>().on_style_changed(
        [=](const slint::SharedString& url) {
            std::cout << "[UI] style_changed url="
                      << std::string(url.data(), url.size()) << std::endl;
            if (kUseGL) {
                if (gl_map)
                    gl_map->setStyleUrl(std::string(url.data(), url.size()));
            } else {
                slint_map_libre->setStyleUrl(
                    std::string(url.data(), url.size()));
            }
        });

    // Connect mouse events
    main_window->global<MapAdapter>().on_mouse_press([=](float x, float y) {
        std::cout << "[UI] mouse_press x=" << x << " y=" << y << std::endl;
        if (kUseGL) {
            if (gl_map)
                gl_map->handle_mouse_press(x, y);
        } else {
            slint_map_libre->handle_mouse_press(x, y);
        }
    });

    main_window->global<MapAdapter>().on_mouse_release([=](float x, float y) {
        std::cout << "[UI] mouse_release x=" << x << " y=" << y << std::endl;
        if (kUseGL) {
            if (gl_map)
                gl_map->handle_mouse_release(x, y);
        } else {
            slint_map_libre->handle_mouse_release(x, y);
        }
    });

    main_window->global<MapAdapter>().on_mouse_move([=](float x, float y,
                                                        bool pressed) {
        std::cout << "[UI] mouse_move x=" << x << " y=" << y
                  << " pressed=" << (pressed ? "true" : "false") << std::endl;
        if (kUseGL) {
            if (gl_map)
                gl_map->handle_mouse_move(x, y, pressed);
        } else {
            slint_map_libre->handle_mouse_move(x, y, pressed);
        }
    });

    // Double click zoom with Shift for zoom-out
    main_window->global<MapAdapter>().on_double_click_with_shift(
        [=](float x, float y, bool shift) {
            std::cout << "[UI] double_click x=" << x << " y=" << y
                      << " shift=" << (shift ? "true" : "false") << std::endl;
            if (kUseGL) {
                if (gl_map)
                    gl_map->handle_double_click(x, y, shift);
            } else {
                slint_map_libre->handle_double_click(x, y, shift);
            }
        });

    // Wheel zoom
    main_window->global<MapAdapter>().on_wheel_zoom(
        [=](float x, float y, float dy) {
            std::cout << "[UI] wheel x=" << x << " y=" << y << " dy=" << dy
                      << std::endl;
            if (kUseGL) {
                if (gl_map)
                    gl_map->handle_wheel_zoom(x, y, dy);
            } else {
                slint_map_libre->handle_wheel_zoom(x, y, dy);
            }
        });

    main_window->global<MapAdapter>().on_fly_to(
        [=](const slint::SharedString& location) {
            auto loc = std::string(location.data(), location.size());
            std::cout << "[UI] fly_to loc=" << loc << std::endl;
            if (kUseGL) {
                if (gl_map)
                    gl_map->fly_to(loc);
            } else {
                slint_map_libre->fly_to(loc);
            }
        });

    // Initialize/resize MapLibre to match the map image area
    main_window->on_map_size_changed([=, &zero_copy_gl, &gl_map]() mutable {
        const auto s = main_window->get_map_size();
        const int w = static_cast<int>(s.width);
        const int h = static_cast<int>(s.height);
        std::cout << "Map Area Size Changed: " << w << "x" << h << std::endl;
        if (w > 0 && h > 0) {
            if (!*initialized) {
                if (kUseGL) {
                    gl_map = std::make_shared<mlns::SlintMapLibreGL>();
                    gl_map->initialize(w, h);
                    // Create and attach the GL zero-copy path only after we
                    // have a non-zero window size so Slint's GL renderer can
                    // initialize.
                    std::cout
                        << "[main] Attaching GL notifier after non-zero size"
                        << std::endl;
                    zero_copy_gl = std::make_shared<mlns::SlintZeroCopyGL>();
                    zero_copy_gl->set_on_ready([&](const slint::Image& img) {
                        std::cout << "[main] on_ready: borrowed image set"
                                  << std::endl;
                        main_window->global<MapAdapter>().set_map_texture(img);
                    });
                    auto& win = main_window->window();
                    zero_copy_gl->attach(
                        win, [&](const mlns::GLRenderTarget& rt) {
                            std::cout << "[main] render hook: fbo=" << rt.fbo
                                      << " size=" << rt.width << "x"
                                      << rt.height << std::endl;
                            if (gl_map)
                                gl_map->render_to_fbo(rt.fbo, rt.width,
                                                      rt.height);
                        });
                    // Use the map image area size as render target
                    zero_copy_gl->set_surface_size(w, h);
                } else {
                    slint_map_libre->initialize(w, h);
                }
                *initialized = true;
            } else {
                if (kUseGL) {
                    if (gl_map)
                        gl_map->resize(w, h);
                    if (zero_copy_gl)
                        zero_copy_gl->set_surface_size(w, h);
                } else {
                    slint_map_libre->resize(w, h);
                }
            }
        }
    });

    // GL notifier is attached after first non-zero size in the handler above

    // Note: close-request hook can be added here if needed.

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
