#include "test_utils.hpp"

#include <cmath>
#include <map>

namespace test {

void OpenGLTest::SetUp() {
    // Note: In a real test environment, you would need to initialize an OpenGL
    // context For headless testing, consider using EGL or a similar solution
}

void OpenGLTest::TearDown() {
    // Cleanup OpenGL resources if needed
}

bool OpenGLTest::hasValidOpenGLContext() const {
    // Simple check for OpenGL context availability
    return glGetError() != GL_INVALID_OPERATION;
}

MockFileSource::MockFileSource() = default;
MockFileSource::~MockFileSource() = default;

void MockFileSource::setResponse(const std::string& url,
                                 const std::string& data) {
    responses[url] = data;
}

std::string MockFileSource::getResponse(const std::string& url) const {
    auto it = responses.find(url);
    if (it != responses.end()) {
        return it->second;
    }
    return "";
}

bool compareImages(const std::vector<uint8_t>& img1,
                   const std::vector<uint8_t>& img2, int width, int height,
                   double threshold) {
    if (img1.size() != img2.size()) {
        return false;
    }

    if (img1.size() != static_cast<size_t>(width * height * 4)) {
        return false;
    }

    double totalDifference = 0.0;
    size_t pixelCount = width * height;

    for (size_t i = 0; i < pixelCount; ++i) {
        size_t baseIndex = i * 4;

        // Calculate per-channel differences
        double rDiff = std::abs(static_cast<int>(img1[baseIndex]) -
                                static_cast<int>(img2[baseIndex]));
        double gDiff = std::abs(static_cast<int>(img1[baseIndex + 1]) -
                                static_cast<int>(img2[baseIndex + 1]));
        double bDiff = std::abs(static_cast<int>(img1[baseIndex + 2]) -
                                static_cast<int>(img2[baseIndex + 2]));
        double aDiff = std::abs(static_cast<int>(img1[baseIndex + 3]) -
                                static_cast<int>(img2[baseIndex + 3]));

        // Average difference for this pixel
        double pixelDiff = (rDiff + gDiff + bDiff + aDiff) / (4.0 * 255.0);
        totalDifference += pixelDiff;
    }

    double avgDifference = totalDifference / pixelCount;
    return avgDifference <= threshold;
}

std::vector<uint8_t> createTestTexture(int width, int height, uint8_t r,
                                       uint8_t g, uint8_t b, uint8_t a) {
    std::vector<uint8_t> data(width * height * 4);

    for (int i = 0; i < width * height; ++i) {
        data[i * 4] = r;
        data[i * 4 + 1] = g;
        data[i * 4 + 2] = b;
        data[i * 4 + 3] = a;
    }

    return data;
}

}  // namespace test