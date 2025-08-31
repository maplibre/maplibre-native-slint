#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include "map_window.h"
#include "slint_maplibre_headless.hpp"
#ifdef MLNS_WITH_SLINT_GL
#include "gl/slint_gl_maplibre.hpp"
#endif

static std::string now_ts() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto t = system_clock::to_time_t(now);
    std::tm lt{};
#ifdef _WIN32
    localtime_s(&lt, &t);
#else
    localtime_r(&t, &lt);
#endif
    const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    std::ostringstream oss;
    oss << std::put_time(&lt, "%Y-%m-%d %H:%M:%S") << "." << std::setw(3)
        << std::setfill('0') << ms.count();
    return oss.str();
}

#define LOGI(expr)                                               \
    do {                                                         \
        std::ostringstream _oss;                                 \
        _oss << expr;                                            \
        std::cout << now_ts() << " " << _oss.str() << std::endl; \
    } while (0)
#define LOGE(expr)                                               \
    do {                                                         \
        std::ostringstream _oss;                                 \
        _oss << expr;                                            \
        std::cerr << now_ts() << " " << _oss.str() << std::endl; \
    } while (0)

int main(int argc, char** argv) {
    LOGI("[main] Starting application");
#ifdef MLNS_WITH_SLINT_GL
    // 選択肢B: SLINT_RENDERER=gl が見えたら無条件で GL 経路（最優先）。
    // 追加: skia-opengl もデスクトップ OpenGL(WGL) の可能性が高いので許可。
    // 回避: MLNS_DISABLE_GL_ZEROCOPY=1 なら GL ゼロコピーを無効化して CPU
    // パスにフォールバック。
    bool kUseGL = false;
    bool disable_gl_zerocopy = false;
    if (const char* dis = std::getenv("MLNS_DISABLE_GL_ZEROCOPY")) {
        std::string v(dis);
        for (auto& c : v)
            c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
        if (v == "1" || v == "true" || v == "on" || v == "yes") {
            disable_gl_zerocopy = true;
        }
    }
    const char* sr_env = std::getenv("SLINT_RENDERER");
    if (!disable_gl_zerocopy && !kUseGL && sr_env) {
        std::string r(sr_env);
        for (auto& c : r)
            c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
        if (r == "gl" || r == "skia-opengl")
            kUseGL = true;
    }
    // 追加の緩和: 明示の MLNS_USE_GL が true 系なら GL 経路に寄せる（従属）。
    if (!disable_gl_zerocopy && !kUseGL) {
        if (const char* use_gl = std::getenv("MLNS_USE_GL")) {
            std::string val(use_gl);
            for (auto& c : val)
                c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
            if (val == "1" || val == "true" || val == "on" || val == "yes")
                kUseGL = true;
        }
    }
    // WindowsでGLパスを使う場合のみ、かつユーザーがSLINT_RENDERERを指定していない場合に限り、GLを明示。
#ifdef _WIN32
    if (!disable_gl_zerocopy && kUseGL) {
        const char* env_renderer = std::getenv("SLINT_RENDERER");
        if (!env_renderer || std::string(env_renderer).empty()) {
            _putenv_s("SLINT_BACKEND", "");
            _putenv_s("SLINT_SKIA_BACKEND", "");
            _putenv_s("SLINT_RENDERER", "gl");
        }
    } else if (disable_gl_zerocopy) {
        // 強制的にソフトウェアレンダラーへ
        _putenv_s("SLINT_BACKEND", "");
        _putenv_s("SLINT_SKIA_BACKEND", "");
        _putenv_s("SLINT_RENDERER", "software");
    }
#endif
    LOGI("[main] GL decision: SLINT_RENDERER="
         << (sr_env ? sr_env : "<unset>") << " MLNS_DISABLE_GL_ZEROCOPY="
         << (std::getenv("MLNS_DISABLE_GL_ZEROCOPY")
                 ? std::getenv("MLNS_DISABLE_GL_ZEROCOPY")
                 : "<unset>")
         << " => GL path "
         << ((!disable_gl_zerocopy && kUseGL) ? "ON" : "OFF"));
    std::shared_ptr<mlns::SlintZeroCopyGL> zero_copy_gl;
    std::shared_ptr<mlns::SlintGLMapLibre> gl_map;
#else
    const bool kUseGL = false;
    std::cout << "[main] GL feature disabled at build. Using CPU path."
              << std::endl;
#endif
    auto main_window = MapWindow::create();
#ifdef MLNS_WITH_SLINT_GL
    // Debug: print the renderer env we set
    auto env_backend = std::getenv("SLINT_BACKEND");
    auto env_renderer = std::getenv("SLINT_RENDERER");
    auto env_skia = std::getenv("SLINT_SKIA_BACKEND");
    LOGI("[main] SLINT_BACKEND="
         << (env_backend ? env_backend : "<unset>")
         << " SLINT_RENDERER=" << (env_renderer ? env_renderer : "<unset>")
         << " SLINT_SKIA_BACKEND=" << (env_skia ? env_skia : "<unset>"));
#endif
    std::shared_ptr<SlintMapLibre> slint_map_libre =
        kUseGL ? nullptr : std::make_shared<SlintMapLibre>();

    // Delay initialization until a non-zero window size is known
    auto initialized = std::make_shared<bool>(false);
    const auto size = main_window->get_window_size();
    LOGI("Initial Window Size: " << static_cast<int>(size.width) << "x"
                                 << static_cast<int>(size.height));

    // This function is ONLY for rendering in the CPU path.
    auto render_function = [=]() {
        if (kUseGL)
            return;  // GL path is driven by Slint's render notifier
        LOGI("Rendering map...");
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
            LOGI("[UI] style_changed url=" << std::string(url.data(),
                                                          url.size()));
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
        LOGI("[UI] mouse_press x=" << x << " y=" << y);
        if (kUseGL) {
            if (gl_map)
                gl_map->handle_mouse_press(x, y);
        } else {
            slint_map_libre->handle_mouse_press(x, y);
        }
    });

    main_window->global<MapAdapter>().on_mouse_release([=](float x, float y) {
        LOGI("[UI] mouse_release x=" << x << " y=" << y);
        if (kUseGL) {
            if (gl_map)
                gl_map->handle_mouse_release(x, y);
        } else {
            slint_map_libre->handle_mouse_release(x, y);
        }
    });

    main_window->global<MapAdapter>().on_mouse_move(
        [=](float x, float y, bool pressed) {
            LOGI("[UI] mouse_move x=" << x << " y=" << y << " pressed="
                                      << (pressed ? "true" : "false"));
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
            LOGI("[UI] double_click x=" << x << " y=" << y << " shift="
                                        << (shift ? "true" : "false"));
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
            LOGI("[UI] wheel x=" << x << " y=" << y << " dy=" << dy);
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
            LOGI("[UI] fly_to loc=" << loc);
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
        LOGI("Map Area Size Changed: " << w << "x" << h);
        if (w > 0 && h > 0) {
            if (!*initialized) {
                if (kUseGL) {
                    gl_map = std::make_shared<mlns::SlintGLMapLibre>();
                    gl_map->initialize(w, h);
                    // Create and attach the GL zero-copy path only after we
                    // have a non-zero window size so Slint's GL renderer can
                    // initialize.
                    LOGI("[main] Attaching GL notifier after non-zero size");
                    zero_copy_gl = std::make_shared<mlns::SlintZeroCopyGL>();
                    zero_copy_gl->set_on_ready([&](const slint::Image& img) {
                        std::cout << "[main] on_ready: borrowed image set"
                                  << std::endl;
                        main_window->global<MapAdapter>().set_map_texture(img);
                    });
                    auto& win = main_window->window();
                    zero_copy_gl->attach(
                        win, [&](const mlns::GLRenderTarget& rt) {
                            LOGI("[main] render hook: fbo="
                                 << rt.fbo << " tex=" << rt.color_tex
                                 << " size=" << rt.width << "x" << rt.height);
                            if (gl_map)
                                gl_map->render_to_texture(rt.color_tex,
                                                          rt.width, rt.height);
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

    // GLパスを使わない場合は、GLノティファイは一切アタッチしない

    // Note: close-request hook can be added here if needed.

    LOGI("[main] Entering UI event loop");
    try {
        main_window->run();
        LOGI("[main] UI event loop exited normally");
        return 0;
    } catch (const std::exception& e) {
        LOGE("[main] Unhandled exception: " << e.what());
    } catch (...) {
        LOGE("[main] Unhandled unknown exception");
    }
    return 1;
}
