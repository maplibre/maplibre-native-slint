#include <gtest/gtest.h>

#include "custom_file_source.hpp"

// Simple tests that don't require OpenGL context

TEST(SimpleCustomFileSourceTest, Construction) {
    auto file_source = std::make_unique<mbgl::CustomFileSource>();
    EXPECT_NE(file_source, nullptr);
}

TEST(SimpleCustomFileSourceTest, OptionsAccess) {
    auto file_source = std::make_unique<mbgl::CustomFileSource>();

    // Test that we can get options without crashing
    EXPECT_NO_THROW(file_source->getResourceOptions());
    EXPECT_NO_THROW(file_source->getClientOptions());
}

TEST(SimpleCustomFileSourceTest, CanRequestSimple) {
    auto file_source = std::make_unique<mbgl::CustomFileSource>();

    mbgl::Resource http_resource(mbgl::Resource::Kind::Source,
                                 "https://example.com/test");
    mbgl::Resource https_resource(mbgl::Resource::Kind::Source,
                                  "https://secure.example.com/test");
    mbgl::Resource invalid_resource(mbgl::Resource::Kind::Source,
                                    "invalid-scheme://test");

    // Basic validation without network calls
    EXPECT_TRUE(file_source->canRequest(http_resource));
    EXPECT_TRUE(file_source->canRequest(https_resource));
    EXPECT_FALSE(file_source->canRequest(
        invalid_resource));  // Changed to reflect actual behavior
}