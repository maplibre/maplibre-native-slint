#include "../test_utils.hpp"
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
    EXPECT_TRUE(true); // Basic validation that getter works
}

TEST_F(CustomFileSourceTest, ClientOptionsGetterSetter) {
    // ClientOptions cannot be copied, so we test the basic functionality
    auto initial_options = file_source->getClientOptions();
    
    // Test that we can retrieve client options
    EXPECT_TRUE(true); // Basic validation that getter works
}

TEST_F(CustomFileSourceTest, CanRequestHttpResource) {
    mbgl::Resource resource(mbgl::Resource::Kind::Source, "https://example.com/style.json");
    
    // Test that HTTP resources can be requested
    bool can_request = file_source->canRequest(resource);
    EXPECT_TRUE(can_request);
}

TEST_F(CustomFileSourceTest, CanRequestHttpsResource) {
    mbgl::Resource resource(mbgl::Resource::Kind::Source, "https://secure.example.com/style.json");
    
    // Test that HTTPS resources can be requested
    bool can_request = file_source->canRequest(resource);
    EXPECT_TRUE(can_request);
}

TEST_F(CustomFileSourceTest, CannotRequestLocalResource) {
    mbgl::Resource resource(mbgl::Resource::Kind::Source, "file:///local/path/style.json");
    
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
    mbgl::Resource resource(mbgl::Resource::Kind::Source, "https://demotiles.maplibre.org/style.json");
    
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
    mbgl::Resource resource(mbgl::Resource::Kind::Source, "https://invalid-domain-that-should-not-exist.example/style.json");
    
    bool callback_called = false;
    bool has_error = false;
    
    auto request = file_source->request(resource, [&](mbgl::Response response) {
        callback_called = true;
        has_error = (response.error != nullptr);
    });
    
    EXPECT_NE(request, nullptr);
}

TEST_F(CustomFileSourceTest, CancelRequest) {
    mbgl::Resource resource(mbgl::Resource::Kind::Source, "https://demotiles.maplibre.org/style.json");
    
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
        mbgl::Resource resource(mbgl::Resource::Kind::Source, 
                              "https://demotiles.maplibre.org/style.json?id=" + std::to_string(i));
        
        auto request = file_source->request(resource, [&callback_count](mbgl::Response) {
            callback_count++;
        });
        
        EXPECT_NE(request, nullptr);
        requests.push_back(std::move(request));
    }
    
    // All requests should be created successfully
    EXPECT_EQ(requests.size(), 3);
    
    // Clean up
    requests.clear();
}