#include "slint_maplibre.hpp"

#include "mbgl/gfx/backend_scope.hpp"
#include "mbgl/style/style.hpp"
#include "mbgl/util/logging.hpp"
#include "mbgl/util/geo.hpp"
#include "mbgl/util/premultiply.hpp"
#include "mbgl/map/camera.hpp"

#include <iostream>
#include <memory>

SlintMapLibre::SlintMapLibre() {
    // Note: The API key is managed through CustomFileSource
    file_source = std::make_unique<mbgl::CustomFileSource>();
    
    // mbgl-renderと同様にRunLoopを初期化
    run_loop = std::make_unique<mbgl::util::RunLoop>();
}

SlintMapLibre::~SlintMapLibre() = default;

void SlintMapLibre::initialize(int w, int h) {
    width = w;
    height = h;

    // mbgl-renderと全く同じパラメータでHeadlessFrontend作成
    frontend = std::make_unique<mbgl::HeadlessFrontend>(
        mbgl::Size{static_cast<uint32_t>(width), static_cast<uint32_t>(height)},
        1.0f);

    // mbgl-renderと同じResourceOptions設定
    mbgl::ResourceOptions resourceOptions;
    resourceOptions.withCachePath("cache.sqlite").withAssetPath(".");
    
    // mbgl-renderと同じMapOptions設定
    map = std::make_unique<mbgl::Map>(
        *frontend,
        *this, // Use this instance as MapObserver
        mbgl::MapOptions()
            .withMapMode(mbgl::MapMode::Static) // Use Static mode like mbgl-render
            .withSize(frontend->getSize())
            .withPixelRatio(1.0f),
        resourceOptions);

    // より確実な背景色スタイルを設定
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
    // 背景色テストから実際の地図スタイルに切り替え
    std::cout << "Loading remote MapLibre style..." << std::endl;
    map->getStyle().loadURL("https://demotiles.maplibre.org/style.json");
    //map->getStyle().loadJSON(simple_style);
    
    // 初期表示位置を設定（東京周辺）
    std::cout << "Setting initial map position..." << std::endl;
    map->jumpTo(mbgl::CameraOptions()
        .withCenter(mbgl::LatLng{35.6762, 139.6503}) // 東京
        .withZoom(10.0));
    
    std::cout << "Map initialization completed with background color" << std::endl;
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
}

void SlintMapLibre::onDidBecomeIdle() {
    std::cout << "[MapObserver] Did become idle" << std::endl;
    map_idle = true;
}

void SlintMapLibre::onDidFailLoadingMap(mbgl::MapLoadError error, const std::string& what) {
    std::cout << "[MapObserver] Failed loading map: " << what << std::endl;
}

slint::Image SlintMapLibre::render_map() {
    std::cout << "render_map() called" << std::endl;
    
    if (!map || !frontend) {
        std::cout << "ERROR: map or frontend is null" << std::endl;
        return {};
    }

    // スタイル読み込み完了を待つ
    if (!style_loaded.load()) {
        std::cout << "Style not loaded yet, returning empty image" << std::endl;
        return {};  // 空の画像を返す
    }

    std::cout << "Style loaded, proceeding with rendering..." << std::endl;

    // mbgl-renderと全く同じレンダリング方法を使用
    std::cout << "Using frontend.render(map) like mbgl-render..." << std::endl;
    auto render_result = frontend->render(*map);
    
    std::cout << "Got render result, extracting image..." << std::endl;
    mbgl::PremultipliedImage rendered_image = std::move(render_result.image);
    
    std::cout << "Image size: " << rendered_image.size.width << "x" << rendered_image.size.height << std::endl;
    std::cout << "Image data pointer: " << (rendered_image.data.get() ? "valid" : "null") << std::endl;

    if (rendered_image.data == nullptr || rendered_image.size.isEmpty()) {
        std::cout << "ERROR: renderStill() returned empty data" << std::endl;
        return {};
    }

    // 助言通り：PremultipliedImage を非プリ乗算に変換
    std::cout << "Converting from premultiplied to unpremultiplied..." << std::endl;
    mbgl::UnassociatedImage unpremult_image = mbgl::util::unpremultiply(std::move(rendered_image));

    std::cout << "Creating Slint pixel buffer..." << std::endl;
    auto pixel_buffer = slint::SharedPixelBuffer<slint::Rgba8Pixel>(
        unpremult_image.size.width, unpremult_image.size.height);

    // 非プリ乗算画像をSlintバッファにコピー
    std::cout << "Copying unpremultiplied pixel data..." << std::endl;
    memcpy(pixel_buffer.begin(), unpremult_image.data.get(), 
           unpremult_image.size.width * unpremult_image.size.height * sizeof(slint::Rgba8Pixel));

    // デバッグ: ピクセルデータの内容をサンプル調査
    const uint8_t* raw_data = unpremult_image.data.get();
    std::cout << "Pixel samples (RGBA): ";
    for (int i = 0; i < 20 && i < unpremult_image.size.width * unpremult_image.size.height; i += 5) {
        int offset = i * 4;
        std::cout << "(" << (int)raw_data[offset] << "," << (int)raw_data[offset+1] 
                  << "," << (int)raw_data[offset+2] << "," << (int)raw_data[offset+3] << ") ";
    }
    std::cout << std::endl;

    // 透明でない（アルファ値が0でない）ピクセルの数をカウント
    int non_transparent_count = 0;
    for (int i = 0; i < unpremult_image.size.width * unpremult_image.size.height; i++) {
        if (raw_data[i * 4 + 3] > 0) {  // アルファチャンネルをチェック
            non_transparent_count++;
        }
    }
    std::cout << "Non-transparent pixels: " << non_transparent_count << " / " 
              << (unpremult_image.size.width * unpremult_image.size.height) << std::endl;

    std::cout << "Image created successfully" << std::endl;
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