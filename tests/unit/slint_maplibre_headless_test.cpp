#include "slint_maplibre_headless.hpp"

#include <gtest/gtest.h>
#include <thread>

class SlintMapLibreTest : public ::testing::Test {
protected:
    void SetUp() override {
        slint_map = std::make_unique<SlintMapLibre>();
    }

    void TearDown() override {
        slint_map.reset();
    }

    std::unique_ptr<SlintMapLibre> slint_map;
};

TEST_F(SlintMapLibreTest, ConstructorDestructor) {
    // Test basic construction and destruction
    auto map = std::make_unique<SlintMapLibre>();
    EXPECT_NE(map, nullptr);

    map.reset();
}

TEST_F(SlintMapLibreTest, Initialize) {
    // Test map initialization with valid dimensions
    EXPECT_NO_THROW(slint_map->initialize(800, 600));
}

TEST_F(SlintMapLibreTest, InitializeMultipleSizes) {
    // Test initialization with different sizes
    EXPECT_NO_THROW(slint_map->initialize(1024, 768));

    auto map2 = std::make_unique<SlintMapLibre>();
    EXPECT_NO_THROW(map2->initialize(640, 480));

    auto map3 = std::make_unique<SlintMapLibre>();
    EXPECT_NO_THROW(map3->initialize(1920, 1080));
}

TEST_F(SlintMapLibreTest, InitializeSmallSize) {
    // Test initialization with minimum size
    EXPECT_NO_THROW(slint_map->initialize(1, 1));
}

TEST_F(SlintMapLibreTest, InitializeLargeSize) {
    // Test initialization with large dimensions
    EXPECT_NO_THROW(slint_map->initialize(4096, 4096));
}

TEST_F(SlintMapLibreTest, Resize) {
    // Test resizing the map
    slint_map->initialize(800, 600);
    EXPECT_NO_THROW(slint_map->resize(1024, 768));
}

TEST_F(SlintMapLibreTest, ResizeMultipleTimes) {
    // Test multiple resizes
    slint_map->initialize(800, 600);
    EXPECT_NO_THROW(slint_map->resize(1024, 768));
    EXPECT_NO_THROW(slint_map->resize(640, 480));
    EXPECT_NO_THROW(slint_map->resize(1920, 1080));
}

TEST_F(SlintMapLibreTest, ResizeToSmall) {
    // Test resizing to small dimensions
    slint_map->initialize(800, 600);
    EXPECT_NO_THROW(slint_map->resize(100, 100));
}

TEST_F(SlintMapLibreTest, MousePress) {
    // Test mouse press event
    slint_map->initialize(800, 600);
    EXPECT_NO_THROW(slint_map->handle_mouse_press(100.0f, 100.0f));
}

TEST_F(SlintMapLibreTest, MouseRelease) {
    // Test mouse release event
    slint_map->initialize(800, 600);
    EXPECT_NO_THROW(slint_map->handle_mouse_release(100.0f, 100.0f));
}

TEST_F(SlintMapLibreTest, MouseMove) {
    // Test mouse move event
    slint_map->initialize(800, 600);
    EXPECT_NO_THROW(slint_map->handle_mouse_move(150.0f, 150.0f, false));
}

TEST_F(SlintMapLibreTest, MouseDrag) {
    // Test mouse drag (move with pressed state)
    slint_map->initialize(800, 600);
    slint_map->handle_mouse_press(100.0f, 100.0f);
    EXPECT_NO_THROW(slint_map->handle_mouse_move(150.0f, 150.0f, true));
    slint_map->handle_mouse_release(150.0f, 150.0f);
}

TEST_F(SlintMapLibreTest, DoubleClick) {
    // Test double-click event
    slint_map->initialize(800, 600);
    EXPECT_NO_THROW(slint_map->handle_double_click(200.0f, 200.0f, false));
}

TEST_F(SlintMapLibreTest, DoubleClickWithShift) {
    // Test double-click with shift modifier
    slint_map->initialize(800, 600);
    EXPECT_NO_THROW(slint_map->handle_double_click(200.0f, 200.0f, true));
}

TEST_F(SlintMapLibreTest, WheelZoomIn) {
    // Test wheel zoom in (positive dy)
    slint_map->initialize(800, 600);
    EXPECT_NO_THROW(slint_map->handle_wheel_zoom(400.0f, 300.0f, 1.0f));
}

TEST_F(SlintMapLibreTest, WheelZoomOut) {
    // Test wheel zoom out (negative dy)
    slint_map->initialize(800, 600);
    EXPECT_NO_THROW(slint_map->handle_wheel_zoom(400.0f, 300.0f, -1.0f));
}

TEST_F(SlintMapLibreTest, SetPitch) {
    // Test setting pitch
    slint_map->initialize(800, 600);
    EXPECT_NO_THROW(slint_map->set_pitch(0));
    EXPECT_NO_THROW(slint_map->set_pitch(30));
    EXPECT_NO_THROW(slint_map->set_pitch(60));
}

TEST_F(SlintMapLibreTest, SetBearing) {
    // Test setting bearing
    slint_map->initialize(800, 600);
    EXPECT_NO_THROW(slint_map->set_bearing(0.0f));
    EXPECT_NO_THROW(slint_map->set_bearing(90.0f));
    EXPECT_NO_THROW(slint_map->set_bearing(180.0f));
    EXPECT_NO_THROW(slint_map->set_bearing(270.0f));
}

TEST_F(SlintMapLibreTest, SetBearingNegative) {
    // Test setting negative bearing
    slint_map->initialize(800, 600);
    EXPECT_NO_THROW(slint_map->set_bearing(-45.0f));
    EXPECT_NO_THROW(slint_map->set_bearing(-90.0f));
}

TEST_F(SlintMapLibreTest, SetStyleUrl) {
    // Test setting style URL
    slint_map->initialize(800, 600);
    EXPECT_NO_THROW(slint_map->setStyleUrl(
        "https://demotiles.maplibre.org/style.json"));
}

TEST_F(SlintMapLibreTest, FlyToLocation) {
    // Test fly-to animation with location name
    slint_map->initialize(800, 600);
    EXPECT_NO_THROW(slint_map->fly_to("tokyo"));
    EXPECT_NO_THROW(slint_map->fly_to("london"));
    EXPECT_NO_THROW(slint_map->fly_to("new york"));
}

TEST_F(SlintMapLibreTest, FlyToUnknownLocation) {
    // Test fly-to with unknown location (should handle gracefully)
    slint_map->initialize(800, 600);
    EXPECT_NO_THROW(slint_map->fly_to("nonexistent-place-xyz"));
}

TEST_F(SlintMapLibreTest, RunMapLoop) {
    // Test running the map loop
    slint_map->initialize(800, 600);
    // Run a few iterations
    for (int i = 0; i < 5; ++i) {
        EXPECT_NO_THROW(slint_map->run_map_loop());
    }
}

TEST_F(SlintMapLibreTest, TickAnimation) {
    // Test animation tick
    slint_map->initialize(800, 600);
    EXPECT_NO_THROW(slint_map->tick_animation());
}

TEST_F(SlintMapLibreTest, RepaintRequest) {
    // Test repaint request mechanism
    slint_map->initialize(800, 600);
    slint_map->request_repaint();
    bool repaint_needed = slint_map->take_repaint_request();
    EXPECT_TRUE(repaint_needed);

    // Second call should return false
    repaint_needed = slint_map->take_repaint_request();
    EXPECT_FALSE(repaint_needed);
}

TEST_F(SlintMapLibreTest, ForcedRepaintArm) {
    // Test forced repaint mechanism
    slint_map->initialize(800, 600);
    slint_map->arm_forced_repaint_ms(100);
    bool forced = slint_map->consume_forced_repaint();
    EXPECT_TRUE(forced);
}

TEST_F(SlintMapLibreTest, RenderMap) {
    // Test rendering the map
    slint_map->initialize(800, 600);
    slint::Image img;
    EXPECT_NO_THROW(img = slint_map->render_map());
}

TEST_F(SlintMapLibreTest, RenderMapMultipleTimes) {
    // Test multiple consecutive renders
    slint_map->initialize(800, 600);
    for (int i = 0; i < 3; ++i) {
        slint::Image img;
        EXPECT_NO_THROW(img = slint_map->render_map());
    }
}

TEST_F(SlintMapLibreTest, SetRenderCallback) {
    // Test setting a render callback
    slint_map->initialize(800, 600);
    bool callback_called = false;
    slint_map->setRenderCallback([&callback_called]() {
        callback_called = true;
    });

    // Trigger a repaint to invoke the callback
    slint_map->request_repaint();
}

TEST_F(SlintMapLibreTest, ComplexInteractionSequence) {
    // Test a complex sequence of interactions
    slint_map->initialize(800, 600);

    // Pan the map
    slint_map->handle_mouse_press(400.0f, 300.0f);
    slint_map->handle_mouse_move(350.0f, 250.0f, true);
    slint_map->handle_mouse_release(350.0f, 250.0f);

    // Zoom in
    slint_map->handle_wheel_zoom(400.0f, 300.0f, 2.0f);

    // Change pitch and bearing
    slint_map->set_pitch(45);
    slint_map->set_bearing(90.0f);

    // Render
    slint::Image img;
    EXPECT_NO_THROW(img = slint_map->render_map());
}

TEST_F(SlintMapLibreTest, BoundaryCoordinates) {
    // Test with boundary coordinates
    slint_map->initialize(800, 600);
    EXPECT_NO_THROW(slint_map->handle_mouse_press(0.0f, 0.0f));
    EXPECT_NO_THROW(slint_map->handle_mouse_press(800.0f, 600.0f));
    EXPECT_NO_THROW(slint_map->handle_mouse_press(-10.0f, -10.0f));
    EXPECT_NO_THROW(slint_map->handle_mouse_press(1000.0f, 1000.0f));
}

TEST_F(SlintMapLibreTest, ExtremeZoomValues) {
    // Test with extreme wheel zoom values
    slint_map->initialize(800, 600);
    EXPECT_NO_THROW(slint_map->handle_wheel_zoom(400.0f, 300.0f, 100.0f));
    EXPECT_NO_THROW(slint_map->handle_wheel_zoom(400.0f, 300.0f, -100.0f));
}

TEST_F(SlintMapLibreTest, ExtremePitchValues) {
    // Test with extreme pitch values
    slint_map->initialize(800, 600);
    EXPECT_NO_THROW(slint_map->set_pitch(-10));  // Should be clamped
    EXPECT_NO_THROW(slint_map->set_pitch(90));   // Should be clamped
    EXPECT_NO_THROW(slint_map->set_pitch(100));  // Should be clamped
}

TEST_F(SlintMapLibreTest, ExtremeBearingValues) {
    // Test with extreme bearing values
    slint_map->initialize(800, 600);
    EXPECT_NO_THROW(slint_map->set_bearing(360.0f));   // Full rotation
    EXPECT_NO_THROW(slint_map->set_bearing(720.0f));   // Double rotation
    EXPECT_NO_THROW(slint_map->set_bearing(-360.0f));  // Negative full rotation
}
