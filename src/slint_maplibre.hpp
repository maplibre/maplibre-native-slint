#pragma once

#include <atomic>
#include <functional>
#include <mbgl/gfx/headless_frontend.hpp>
#include <mbgl/map/map.hpp>
#include <mbgl/map/map_observer.hpp>
#include <mbgl/map/map_options.hpp>
#include <mbgl/storage/resource_options.hpp>
#include <mbgl/util/run_loop.hpp>
#include <memory>
#include <slint.h>
#include <string>

#include "../platform/custom_file_source.hpp"

class SlintMapLibre : public mbgl::MapObserver {
public:
    SlintMapLibre();
    ~SlintMapLibre();

    void initialize(int width, int height);
    void setRenderCallback(std::function<void()> callback);
    slint::Image render_map();
    void resize(int width, int height);
    void handle_mouse_press(float x, float y);
    void handle_mouse_release(float x, float y);
    void handle_mouse_move(float x, float y, bool pressed);
    void handle_double_click(float x, float y, bool shift);
    void handle_wheel_zoom(float x, float y, float dy);
    void setStyleUrl(const std::string& url);
    void fly_to(const std::string& location);

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
    std::function<void()> m_renderCallback;

    int width = 0;
    int height = 0;

    mbgl::Point<double> last_pos;
    double min_zoom = 0.0;
    double max_zoom = 22.0;

    // Style loading state management
    std::atomic<bool> style_loaded{false};
    std::atomic<bool> map_idle{false};
};
