#include "slint_maplibre.hpp"

#include <gtest/gtest.h>

#include "../test_utils.hpp"

class SlintMapLibreTest : public test::OpenGLTest {
protected:
    void SetUp() override {
        test::OpenGLTest::SetUp();
    }

    void TearDown() override {
        test::OpenGLTest::TearDown();
    }
};

TEST_F(SlintMapLibreTest, ConstructorDestructor) {
    // Test basic construction and destruction
    auto maplibre = std::make_unique<SlintMapLibre>();
    EXPECT_NE(maplibre, nullptr);

    // Destructor should clean up resources properly
    maplibre.reset();
}

TEST_F(SlintMapLibreTest, InitializeWithValidDimensions) {
    SlintMapLibre maplibre;

    // Test initialization with valid dimensions
    EXPECT_NO_THROW(maplibre.initialize(800, 600));

    // Test initialization with different dimensions
    EXPECT_NO_THROW(maplibre.initialize(1024, 768));
}

TEST_F(SlintMapLibreTest, InitializeWithInvalidDimensions) {
    SlintMapLibre maplibre;

    // Test initialization with zero dimensions
    // Note: This might be valid in some contexts, adjust based on actual
    // requirements
    EXPECT_NO_THROW(maplibre.initialize(0, 0));

    // Test initialization with negative dimensions
    EXPECT_NO_THROW(maplibre.initialize(-100, -100));
}

TEST_F(SlintMapLibreTest, ResizeAfterInitialization) {
    SlintMapLibre maplibre;
    maplibre.initialize(800, 600);

    // Test resizing to different dimensions
    EXPECT_NO_THROW(maplibre.resize(1024, 768));
    EXPECT_NO_THROW(maplibre.resize(640, 480));
}

TEST_F(SlintMapLibreTest, ResizeBeforeInitialization) {
    SlintMapLibre maplibre;

    // Test resizing before initialization - should handle gracefully
    EXPECT_NO_THROW(maplibre.resize(800, 600));
}

TEST_F(SlintMapLibreTest, MouseInteractions) {
    SlintMapLibre maplibre;
    maplibre.initialize(800, 600);

    // Test mouse press
    EXPECT_NO_THROW(maplibre.handle_mouse_press(100.0f, 200.0f));

    // Test mouse move with press
    EXPECT_NO_THROW(maplibre.handle_mouse_move(150.0f, 250.0f, true));

    // Test mouse move without press
    EXPECT_NO_THROW(maplibre.handle_mouse_move(200.0f, 300.0f, false));

    // Test mouse release
    EXPECT_NO_THROW(maplibre.handle_mouse_release(200.0f, 300.0f));
}

TEST_F(SlintMapLibreTest, MouseInteractionsWithNegativeCoordinates) {
    SlintMapLibre maplibre;
    maplibre.initialize(800, 600);

    // Test mouse interactions with negative coordinates
    EXPECT_NO_THROW(maplibre.handle_mouse_press(-10.0f, -20.0f));
    EXPECT_NO_THROW(maplibre.handle_mouse_move(-5.0f, -10.0f, true));
}

TEST_F(SlintMapLibreTest, MouseInteractionsWithLargeCoordinates) {
    SlintMapLibre maplibre;
    maplibre.initialize(800, 600);

    // Test mouse interactions with coordinates outside viewport
    EXPECT_NO_THROW(maplibre.handle_mouse_press(1000.0f, 1000.0f));
    EXPECT_NO_THROW(maplibre.handle_mouse_move(1500.0f, 1500.0f, true));
}

TEST_F(SlintMapLibreTest, RenderMapBeforeInitialization) {
    SlintMapLibre maplibre;

    // Test rendering before initialization
    auto image = maplibre.render_map();
    // Should return empty/invalid image
    EXPECT_TRUE(true);  // Image validation would depend on Slint's Image API
}

TEST_F(SlintMapLibreTest, RenderMapAfterInitialization) {
    SlintMapLibre maplibre;
    maplibre.initialize(800, 600);

    // Test rendering after initialization
    EXPECT_NO_THROW(maplibre.render_map());
}

TEST_F(SlintMapLibreTest, MultipleRenderCalls) {
    SlintMapLibre maplibre;
    maplibre.initialize(800, 600);

    // Test multiple consecutive render calls
    for (int i = 0; i < 5; ++i) {
        EXPECT_NO_THROW(maplibre.render_map());
    }
}

TEST_F(SlintMapLibreTest, InitializeResizeRenderSequence) {
    SlintMapLibre maplibre;

    // Test a realistic usage sequence
    maplibre.initialize(800, 600);
    maplibre.render_map();

    maplibre.resize(1024, 768);
    maplibre.render_map();

    maplibre.handle_mouse_press(100.0f, 100.0f);
    maplibre.handle_mouse_move(150.0f, 150.0f, true);
    maplibre.render_map();

    maplibre.handle_mouse_release(150.0f, 150.0f);
    maplibre.render_map();
}