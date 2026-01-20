#include <gtest/gtest.h>
#include <mbgl/storage/resource.hpp>
#include <memory>

#include "custom_file_source.hpp"
#include "slint_maplibre_headless.hpp"

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        file_source = std::make_unique<mbgl::CustomFileSource>();
        slint_map = std::make_unique<SlintMapLibre>();
    }

    void TearDown() override {
        slint_map.reset();
        file_source.reset();
    }

    std::unique_ptr<mbgl::CustomFileSource> file_source;
    std::unique_ptr<SlintMapLibre> slint_map;
};

TEST_F(IntegrationTest, FileSourceAndMapInitialization) {
    // Test that both components can be initialized together
    EXPECT_NE(file_source, nullptr);
    EXPECT_NE(slint_map, nullptr);

    EXPECT_NO_THROW(slint_map->initialize(800, 600));
}

TEST_F(IntegrationTest, FileSourceCanHandleMapResources) {
    // Test that file source can handle typical map resources
    slint_map->initialize(800, 600);

    // Test various resources that a map would request
    mbgl::Resource style_resource(mbgl::Resource::Kind::Style,
                                  "https://demotiles.maplibre.org/style.json");
    EXPECT_TRUE(file_source->canRequest(style_resource));

    mbgl::Resource tile_resource(
        mbgl::Resource::Kind::Tile,
        "https://demotiles.maplibre.org/tiles/1/2/3.mvt");
    EXPECT_TRUE(file_source->canRequest(tile_resource));
}

TEST_F(IntegrationTest, MapWithStyleAndFileSource) {
    // Test map initialization with style URL
    slint_map->initialize(800, 600);
    EXPECT_NO_THROW(
        slint_map->setStyleUrl("https://demotiles.maplibre.org/style.json"));

    // Verify file source can handle the style URL
    mbgl::Resource resource(mbgl::Resource::Kind::Style,
                            "https://demotiles.maplibre.org/style.json");
    EXPECT_TRUE(file_source->canRequest(resource));
}

TEST_F(IntegrationTest, MapInteractionsWithFileSource) {
    // Test map interactions while file source is active
    slint_map->initialize(800, 600);

    // Perform various map interactions
    slint_map->handle_mouse_press(100.0f, 100.0f);
    slint_map->handle_mouse_move(150.0f, 150.0f, true);
    slint_map->handle_mouse_release(150.0f, 150.0f);

    // Verify file source still works
    mbgl::Resource resource(mbgl::Resource::Kind::Source,
                            "https://example.com/test.json");
    EXPECT_TRUE(file_source->canRequest(resource));
}

TEST_F(IntegrationTest, MultipleComponentsLifecycle) {
    // Test creating and destroying multiple components
    auto fs1 = std::make_unique<mbgl::CustomFileSource>();
    auto map1 = std::make_unique<SlintMapLibre>();
    map1->initialize(640, 480);

    auto fs2 = std::make_unique<mbgl::CustomFileSource>();
    auto map2 = std::make_unique<SlintMapLibre>();
    map2->initialize(1024, 768);

    // Both should work independently
    mbgl::Resource resource(mbgl::Resource::Kind::Source,
                            "https://example.com/source.json");
    EXPECT_TRUE(fs1->canRequest(resource));
    EXPECT_TRUE(fs2->canRequest(resource));

    // Clean up in order
    map1.reset();
    fs1.reset();
    map2.reset();
    fs2.reset();
}

TEST_F(IntegrationTest, FileSourceRequestDuringMapRendering) {
    // Test file source requests while map is being used
    slint_map->initialize(800, 600);

    bool request_completed = false;
    mbgl::Resource resource(mbgl::Resource::Kind::Source,
                            "https://demotiles.maplibre.org/style.json");

    auto request = file_source->request(
        resource, [&request_completed](mbgl::Response response) {
            request_completed = true;
        });

    EXPECT_NE(request, nullptr);

    // Perform map operations while request might be in progress
    slint_map->set_pitch(30);
    slint_map->set_bearing(45.0f);
    slint_map->handle_wheel_zoom(400.0f, 300.0f, 1.0f);

    // Clean up request
    request.reset();
}

TEST_F(IntegrationTest, MapResizeWithActiveFileSource) {
    // Test map resize while file source is active
    slint_map->initialize(800, 600);

    mbgl::Resource resource(mbgl::Resource::Kind::Source,
                            "https://example.com/test.json");
    auto request =
        file_source->request(resource, [](mbgl::Response response) {});

    EXPECT_NE(request, nullptr);

    // Resize map
    EXPECT_NO_THROW(slint_map->resize(1024, 768));
    EXPECT_NO_THROW(slint_map->resize(640, 480));

    request.reset();
}

TEST_F(IntegrationTest, FileSourceResourceOptions) {
    // Test file source resource options interaction
    slint_map->initialize(800, 600);

    mbgl::ResourceOptions options;
    options.withCachePath("/tmp/integration-test-cache");
    options.withAssetPath("./assets");

    file_source->setResourceOptions(std::move(options));

    // Map operations should still work
    slint_map->set_pitch(45);
    slint::Image img;
    EXPECT_NO_THROW(img = slint_map->render_map());
}

TEST_F(IntegrationTest, ConcurrentMapAndFileSourceOperations) {
    // Test concurrent operations on map and file source
    slint_map->initialize(800, 600);

    // Create multiple file source requests
    std::vector<std::unique_ptr<mbgl::AsyncRequest>> requests;
    for (int i = 0; i < 3; ++i) {
        mbgl::Resource resource(
            mbgl::Resource::Kind::Source,
            "https://example.com/test" + std::to_string(i) + ".json");
        auto request =
            file_source->request(resource, [](mbgl::Response response) {});
        requests.push_back(std::move(request));
    }

    // Perform map operations
    slint_map->handle_mouse_press(200.0f, 200.0f);
    slint_map->handle_mouse_move(250.0f, 250.0f, true);
    slint_map->handle_mouse_release(250.0f, 250.0f);
    slint_map->set_bearing(90.0f);

    // Clean up
    requests.clear();
}

TEST_F(IntegrationTest, MapRenderingWithFileSourceRequests) {
    // Test map rendering while file source handles requests
    slint_map->initialize(800, 600);

    // Start file source request
    mbgl::Resource resource(mbgl::Resource::Kind::Source,
                            "https://demotiles.maplibre.org/style.json");
    auto request =
        file_source->request(resource, [](mbgl::Response response) {});

    // Render map multiple times
    for (int i = 0; i < 3; ++i) {
        slint::Image img;
        EXPECT_NO_THROW(img = slint_map->render_map());
    }

    request.reset();
}

TEST_F(IntegrationTest, FileSourceAndMapWithDifferentResourceTypes) {
    // Test file source handling different resource types for map
    slint_map->initialize(800, 600);

    // Test all resource types that map might use
    struct ResourceTest {
        mbgl::Resource::Kind kind;
        std::string url;
    };

    std::vector<ResourceTest> resource_tests = {
        {mbgl::Resource::Kind::Style, "https://example.com/style.json"},
        {mbgl::Resource::Kind::Source, "https://example.com/source.json"},
        {mbgl::Resource::Kind::Tile, "https://example.com/tiles/1/2/3.mvt"},
        {mbgl::Resource::Kind::Glyphs, "https://example.com/fonts/font.pbf"},
        {mbgl::Resource::Kind::SpriteImage, "https://example.com/sprite.png"},
        {mbgl::Resource::Kind::SpriteJSON, "https://example.com/sprite.json"}};

    for (const auto& test : resource_tests) {
        mbgl::Resource resource(test.kind, test.url);
        EXPECT_TRUE(file_source->canRequest(resource))
            << "Failed for resource type: " << static_cast<int>(test.kind);
    }

    // Map should still be functional
    slint_map->set_pitch(30);
    slint_map->set_bearing(45.0f);
}

TEST_F(IntegrationTest, MapAnimationWithFileSourceActive) {
    // Test map animation while file source is active
    slint_map->initialize(800, 600);

    // Start animation
    EXPECT_NO_THROW(slint_map->fly_to("tokyo"));

    // File source should still work
    mbgl::Resource resource(mbgl::Resource::Kind::Source,
                            "https://example.com/test.json");
    EXPECT_TRUE(file_source->canRequest(resource));

    // Tick animation
    for (int i = 0; i < 5; ++i) {
        EXPECT_NO_THROW(slint_map->tick_animation());
    }
}

TEST_F(IntegrationTest, StressTest) {
    // Stress test with many operations
    slint_map->initialize(800, 600);

    // Multiple file source requests
    std::vector<std::unique_ptr<mbgl::AsyncRequest>> requests;
    for (int i = 0; i < 10; ++i) {
        mbgl::Resource resource(
            mbgl::Resource::Kind::Source,
            "https://example.com/stress" + std::to_string(i) + ".json");
        auto request =
            file_source->request(resource, [](mbgl::Response response) {});
        requests.push_back(std::move(request));
    }

    // Multiple map operations
    for (int i = 0; i < 10; ++i) {
        slint_map->handle_mouse_press(100.0f + i * 10, 100.0f + i * 10);
        slint_map->handle_mouse_release(100.0f + i * 10, 100.0f + i * 10);
        slint_map->handle_wheel_zoom(400.0f, 300.0f, 0.5f);
    }

    // Clean up
    requests.clear();
}
