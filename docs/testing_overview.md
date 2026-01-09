# Testing Overview

This document provides an overview of the test suite for maplibre-native-slint.

## Test Structure

The test suite is organized into the following categories:

### Unit Tests

Unit tests verify individual components in isolation:

#### 1. Custom File Source Tests (`tests/unit/custom_file_source_test.cpp`)

Tests for the `CustomFileSource` class, which handles HTTP/HTTPS resource requests for MapLibre:

- **Basic Operations**: Constructor, destructor, resource options, client options
- **Resource Validation**: URL validation, protocol checking (HTTP/HTTPS), resource type filtering
- **Request Handling**: Synchronous and asynchronous requests, request cancellation, simultaneous requests
- **Edge Cases**: Empty URLs, invalid URLs, special characters, international characters, very long URLs
- **Error Handling**: Network errors, invalid domains, HTTP status codes

**Test Count**: 20+ test cases

#### 2. SlintMapLibre Headless Tests (`tests/unit/slint_maplibre_headless_test.cpp`)

Tests for the `SlintMapLibre` class, which integrates MapLibre with Slint UI:

- **Initialization**: Various window sizes, resize operations
- **Mouse Interactions**: Press, release, move, drag, double-click with modifiers
- **Zoom Operations**: Wheel zoom in/out, extreme zoom values
- **Camera Controls**: Pitch and bearing settings, extreme values
- **Rendering**: Map rendering, multiple consecutive renders
- **Animations**: Fly-to animation, animation ticks
- **Callbacks**: Render callbacks, repaint requests
- **Complex Sequences**: Multiple interactions in sequence

**Test Count**: 30+ test cases

#### 3. Integration Tests (`tests/unit/integration_test.cpp`)

Tests that verify multiple components working together:

- **Component Lifecycle**: Initialization and destruction of multiple components
- **File Source and Map Interaction**: Resource handling during map operations
- **Concurrent Operations**: Simultaneous file source requests and map interactions
- **Stress Testing**: Many operations and requests at once
- **Resource Types**: All MapLibre resource types with file source
- **Animation Integration**: Map animations while file source is active

**Test Count**: 15+ test cases

## Running Tests

### Prerequisites

Before running tests, you need to build the project. Follow the build instructions for your platform in the `docs/build_guides/` directory.

### Building Tests

```bash
# Configure the project (if not already done)
cmake -B build -DCMAKE_BUILD_TYPE=Release -G Xcode .

# Build tests
cmake --build build --target unit-tests
```

### Running Tests

```bash
# Run all unit tests
./build/Debug/unit-tests

# Run with verbose output
./build/Debug/unit-tests --gtest_verbose

# Run specific test suite
./build/Debug/unit-tests --gtest_filter=CustomFileSourceTest.*

# Run specific test case
./build/Debug/unit-tests --gtest_filter=CustomFileSourceTest.CanRequestHttpResource
```

## Test Coverage

The test suite covers:

- ✅ Custom file source HTTP/HTTPS request handling
- ✅ MapLibre initialization and lifecycle
- ✅ User interaction handling (mouse, wheel)
- ✅ Camera controls (pitch, bearing, zoom)
- ✅ Map rendering
- ✅ Animation system
- ✅ Integration between file source and map
- ✅ Edge cases and error handling

## Adding New Tests

When adding new functionality to the codebase:

1. Add unit tests for the new component in `tests/unit/`
2. Add integration tests if the component interacts with existing systems
3. Follow the existing test structure and naming conventions
4. Use GoogleTest macros: `TEST_F`, `EXPECT_*`, `ASSERT_*`
5. Ensure tests are deterministic and don't depend on external resources when possible

### Test File Template

```cpp
#include "your_component.hpp"
#include <gtest/gtest.h>

class YourComponentTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test fixtures
    }

    void TearDown() override {
        // Clean up test fixtures
    }

    // Test fixture members
};

TEST_F(YourComponentTest, TestName) {
    // Test implementation
    EXPECT_TRUE(true);
}
```

## Continuous Integration

Tests should pass on all supported platforms:

- Linux (Ubuntu 24.04+)
- macOS (Apple Silicon)
- Windows 11

## Test Dependencies

The test suite uses:

- **GoogleTest**: Testing framework (provided by MapLibre Native)
- **MapLibre Native**: Core rendering engine
- **Slint**: UI framework
- **CPR**: HTTP client library

All dependencies are managed through vcpkg and the existing build system.

## Known Limitations

- Some tests may require an OpenGL context (handled by HeadlessFrontend)
- Network-dependent tests use mock responses where possible
- macOS-specific considerations for RunLoop handling
- Tests requiring actual network requests may fail in offline environments

## Future Improvements

Planned enhancements to the test suite:

- [ ] Mock HTTP client for fully offline testing
- [ ] Performance benchmarks
- [ ] Memory leak detection
- [ ] Coverage reports
- [ ] Automated UI testing with Slint
- [ ] Fuzzing tests for robustness
