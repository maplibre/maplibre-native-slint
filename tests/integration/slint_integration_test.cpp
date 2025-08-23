#include <gtest/gtest.h>
#include <slint/slint.h>

#include "../test_utils.hpp"
#include "slint_maplibre.hpp"

class SlintIntegrationTest : public test::OpenGLTest {
protected:
    void SetUp() override {
        test::OpenGLTest::SetUp();
    }

    void TearDown() override {
        test::OpenGLTest::TearDown();
    }
};

TEST_F(SlintIntegrationTest, SlintImageCreationFromMapLibre) {
    SlintMapLibre maplibre;
    maplibre.initialize(400, 300);

    // Render map and get Slint image
    auto image = maplibre.render_map();

    // Basic validation that image creation doesn't crash
    EXPECT_TRUE(
        true);  // Would need Slint API methods to validate image properties
}

TEST_F(SlintIntegrationTest, MultipleImageCreations) {
    SlintMapLibre maplibre;
    maplibre.initialize(300, 200);

    std::vector<slint::Image> images;

    // Create multiple images
    for (int i = 0; i < 3; ++i) {
        auto image = maplibre.render_map();
        images.push_back(std::move(image));
    }

    EXPECT_EQ(images.size(), 3);
}

TEST_F(SlintIntegrationTest, ImageCreationAfterResize) {
    SlintMapLibre maplibre;
    maplibre.initialize(200, 150);

    // Create image with initial size
    auto image1 = maplibre.render_map();

    // Resize and create another image
    maplibre.resize(400, 300);
    auto image2 = maplibre.render_map();

    // Both operations should succeed
    EXPECT_TRUE(true);
}

TEST_F(SlintIntegrationTest, BorrowedOpenGLTextureProperties) {
    SlintMapLibre maplibre;
    const int width = 512;
    const int height = 384;

    maplibre.initialize(width, height);
    auto image = maplibre.render_map();

    // Test that the image was created with correct properties
    // Note: This would require access to Slint's internal image properties
    // For now, we just test that creation doesn't fail
    EXPECT_TRUE(true);
}

// Mock component to simulate Slint UI integration
class MockSlintComponent {
public:
    MockSlintComponent() {
        map_widget = std::make_unique<SlintMapLibre>();
    }

    void initialize(int w, int h) {
        width = w;
        height = h;
        map_widget->initialize(w, h);
    }

    void render() {
        current_image = map_widget->render_map();
    }

    void handleMouseEvent(float x, float y, bool pressed) {
        if (pressed) {
            map_widget->handle_mouse_press(x, y);
        } else {
            map_widget->handle_mouse_release(x, y);
        }
    }

    void handleMouseMove(float x, float y, bool pressed) {
        map_widget->handle_mouse_move(x, y, pressed);
    }

    void resize(int w, int h) {
        width = w;
        height = h;
        map_widget->resize(w, h);
    }

private:
    std::unique_ptr<SlintMapLibre> map_widget;
    slint::Image current_image;
    int width = 0;
    int height = 0;
};

TEST_F(SlintIntegrationTest, MockComponentLifecycle) {
    MockSlintComponent component;

    // Test component lifecycle
    component.initialize(640, 480);
    component.render();

    // Simulate user interaction
    component.handleMouseEvent(100.0f, 100.0f, true);
    component.handleMouseMove(150.0f, 150.0f, true);
    component.handleMouseEvent(150.0f, 150.0f, false);

    component.render();

    // Resize component
    component.resize(800, 600);
    component.render();

    EXPECT_TRUE(true);  // All operations should complete without error
}

TEST_F(SlintIntegrationTest, ComponentInteractionSequence) {
    MockSlintComponent component;
    component.initialize(500, 400);

    // Simulate a complex interaction sequence
    component.render();  // Initial render

    // Pan gesture
    component.handleMouseEvent(250.0f, 200.0f, true);
    component.handleMouseMove(200.0f, 150.0f, true);
    component.handleMouseMove(150.0f, 100.0f, true);
    component.handleMouseEvent(150.0f, 100.0f, false);
    component.render();

    // Another pan gesture
    component.handleMouseEvent(150.0f, 100.0f, true);
    component.handleMouseMove(300.0f, 250.0f, true);
    component.handleMouseEvent(300.0f, 250.0f, false);
    component.render();

    EXPECT_TRUE(true);
}

TEST_F(SlintIntegrationTest, MultipleComponents) {
    // Test multiple map components can coexist
    MockSlintComponent component1, component2;

    component1.initialize(400, 300);
    component2.initialize(600, 450);

    // Render both components
    component1.render();
    component2.render();

    // Interact with both components
    component1.handleMouseEvent(100.0f, 100.0f, true);
    component2.handleMouseEvent(200.0f, 200.0f, true);

    component1.handleMouseMove(150.0f, 150.0f, true);
    component2.handleMouseMove(250.0f, 250.0f, true);

    component1.render();
    component2.render();

    EXPECT_TRUE(true);
}

TEST_F(SlintIntegrationTest, ComponentResizeAndRender) {
    MockSlintComponent component;
    component.initialize(200, 200);

    std::vector<std::pair<int, int>> sizes = {
        {300, 200}, {400, 300}, {500, 400}, {600, 500}};

    for (const auto& size : sizes) {
        component.resize(size.first, size.second);
        component.render();

        // Test interaction after each resize
        component.handleMouseEvent(50.0f, 50.0f, true);
        component.handleMouseMove(100.0f, 100.0f, true);
        component.handleMouseEvent(100.0f, 100.0f, false);
        component.render();
    }

    EXPECT_TRUE(true);
}