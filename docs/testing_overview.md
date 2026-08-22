# Testing Overview

This document provides an overview of the test suite for maplibre-native-slint.

## Test Structure

The test suite is organized into the following categories:

### Unit Tests

Unit tests verify individual components in isolation:

#### 1. SlintMapLibre Headless Tests (`tests/unit/slint_maplibre_headless_test.cpp`)

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

#### 2. Integration Tests (`tests/unit/integration_test.cpp`)

Tests that verify the map behaves correctly across longer sequences of use:

- **Component Lifecycle**: Initialization and destruction of multiple maps
- **Style Loading**: Setting a style URL on an initialized map
- **Interaction and Rendering**: Mouse, wheel and camera operations followed by a render
- **Resize**: Rendering across size changes
- **Animation Integration**: Fly-to animation driven by animation ticks
- **Stress Testing**: Many interactions in sequence

**Test Count**: 9 test cases

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
./build/Debug/unit-tests --gtest_filter=IntegrationTest.*

# Run specific test case
./build/Debug/unit-tests --gtest_filter=IntegrationTest.MapRendering
```

## Test Coverage

The test suite covers:

- ✅ MapLibre initialization and lifecycle
- ✅ User interaction handling (mouse, wheel)
- ✅ Camera controls (pitch, bearing, zoom)
- ✅ Map rendering
- ✅ Animation system
- ✅ Integration across map lifecycle, interaction and rendering
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
