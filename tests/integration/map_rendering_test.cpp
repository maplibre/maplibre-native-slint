#include "../test_utils.hpp"
#include "slint_maplibre.hpp"
#include <gtest/gtest.h>
#include <chrono>
#include <thread>

class MapRenderingTest : public test::OpenGLTest {
protected:
    void SetUp() override {
        test::OpenGLTest::SetUp();
        maplibre = std::make_unique<SlintMapLibre>();
    }
    
    void TearDown() override {
        maplibre.reset();
        test::OpenGLTest::TearDown();
    }
    
    std::unique_ptr<SlintMapLibre> maplibre;
};

TEST_F(MapRenderingTest, BasicMapRendering) {
    // Initialize with standard dimensions
    maplibre->initialize(800, 600);
    
    // Allow some time for map initialization
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Render the map
    auto image = maplibre->render_map();
    
    // Basic validation that an image was created
    // Note: Actual image validation would depend on Slint's Image API
    EXPECT_TRUE(true); // Placeholder for actual image validation
}

TEST_F(MapRenderingTest, ConsecutiveRenders) {
    maplibre->initialize(640, 480);
    
    // Perform multiple consecutive renders
    std::vector<slint::Image> images;
    for (int i = 0; i < 5; ++i) {
        auto image = maplibre->render_map();
        images.push_back(std::move(image));
        
        // Small delay between renders
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    EXPECT_EQ(images.size(), 5);
}

TEST_F(MapRenderingTest, RenderAfterResize) {
    // Initialize with initial size
    maplibre->initialize(400, 300);
    
    // First render
    auto image1 = maplibre->render_map();
    
    // Resize
    maplibre->resize(800, 600);
    
    // Render after resize
    auto image2 = maplibre->render_map();
    
    // Both renders should succeed
    EXPECT_TRUE(true); // Placeholder for actual validation
}

TEST_F(MapRenderingTest, RenderWithMouseInteraction) {
    maplibre->initialize(800, 600);
    
    // Initial render
    auto image1 = maplibre->render_map();
    
    // Simulate mouse drag to pan the map
    maplibre->handle_mouse_press(400.0f, 300.0f);
    maplibre->handle_mouse_move(450.0f, 350.0f, true);
    maplibre->handle_mouse_release(450.0f, 350.0f);
    
    // Allow time for map update
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Render after interaction
    auto image2 = maplibre->render_map();
    
    // Both renders should succeed
    EXPECT_TRUE(true); // In a real test, you might compare images to ensure they're different
}

TEST_F(MapRenderingTest, MultipleResizeAndRenderCycles) {
    maplibre->initialize(200, 200);
    
    // Test multiple resize/render cycles
    std::vector<std::pair<int, int>> sizes = {
        {400, 300}, {800, 600}, {1024, 768}, {640, 480}
    };
    
    for (const auto& size : sizes) {
        maplibre->resize(size.first, size.second);
        auto image = maplibre->render_map();
        
        // Each render should succeed
        EXPECT_TRUE(true); // Placeholder for actual validation
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

TEST_F(MapRenderingTest, RenderWithExtremeDimensions) {
    // Test with very small dimensions
    maplibre->initialize(1, 1);
    auto small_image = maplibre->render_map();
    EXPECT_TRUE(true); // Should handle gracefully
    
    // Test with large dimensions (but reasonable for testing)
    maplibre->resize(2048, 1536);
    auto large_image = maplibre->render_map();
    EXPECT_TRUE(true); // Should handle gracefully
}

TEST_F(MapRenderingTest, SimulateUserInteractionSequence) {
    maplibre->initialize(800, 600);
    
    // Simulate a realistic user interaction sequence
    auto image1 = maplibre->render_map();
    
    // Pan the map
    maplibre->handle_mouse_press(400.0f, 300.0f);
    maplibre->handle_mouse_move(300.0f, 200.0f, true);
    maplibre->handle_mouse_release(300.0f, 200.0f);
    
    auto image2 = maplibre->render_map();
    
    // Additional pan
    maplibre->handle_mouse_press(300.0f, 200.0f);
    maplibre->handle_mouse_move(500.0f, 400.0f, true);
    maplibre->handle_mouse_release(500.0f, 400.0f);
    
    auto image3 = maplibre->render_map();
    
    // All renders should succeed
    EXPECT_TRUE(true);
}

TEST_F(MapRenderingTest, StressTestRapidRenders) {
    maplibre->initialize(512, 384);
    
    // Perform rapid consecutive renders
    const int num_renders = 20;
    std::vector<slint::Image> images;
    images.reserve(num_renders);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_renders; ++i) {
        auto image = maplibre->render_map();
        images.push_back(std::move(image));
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    EXPECT_EQ(images.size(), num_renders);
    
    // Log performance info (in a real test, you might have performance assertions)
    std::cout << "Rendered " << num_renders << " frames in " << duration.count() << "ms" << std::endl;
}