#pragma once

#include <GLES3/gl3.h>
#include <gtest/gtest.h>
#include <mbgl/test/util.hpp>

namespace test {

// Base test class for OpenGL context management
class OpenGLTest : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;

    bool hasValidOpenGLContext() const;
};

// Mock file source for testing without network
class MockFileSource {
public:
    MockFileSource();
    ~MockFileSource();

    void setResponse(const std::string& url, const std::string& data);
    std::string getResponse(const std::string& url) const;

private:
    std::map<std::string, std::string> responses;
};

// Helper functions
bool compareImages(const std::vector<uint8_t>& img1,
                   const std::vector<uint8_t>& img2, int width, int height,
                   double threshold = 0.01);

std::vector<uint8_t> createTestTexture(int width, int height, uint8_t r,
                                       uint8_t g, uint8_t b, uint8_t a = 255);

}  // namespace test