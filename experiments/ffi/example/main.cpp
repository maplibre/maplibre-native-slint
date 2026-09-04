// The same thing cpp/ does, over the C API instead of direct C++ interop:
// render MapLibre offscreen and hand the pixels to Slint as an Image.
//
// The interesting part is not that it draws a map. It is that Slint's renderer
// and MapLibre now share a thread and a graphics stack across a C boundary,
// which is exactly where cpp/ needed HostGLContextGuard.
//
// Order matters here. Bringing our own EGL context up before Slint's window
// exists leaves it unusable: eglMakeCurrent on it starts failing the moment
// Slint's renderer initialises. So everything MapLibre-side starts on the
// first tick, once Slint has claimed the thread's graphics stack.

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <maplibre_native_c.h>
#include <slint.h>
#include <string>
#include <vector>

#include "app.h"
#include "backend.h"

namespace {

constexpr unsigned kWidth = 800;
constexpr unsigned kHeight = 520;

bool check(mln_status status, const char* what) {
    if (status != MLN_STATUS_OK) {
        std::printf("FAIL %s -> status %d\n", what, static_cast<int>(status));
        std::fflush(stdout);
        return false;
    }
    return true;
}

const char* event_name(uint32_t type) {
    switch (type) {
    case MLN_RUNTIME_EVENT_MAP_STYLE_LOADED:
        return "style loaded";
    case MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED:
        return "loaded";
    case MLN_RUNTIME_EVENT_MAP_LOADING_FAILED:
        return "load failed";
    case MLN_RUNTIME_EVENT_MAP_RENDER_ERROR:
        return "render error";
    default:
        return nullptr;
    }
}

struct MapSide {
    mln_runtime runtime = 0;
    mln_map map = 0;
    mln_render_session session = 0;

    bool start() {
        if (backend_setup() != 0) {
            std::printf("backend_setup failed\n");
            std::fflush(stdout);
            return false;
        }
        std::printf("backend up\n");
        std::fflush(stdout);

        mln_runtime_options runtime_options = mln_runtime_options_default();
        runtime_options.cache_path = "/tmp/mln_ffi_example_cache.sqlite";
        runtime_options.asset_path = ".";
        if (!check(mln_runtime_create(&runtime_options, &runtime),
                   "runtime_create"))
            return false;

        mln_map_options map_options = mln_map_options_default();
        map_options.width = kWidth;
        map_options.height = kHeight;
        map_options.scale_factor = 1.0;
        map_options.map_mode = MLN_MAP_MODE_CONTINUOUS;
        if (!check(mln_map_create(runtime, &map_options, &map), "map_create"))
            return false;

        if (!check(backend_attach(map, kWidth, kHeight, &session), "attach"))
            return false;
        if (!check(mln_map_set_style_url(
                       map, "https://demotiles.maplibre.org/style.json"),
                   "set_style_url")) {
            return false;
        }
        std::printf("maplibre side up\n");
        std::fflush(stdout);
        return true;
    }

    ~MapSide() {
        if (session)
            mln_render_session_destroy(session);
        if (map)
            mln_map_destroy(map);
        if (runtime)
            mln_runtime_destroy(runtime);
    }
};

}  // namespace

int main() {
    std::printf("backend: %s\n", backend_name());
    std::fflush(stdout);

    auto ui = MapWindow::create();

    MapSide side;
    bool started = false;
    bool failed = false;
    double zoom = 1.0;
    std::vector<uint8_t> pixels;
    int frames = 0;
    std::string last_event = "starting";

    auto jump = [&] {
        if (!started)
            return;
        mln_camera_options camera = mln_camera_options_default();
        camera.fields = MLN_CAMERA_OPTION_ZOOM;
        camera.zoom = zoom;
        mln_map_jump_to(side.map, &camera);
    };
    ui->on_zoom_in([&] {
        zoom += 1.0;
        jump();
    });
    ui->on_zoom_out([&] {
        zoom = zoom > 1.0 ? zoom - 1.0 : 0.0;
        jump();
    });

    // One tick is the FFI equivalent of SlintMapLibre::run_map_loop() plus
    // render_map(): drain MapLibre's queue, render, and read the frame back.
    slint::Timer ticker;
    ticker.start(
        slint::TimerMode::Repeated, std::chrono::milliseconds(16), [&] {
            if (failed)
                return;
            if (!started) {
                std::printf("starting the map side\n");
                std::fflush(stdout);
                started = side.start();
                // Slint keeps no context current between its own draws, so hand
                // the thread back the way we found it.
                backend_release_current();
                if (!started) {
                    failed = true;
                    ui->set_status(
                        slint::SharedString("could not start the map side"));
                }
                return;
            }

            mln_runtime_pump(side.runtime, 0);
            for (;;) {
                mln_runtime_event event{};
                event.size = sizeof event;
                bool has_event = false;
                if (mln_runtime_poll_event(side.runtime, &event, &has_event) !=
                        MLN_STATUS_OK ||
                    !has_event) {
                    break;
                }
                if (const char* name = event_name(event.type)) {
                    last_event = name;
                    if (event.message_size) {
                        last_event += ": ";
                        last_event.append(event.message, event.message_size);
                    }
                }
            }

            // The released artifact renders on whatever context is current, and
            // leaves the host's alone only because we put it back afterwards.
            backend_begin_render();
            bool rendered = false;
            mln_status render_status =
                mln_render_session_render_update(side.session, &rendered);

            mln_texture_image_info info = mln_texture_image_info_default();
            mln_status read_status = MLN_STATUS_INVALID_STATE;
            if (render_status == MLN_STATUS_OK && rendered) {
                read_status = mln_texture_read_premultiplied_rgba8(
                    side.session, nullptr, 0, &info);
                if (read_status == MLN_STATUS_OK) {
                    pixels.resize(info.byte_length);
                    read_status = mln_texture_read_premultiplied_rgba8(
                        side.session, pixels.data(), pixels.size(), &info);
                }
            }
            backend_end_render();

            static int diag = 0;
            if (diag < 5) {
                std::printf("tick: render=%d rendered=%d read=%d\n",
                            static_cast<int>(render_status),
                            static_cast<int>(rendered),
                            static_cast<int>(read_status));
                std::fflush(stdout);
                ++diag;
            }
            if (read_status != MLN_STATUS_OK) {
                return;
            }

            slint::SharedPixelBuffer<slint::Rgba8Pixel> buffer(info.width,
                                                               info.height);
            for (uint32_t y = 0; y < info.height; ++y) {
                std::memcpy(
                    buffer.begin() + static_cast<size_t>(y) * info.width,
                    pixels.data() + static_cast<size_t>(y) * info.stride,
                    static_cast<size_t>(info.width) * 4);
            }
            ui->set_map_image(slint::Image(buffer));

            ++frames;
            // A way to exercise the camera path without driving the UI.
            if (frames == 200 && std::getenv("MLN_FFI_AUTOZOOM")) {
                zoom = 3.0;
                jump();
                std::printf("auto zoom to %.1f\n", zoom);
                std::fflush(stdout);
            }
            if (frames <= 3) {
                backend_report_host_context("after a map frame");
            }
            ui->set_status(slint::SharedString(
                last_event + "  |  frame " + std::to_string(frames) +
                "  |  zoom " + std::to_string(zoom).substr(0, 4)));
        });

    ui->run();
    std::printf("%d frames\n", frames);
    return 0;
}
