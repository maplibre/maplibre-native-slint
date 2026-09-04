// Smallest end-to-end check that maplibre-native-ffi can stand in for the C++
// backend maplibre-native-slint uses today: bring up a host graphics context
// the way Slint's renderer would, hand it to an owned-texture render session,
// load a style, render, and read the pixels back to host memory.
//
// The host-side setup lives in backend_egl.c / backend_vulkan.c; everything
// below is the same for both.
#include <maplibre_native_c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "backend.h"

#define CHECK(expr)                                           \
    do {                                                      \
        mln_status _s = (expr);                               \
        if (_s != MLN_STATUS_OK) {                            \
            printf("FAIL %s -> status %d\n", #expr, (int)_s); \
            return 1;                                         \
        }                                                     \
    } while (0)

static const char* event_name(uint32_t type) {
    switch (type) {
    case MLN_RUNTIME_EVENT_MAP_STYLE_LOADED:
        return "STYLE_LOADED";
    case MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED:
        return "LOADING_FINISHED";
    case MLN_RUNTIME_EVENT_MAP_LOADING_FAILED:
        return "LOADING_FAILED";
    case MLN_RUNTIME_EVENT_MAP_IDLE:
        return "IDLE";
    case MLN_RUNTIME_EVENT_MAP_RENDER_ERROR:
        return "RENDER_ERROR";
    default:
        return NULL;
    }
}

// A rendered map has many colours. A background-only frame has one or two, and
// it is opaque, so counting opaque pixels is not a test of anything.
static size_t count_distinct(const uint8_t* pixels,
                             const mln_texture_image_info* info) {
    static unsigned char seen[1 << 24];
    memset(seen, 0, sizeof seen);
    size_t distinct = 0;
    for (uint32_t y = 0; y < info->height; ++y) {
        const uint8_t* row = pixels + (size_t)y * info->stride;
        for (uint32_t x = 0; x < info->width; ++x) {
            uint32_t key = ((uint32_t)row[4 * x] << 16) |
                           ((uint32_t)row[4 * x + 1] << 8) | row[4 * x + 2];
            if (!seen[key]) {
                seen[key] = 1;
                ++distinct;
            }
        }
    }
    return distinct;
}

int main(void) {
    const unsigned width = 512, height = 512;

    printf("backend: %s\n", backend_name());
    if (backend_setup() != 0) {
        return 1;
    }
    backend_report_host_context("before");

    mln_runtime_options runtime_options = mln_runtime_options_default();
    runtime_options.cache_path = "/tmp/mln_ffi_smoke_cache.sqlite";
    runtime_options.asset_path = ".";
    mln_runtime runtime = 0;
    CHECK(mln_runtime_create(&runtime_options, &runtime));

    mln_map_options map_options = mln_map_options_default();
    map_options.width = width;
    map_options.height = height;
    map_options.scale_factor = 1.0;
    map_options.map_mode = MLN_MAP_MODE_CONTINUOUS;
    mln_map map = 0;
    CHECK(mln_map_create(runtime, &map_options, &map));

    mln_render_session session = 0;
    CHECK(backend_attach(map, width, height, &session));
    printf("render session attached\n");

    CHECK(mln_map_set_style_url(map,
                                "https://demotiles.maplibre.org/style.json"));

    // MAP_IDLE never arrives in MLN_MAP_MODE_CONTINUOUS; LOADING_FINISHED is
    // the signal that the style and its first tiles are in.
    int style_loaded = 0, loading_finished = 0, tile_events = 0;
    for (int tick = 0; tick < 1200 && !(style_loaded && loading_finished);
         ++tick) {
        mln_runtime_pump(runtime, 25);
        for (;;) {
            mln_runtime_event event;
            memset(&event, 0, sizeof event);
            event.size = sizeof event;
            bool has_event = false;
            if (mln_runtime_poll_event(runtime, &event, &has_event) !=
                    MLN_STATUS_OK ||
                !has_event) {
                break;
            }
            const char* name = event_name(event.type);
            if (name) {
                printf("  event: %s%s%.*s\n", name,
                       event.message_size ? " | " : "", (int)event.message_size,
                       event.message ? event.message : "");
            }
            if (event.type == MLN_RUNTIME_EVENT_MAP_STYLE_LOADED)
                style_loaded = 1;
            if (event.type == MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED)
                loading_finished = 1;
            if (event.type == MLN_RUNTIME_EVENT_MAP_TILE_ACTION)
                ++tile_events;
            if (event.type == MLN_RUNTIME_EVENT_MAP_LOADING_FAILED) {
                printf("FAIL style load\n");
                return 1;
            }
        }
        bool rendered = false;
        mln_render_session_render_update(session, &rendered);
    }
    printf("style_loaded=%d loading_finished=%d tile_events=%d\n", style_loaded,
           loading_finished, tile_events);

    bool rendered = false;
    CHECK(mln_render_session_render_update(session, &rendered));
    printf("render_update rendered=%d\n", (int)rendered);

    mln_texture_image_info info = mln_texture_image_info_default();
    CHECK(mln_texture_read_premultiplied_rgba8(session, NULL, 0, &info));
    printf("readback probe: %ux%u stride=%u bytes=%zu\n", info.width,
           info.height, info.stride, info.byte_length);

    uint8_t* pixels = malloc(info.byte_length);
    CHECK(mln_texture_read_premultiplied_rgba8(session, pixels,
                                               info.byte_length, &info));

    size_t opaque = 0;
    for (size_t i = 3; i < info.byte_length; i += 4) {
        if (pixels[i] != 0)
            ++opaque;
    }
    size_t distinct = count_distinct(pixels, &info);
    printf("non-transparent pixels: %zu / %u\n", opaque,
           info.width * info.height);
    printf("distinct colours: %zu\n", distinct);
    printf("samples RGBA: ");
    for (int k = 0; k < 3; ++k) {
        size_t off = ((size_t)info.stride * (info.height / 4 * (k + 1))) +
                     (size_t)info.width * 2;
        printf("(%u,%u,%u,%u) ", pixels[off], pixels[off + 1], pixels[off + 2],
               pixels[off + 3]);
    }
    printf("\n");

    char path[256];
    snprintf(path, sizeof path, "/tmp/mln_ffi_smoke_%s.ppm", backend_name());
    FILE* out = fopen(path, "wb");
    fprintf(out, "P6\n%u %u\n255\n", info.width, info.height);
    for (uint32_t y = 0; y < info.height; ++y) {
        const uint8_t* row = pixels + (size_t)y * info.stride;
        for (uint32_t x = 0; x < info.width; ++x)
            fwrite(row + 4 * x, 1, 3, out);
    }
    fclose(out);
    printf("wrote %s\n", path);

    free(pixels);
    mln_render_session_destroy(session);
    mln_map_destroy(map);
    mln_runtime_destroy(runtime);
    backend_report_host_context("after teardown");

    const int is_map = distinct >= 8 && opaque > 0;
    printf("RESULT: %s\n", is_map ? "rendered a map" : "flat fill, not a map");
    return is_map ? 0 : 1;
}
