#pragma once

#include <atomic>
#include <mbgl/gfx/headless_frontend.hpp>
#include <mbgl/map/map.hpp>
#include <mbgl/map/map_observer.hpp>
#include <mbgl/map/map_options.hpp>
#include <mbgl/storage/resource_options.hpp>
#include <mbgl/util/run_loop.hpp>
#include <memory>
#include <slint.h>

#include "../platform/custom_file_source.hpp"

class SlintMapLibre : public mbgl::MapObserver {
public:
    SlintMapLibre();
    ~SlintMapLibre();

    void initialize(int width, int height);
    slint::Image render_map();
    void resize(int width, int height);
    void handle_mouse_press(float x, float y);
    void handle_mouse_release(float x, float y);
    void handle_mouse_move(float x, float y, bool pressed);

    // Manually drive the map's run loop
    void run_map_loop();

    // MapObserver implementation
    void onWillStartLoadingMap() override;
    void onDidFinishLoadingStyle() override;
    void onDidBecomeIdle() override;
    void onDidFailLoadingMap(mbgl::MapLoadError error,
                             const std::string& what) override;

private:
    std::unique_ptr<mbgl::Map> map;
    std::unique_ptr<mbgl::HeadlessFrontend> frontend;
    std::unique_ptr<mbgl::CustomFileSource> file_source;
    std::unique_ptr<mbgl::util::RunLoop> run_loop;

    int width = 0;
    int height = 0;

    mbgl::Point<double> last_pos;

    // スタイル読み込み状態管理
    std::atomic<bool> style_loaded{false};
    std::atomic<bool> map_idle{false};
};