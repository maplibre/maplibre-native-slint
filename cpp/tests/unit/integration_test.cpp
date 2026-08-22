#include <gtest/gtest.h>
#include <memory>

#include "slint_maplibre_headless.hpp"

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        slint_map = std::make_unique<SlintMapLibre>();
    }

    void TearDown() override {
        slint_map.reset();
    }

    std::unique_ptr<SlintMapLibre> slint_map;
};

TEST_F(IntegrationTest, MapInitialization) {
    EXPECT_NE(slint_map, nullptr);
    EXPECT_NO_THROW(slint_map->initialize(800, 600));
}

TEST_F(IntegrationTest, MapWithStyleUrl) {
    slint_map->initialize(800, 600);
    EXPECT_NO_THROW(
        slint_map->setStyleUrl("https://demotiles.maplibre.org/style.json"));
}

TEST_F(IntegrationTest, MapInteractions) {
    slint_map->initialize(800, 600);

    slint_map->handle_mouse_press(100.0f, 100.0f);
    slint_map->handle_mouse_move(150.0f, 150.0f, true);
    slint_map->handle_mouse_release(150.0f, 150.0f);
}

TEST_F(IntegrationTest, MultipleComponentsLifecycle) {
    // Test creating and destroying multiple maps
    auto map1 = std::make_unique<SlintMapLibre>();
    map1->initialize(640, 480);

    auto map2 = std::make_unique<SlintMapLibre>();
    map2->initialize(1024, 768);

    // Clean up in order
    map1.reset();
    map2.reset();
}

TEST_F(IntegrationTest, MapResize) {
    slint_map->initialize(800, 600);

    EXPECT_NO_THROW(slint_map->resize(1024, 768));
    EXPECT_NO_THROW(slint_map->resize(640, 480));
}

TEST_F(IntegrationTest, MapRendering) {
    slint_map->initialize(800, 600);

    for (int i = 0; i < 3; ++i) {
        slint::Image img;
        EXPECT_NO_THROW(img = slint_map->render_map());
    }
}

TEST_F(IntegrationTest, MapCameraAndRendering) {
    slint_map->initialize(800, 600);

    slint_map->set_pitch(45);
    slint_map->set_bearing(90.0f);
    slint_map->handle_wheel_zoom(400.0f, 300.0f, 1.0f);

    slint::Image img;
    EXPECT_NO_THROW(img = slint_map->render_map());
}

TEST_F(IntegrationTest, MapAnimation) {
    slint_map->initialize(800, 600);

    EXPECT_NO_THROW(slint_map->fly_to("tokyo"));

    for (int i = 0; i < 5; ++i) {
        EXPECT_NO_THROW(slint_map->tick_animation());
    }
}

TEST_F(IntegrationTest, StressTest) {
    slint_map->initialize(800, 600);

    for (int i = 0; i < 10; ++i) {
        slint_map->handle_mouse_press(100.0f + i * 10, 100.0f + i * 10);
        slint_map->handle_mouse_release(100.0f + i * 10, 100.0f + i * 10);
        slint_map->handle_wheel_zoom(400.0f, 300.0f, 0.5f);
    }
}
