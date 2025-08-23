#include "slint_maplibre.hpp"

#include <iostream>
#include <memory>

#include "mbgl/gfx/backend_scope.hpp"
#include "mbgl/map/bound_options.hpp"
#include "mbgl/map/camera.hpp"
#include "mbgl/style/style.hpp"
#include "mbgl/util/geo.hpp"
#include "mbgl/util/logging.hpp"
#include "mbgl/util/premultiply.hpp"

SlintMapLibre::SlintMapLibre() {
    // Note: The API key is managed through CustomFileSource
    file_source = std::make_unique<mbgl::CustomFileSource>();

    // Initialize RunLoop in the same way as mbgl-render
    run_loop = std::make_unique<mbgl::util::RunLoop>();
}

SlintMapLibre::~SlintMapLibre() = default;

void SlintMapLibre::initialize(int w, int h) {
    width = w;
    height = h;

    // Create HeadlessFrontend with the exact same parameters as mbgl-render
    frontend = std::make_unique<mbgl::HeadlessFrontend>(
        mbgl::Size{static_cast<uint32_t>(width), static_cast<uint32_t>(height)},
        1.0f);

    // Set ResourceOptions same as mbgl-render
    mbgl::ResourceOptions resourceOptions;
    resourceOptions.withCachePath("cache.sqlite").withAssetPath(".");

    // Set MapOptions same as mbgl-render
    map = std::make_unique<mbgl::Map>(
        *frontend,
        *this,  // Use this instance as MapObserver
        mbgl::MapOptions()
            .withMapMode(
                mbgl::MapMode::Static)  // Use Static mode like mbgl-render
            .withSize(frontend->getSize())
            .withPixelRatio(1.0f),
        resourceOptions);

    // Constrain zoom range
    map->setBounds(
        mbgl::BoundOptions().withMinZoom(min_zoom).withMaxZoom(max_zoom));

    // Set a more reliable background color style
    std::cout << "Setting solid background color style..." << std::endl;
    std::string simple_style = R"JSON({
        "version": 8,
        "name": "solid-background",
        "sources": {},
        "layers": [
            {
                "id": "background",
                "type": "background",
                "paint": {
                    "background-color": "rgb(255, 0, 0)",
                    "background-opacity": 1.0
                }
            }
        ]
    })JSON";
    // Switch from background color test to actual map style
    std::cout << "Loading remote MapLibre style..." << std::endl;
    map->getStyle().loadURL("https://demotiles.maplibre.org/style.json");
    // map->getStyle().loadJSON(simple_style);

    // Set initial display position (around Tokyo)
    // std::cout << "Setting initial map position..." << std::endl;
    // map->jumpTo(mbgl::CameraOptions()
    //     .withCenter(mbgl::LatLng{35.6762, 139.6503}) // Tokyo
    //    .withZoom(10.0));

    std::cout << "Map initialization completed with background color"
              << std::endl;
}

void SlintMapLibre::setRenderCallback(std::function<void()> callback) {
    m_renderCallback = std::move(callback);
}

// MapObserver implementation
void SlintMapLibre::onWillStartLoadingMap() {
    std::cout << "[MapObserver] Will start loading map" << std::endl;
    style_loaded = false;
    map_idle = false;
}

void SlintMapLibre::onDidFinishLoadingStyle() {
    std::cout << "[MapObserver] Did finish loading style" << std::endl;
    style_loaded = true;
    if (m_renderCallback) {
        // Schedule a redraw on the event loop
        slint::invoke_from_event_loop(m_renderCallback);
    }
}

void SlintMapLibre::onDidBecomeIdle() {
    std::cout << "[MapObserver] Did become idle" << std::endl;
    map_idle = true;
}

void SlintMapLibre::onDidFailLoadingMap(mbgl::MapLoadError error,
                                        const std::string& what) {
    std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
    std::cout << "[MapObserver] FAILED loading map." << std::endl;
    std::cout << "    Error type: " << static_cast<int>(error) << std::endl;
    std::cout << "    What: " << what << std::endl;
    std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
}

void SlintMapLibre::setStyleUrl(const std::string& url) {
    if (map) {
        map->getStyle().loadURL(url);
    }
}

slint::Image SlintMapLibre::render_map() {
    std::cout << "render_map() called" << std::endl;

    if (!map || !frontend) {
        std::cout << "ERROR: map or frontend is null" << std::endl;
        return {};
    }

    // Wait for style to finish loading
    if (!style_loaded.load()) {
        std::cout << "Style not loaded yet, returning empty image" << std::endl;
        return {};  // Return an empty image
    }

    std::cout << "Style loaded, proceeding with rendering..." << std::endl;

    // Use the exact same rendering method as mbgl-render
    std::cout << "Using frontend.render(map) like mbgl-render..." << std::endl;
    auto render_result = frontend->render(*map);

    std::cout << "Got render result, extracting image..." << std::endl;
    mbgl::PremultipliedImage rendered_image = std::move(render_result.image);

    std::cout << "Image size: " << rendered_image.size.width << "x"
              << rendered_image.size.height << std::endl;
    std::cout << "Image data pointer: "
              << (rendered_image.data.get() ? "valid" : "null") << std::endl;

    if (rendered_image.data == nullptr || rendered_image.size.isEmpty()) {
        std::cout << "ERROR: renderStill() returned empty data" << std::endl;
        return {};
    }

    // As advised: Convert PremultipliedImage to non-premultiplied
    std::cout << "Converting from premultiplied to unpremultiplied..."
              << std::endl;
    mbgl::UnassociatedImage unpremult_image =
        mbgl::util::unpremultiply(std::move(rendered_image));

    std::cout << "Creating Slint pixel buffer..." << std::endl;
    auto pixel_buffer = slint::SharedPixelBuffer<slint::Rgba8Pixel>(
        unpremult_image.size.width, unpremult_image.size.height);

    // Copy the non-premultiplied image to the Slint buffer
    std::cout << "Copying unpremultiplied pixel data..." << std::endl;
    memcpy(pixel_buffer.begin(), unpremult_image.data.get(),
           unpremult_image.size.width * unpremult_image.size.height *
               sizeof(slint::Rgba8Pixel));

    // Debug: Sample and inspect pixel data content
    const uint8_t* raw_data = unpremult_image.data.get();
    std::cout << "Pixel samples (RGBA): ";
    for (int i = 0;
         i < 20 && i < unpremult_image.size.width * unpremult_image.size.height;
         i += 5) {
        int offset = i * 4;
        std::cout << "(" << (int)raw_data[offset] << ","
                  << (int)raw_data[offset + 1] << ","
                  << (int)raw_data[offset + 2] << ","
                  << (int)raw_data[offset + 3] << ") ";
    }
    std::cout << std::endl;

    // Count the number of non-transparent (alpha value is not 0) pixels
    int non_transparent_count = 0;
    for (int i = 0;
         i < unpremult_image.size.width * unpremult_image.size.height; i++) {
        if (raw_data[i * 4 + 3] > 0) {  // Check the alpha channel
            non_transparent_count++;
        }
    }
    std::cout << "Non-transparent pixels: " << non_transparent_count << " / "
              << (unpremult_image.size.width * unpremult_image.size.height)
              << std::endl;

    std::cout << "Image created successfully" << std::endl;
    return slint::Image(pixel_buffer);
}

void SlintMapLibre::resize(int w, int h) {
    width = w;
    height = h;

    if (frontend && map) {
        frontend->setSize(
            {static_cast<uint32_t>(width), static_cast<uint32_t>(height)});
        map->setSize(
            {static_cast<uint32_t>(width), static_cast<uint32_t>(height)});
    }
}

void SlintMapLibre::handle_mouse_press(float x, float y) {
    last_pos = {x, y};
    // Trigger a redraw after interaction starts for responsiveness
    if (m_renderCallback) {
        slint::invoke_from_event_loop(m_renderCallback);
    }
}

void SlintMapLibre::handle_mouse_release(float x, float y) {
    // No action needed for release
}

void SlintMapLibre::handle_mouse_move(float x, float y, bool pressed) {
    if (pressed) {
        mbgl::Point<double> current_pos = {x, y};
        // Move the map along with the pointer movement (dragging behavior)
        map->moveBy(current_pos - last_pos);
        last_pos = current_pos;
        if (m_renderCallback) {
            slint::invoke_from_event_loop(m_renderCallback);
        }
    }
}

void SlintMapLibre::handle_double_click(float x, float y, bool shift) {
    if (!map)
        return;
    // Center the map on the clicked location and zoom by one level (+/- with
    // Shift)
    const mbgl::LatLng ll = map->latLngForPixel(mbgl::ScreenCoordinate{x, y});
    const auto cam = map->getCameraOptions();
    const double currentZoom = cam.zoom.value_or(0.0);
    const double delta = shift ? -1.0 : 1.0;
    const double targetZoom =
        std::min(max_zoom, std::max(min_zoom, currentZoom + delta));

    mbgl::CameraOptions next;
    next.withCenter(std::optional<mbgl::LatLng>(ll));
    next.withZoom(std::optional<double>(targetZoom));
    map->jumpTo(next);
    if (m_renderCallback) {
        slint::invoke_from_event_loop(m_renderCallback);
    }
}

void SlintMapLibre::handle_wheel_zoom(float x, float y, float dy) {
    if (!map)
        return;
    // Lower sensitivity: dy < 0 => zoom in, dy > 0 => zoom out
    constexpr double step = 1.2;  // smoother than 2.0
    double scale = (dy < 0.0) ? step : (1.0 / step);
    map->scaleBy(scale, mbgl::ScreenCoordinate{x, y});
    if (m_renderCallback) {
        slint::invoke_from_event_loop(m_renderCallback);
    }
}

void SlintMapLibre::run_map_loop() {
    if (run_loop) {
        run_loop->runOnce();
    }
}
