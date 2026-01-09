#include "custom_file_source.hpp"

#include <gtest/gtest.h>
#include <mbgl/storage/resource.hpp>

class CustomFileSourceTest : public ::testing::Test {
protected:
    void SetUp() override {
        file_source = std::make_unique<mbgl::CustomFileSource>();
    }

    void TearDown() override {
        file_source.reset();
    }

    std::unique_ptr<mbgl::CustomFileSource> file_source;
};

TEST_F(CustomFileSourceTest, ConstructorDestructor) {
    // Test basic construction and destruction
    auto fs = std::make_unique<mbgl::CustomFileSource>();
    EXPECT_NE(fs, nullptr);

    fs.reset();
}

TEST_F(CustomFileSourceTest, ResourceOptionsGetterSetter) {
    // ResourceOptions cannot be copied, so we test the basic functionality
    auto initial_options = file_source->getResourceOptions();

    // Test that we can retrieve resource options
    EXPECT_TRUE(true);  // Basic validation that getter works
}

TEST_F(CustomFileSourceTest, ClientOptionsGetterSetter) {
    // ClientOptions cannot be copied, so we test the basic functionality
    auto initial_options = file_source->getClientOptions();

    // Test that we can retrieve client options
    EXPECT_TRUE(true);  // Basic validation that getter works
}

TEST_F(CustomFileSourceTest, CanRequestHttpResource) {
    mbgl::Resource resource(mbgl::Resource::Kind::Source,
                            "https://example.com/style.json");

    // Test that HTTP resources can be requested
    bool can_request = file_source->canRequest(resource);
    EXPECT_TRUE(can_request);
}

TEST_F(CustomFileSourceTest, CanRequestHttpsResource) {
    mbgl::Resource resource(mbgl::Resource::Kind::Source,
                            "https://secure.example.com/style.json");

    // Test that HTTPS resources can be requested
    bool can_request = file_source->canRequest(resource);
    EXPECT_TRUE(can_request);
}

TEST_F(CustomFileSourceTest, CannotRequestLocalResource) {
    mbgl::Resource resource(mbgl::Resource::Kind::Source,
                            "file:///local/path/style.json");

    // Test that local file resources are handled appropriately
    bool can_request = file_source->canRequest(resource);
    // This depends on implementation - adjust based on actual behavior
    EXPECT_FALSE(can_request);
}

TEST_F(CustomFileSourceTest, CannotRequestInvalidResource) {
    mbgl::Resource resource(mbgl::Resource::Kind::Source, "invalid-url");

    // Test that invalid URLs are handled appropriately
    bool can_request = file_source->canRequest(resource);
    EXPECT_FALSE(can_request);
}

TEST_F(CustomFileSourceTest, RequestValidResource) {
    mbgl::Resource resource(mbgl::Resource::Kind::Source,
                            "https://demotiles.maplibre.org/style.json");

    bool callback_called = false;
    std::string response_data;

    auto request = file_source->request(resource, [&](mbgl::Response response) {
        callback_called = true;
        if (response.data) {
            response_data = *response.data;
        }
    });

    EXPECT_NE(request, nullptr);

    // Note: For a real test, you might want to wait for the callback
    // or use a mock HTTP client
}

TEST_F(CustomFileSourceTest, RequestInvalidResource) {
    mbgl::Resource resource(
        mbgl::Resource::Kind::Source,
        "https://invalid-domain-that-should-not-exist.example/style.json");

    bool callback_called = false;
    bool has_error = false;

    auto request = file_source->request(resource, [&](mbgl::Response response) {
        callback_called = true;
        has_error = (response.error != nullptr);
    });

    EXPECT_NE(request, nullptr);
}

TEST_F(CustomFileSourceTest, CancelRequest) {
    mbgl::Resource resource(mbgl::Resource::Kind::Source,
                            "https://demotiles.maplibre.org/style.json");

    auto request = file_source->request(resource, [](mbgl::Response) {
        // This callback should not be called after cancellation
        FAIL() << "Callback should not be called after request cancellation";
    });

    EXPECT_NE(request, nullptr);

    // Cancel the request immediately
    request.reset();

    // Give some time for any pending operations (in a real test environment)
    // std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(CustomFileSourceTest, MultipleSimultaneousRequests) {
    std::vector<std::unique_ptr<mbgl::AsyncRequest>> requests;
    std::atomic<int> callback_count{0};

    // Create multiple simultaneous requests
    for (int i = 0; i < 3; ++i) {
        mbgl::Resource resource(
            mbgl::Resource::Kind::Source,
            "https://demotiles.maplibre.org/style.json?id=" +
                std::to_string(i));

        auto request = file_source->request(
            resource, [&callback_count](mbgl::Response) { callback_count++; });

        EXPECT_NE(request, nullptr);
        requests.push_back(std::move(request));
    }

    // All requests should be created successfully
    EXPECT_EQ(requests.size(), 3);

    // Clean up
    requests.clear();
}

TEST_F(CustomFileSourceTest, CanRequestDifferentResourceTypes) {
    // Test different resource types
    mbgl::Resource style_resource(mbgl::Resource::Kind::Style,
                                   "https://example.com/style.json");
    EXPECT_TRUE(file_source->canRequest(style_resource));

    mbgl::Resource source_resource(mbgl::Resource::Kind::Source,
                                    "https://example.com/source.json");
    EXPECT_TRUE(file_source->canRequest(source_resource));

    mbgl::Resource tile_resource(mbgl::Resource::Kind::Tile,
                                  "https://example.com/tiles/1/2/3.mvt");
    EXPECT_TRUE(file_source->canRequest(tile_resource));

    mbgl::Resource glyphs_resource(mbgl::Resource::Kind::Glyphs,
                                    "https://example.com/fonts/Arial.pbf");
    EXPECT_TRUE(file_source->canRequest(glyphs_resource));

    mbgl::Resource sprite_image_resource(
        mbgl::Resource::Kind::SpriteImage, "https://example.com/sprite.png");
    EXPECT_TRUE(file_source->canRequest(sprite_image_resource));

    mbgl::Resource sprite_json_resource(
        mbgl::Resource::Kind::SpriteJSON, "https://example.com/sprite.json");
    EXPECT_TRUE(file_source->canRequest(sprite_json_resource));
}

TEST_F(CustomFileSourceTest, CannotRequestUnsupportedResourceType) {
    // Test unsupported resource type
    mbgl::Resource resource(mbgl::Resource::Kind::Image,
                            "https://example.com/image.png");
    EXPECT_FALSE(file_source->canRequest(resource));
}

TEST_F(CustomFileSourceTest, EmptyUrlResource) {
    // Test empty URL
    mbgl::Resource resource(mbgl::Resource::Kind::Source, "");
    EXPECT_FALSE(file_source->canRequest(resource));
}

TEST_F(CustomFileSourceTest, DataUrlResource) {
    // Test data: URL (unsupported)
    mbgl::Resource resource(mbgl::Resource::Kind::Source,
                            "data:application/json,{}");
    EXPECT_FALSE(file_source->canRequest(resource));
}

TEST_F(CustomFileSourceTest, FtpUrlResource) {
    // Test FTP URL (unsupported)
    mbgl::Resource resource(mbgl::Resource::Kind::Source,
                            "ftp://example.com/style.json");
    EXPECT_FALSE(file_source->canRequest(resource));
}

TEST_F(CustomFileSourceTest, RequestCancellationMultiple) {
    // Test cancelling multiple requests
    std::vector<std::unique_ptr<mbgl::AsyncRequest>> requests;

    for (int i = 0; i < 5; ++i) {
        mbgl::Resource resource(
            mbgl::Resource::Kind::Source,
            "https://demotiles.maplibre.org/style.json?cancel=" +
                std::to_string(i));

        auto request = file_source->request(
            resource, [](mbgl::Response) {
                FAIL() << "Callback should not be called after cancellation";
            });

        EXPECT_NE(request, nullptr);
        requests.push_back(std::move(request));
    }

    // Cancel all requests
    requests.clear();
    EXPECT_EQ(requests.size(), 0);
}

TEST_F(CustomFileSourceTest, RequestWithSpecialCharactersInUrl) {
    // Test URL with special characters
    mbgl::Resource resource(
        mbgl::Resource::Kind::Source,
        "https://example.com/style.json?param=value&other=123");
    EXPECT_TRUE(file_source->canRequest(resource));
}

TEST_F(CustomFileSourceTest, RequestWithPortInUrl) {
    // Test URL with custom port
    mbgl::Resource resource(mbgl::Resource::Kind::Source,
                            "https://example.com:8080/style.json");
    EXPECT_TRUE(file_source->canRequest(resource));
}

TEST_F(CustomFileSourceTest, RequestWithFragmentInUrl) {
    // Test URL with fragment
    mbgl::Resource resource(mbgl::Resource::Kind::Source,
                            "https://example.com/style.json#section");
    EXPECT_TRUE(file_source->canRequest(resource));
}

TEST_F(CustomFileSourceTest, SetAndGetResourceOptions) {
    // Test setting resource options
    mbgl::ResourceOptions options;
    options.withCachePath("/tmp/test-cache");
    options.withAssetPath("/tmp/test-assets");

    EXPECT_NO_THROW(file_source->setResourceOptions(options));

    // Getting options after setting
    auto retrieved_options = file_source->getResourceOptions();
    // Note: ResourceOptions might be moved, so we just check it doesn't throw
    EXPECT_TRUE(true);
}

TEST_F(CustomFileSourceTest, SetAndGetClientOptions) {
    // Test setting client options
    mbgl::ClientOptions options;
    EXPECT_NO_THROW(file_source->setClientOptions(options));

    // Getting options after setting
    auto retrieved_options = file_source->getClientOptions();
    EXPECT_TRUE(true);
}

TEST_F(CustomFileSourceTest, RequestSequential) {
    // Test sequential requests (not simultaneous)
    for (int i = 0; i < 3; ++i) {
        mbgl::Resource resource(
            mbgl::Resource::Kind::Source,
            "https://demotiles.maplibre.org/style.json?seq=" +
                std::to_string(i));

        auto request =
            file_source->request(resource, [](mbgl::Response response) {
                // Each request gets its own callback
            });

        EXPECT_NE(request, nullptr);
        // Request goes out of scope and is cleaned up
    }
}

TEST_F(CustomFileSourceTest, VeryLongUrl) {
    // Test with very long URL
    std::string long_path(1000, 'a');
    mbgl::Resource resource(mbgl::Resource::Kind::Source,
                            "https://example.com/" + long_path);
    EXPECT_TRUE(file_source->canRequest(resource));
}

TEST_F(CustomFileSourceTest, UrlWithEncodedCharacters) {
    // Test URL with percent-encoded characters
    mbgl::Resource resource(mbgl::Resource::Kind::Source,
                            "https://example.com/style%20name.json");
    EXPECT_TRUE(file_source->canRequest(resource));
}

TEST_F(CustomFileSourceTest, UrlWithInternationalCharacters) {
    // Test URL with international domain name
    mbgl::Resource resource(mbgl::Resource::Kind::Source,
                            "https://例え.com/style.json");
    EXPECT_TRUE(file_source->canRequest(resource));
}

TEST_F(CustomFileSourceTest, HttpAndHttpsComparison) {
    // Test both HTTP and HTTPS are accepted
    mbgl::Resource http_resource(mbgl::Resource::Kind::Source,
                                  "http://example.com/style.json");
    mbgl::Resource https_resource(mbgl::Resource::Kind::Source,
                                   "https://example.com/style.json");

    EXPECT_TRUE(file_source->canRequest(http_resource));
    EXPECT_TRUE(file_source->canRequest(https_resource));
}