# Testing Guide

This document describes the testing setup for the MapLibre Native + Slint integration project.

## Overview

The test suite is organized into three main categories:

1. **Simple Unit Tests** - Basic tests that don't require OpenGL context
2. **Unit Tests** - Test individual components in isolation (requires OpenGL)
3. **Integration Tests** - Test the interaction between MapLibre Native and Slint (requires OpenGL)

## Building and Running Tests

### Prerequisites

- All standard build dependencies (see main README.md)
- OpenGL/EGL context for headless testing (recommended for CI)

### Build with Tests

```bash
# Configure with tests enabled (default)
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON

# Or disable tests
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF

# Build everything including tests
cmake --build .

# Run all tests (some may fail without OpenGL context)
ctest --output-on-failure

# Run only simple tests (no OpenGL required)
./tests/simple-unit-tests

# Run specific test suites (require OpenGL context)
./tests/unit-tests
./tests/integration-tests

# For headless environments, use xvfb
xvfb-run -a ./tests/unit-tests
xvfb-run -a ./tests/integration-tests
```

## Test Structure

```
tests/
├── CMakeLists.txt              # Test build configuration
├── test_utils.hpp             # Common test utilities
├── test_utils.cpp
├── unit/                      # Unit tests
│   ├── simple_unit_test.cpp   # Simple tests (no OpenGL)
│   ├── slint_maplibre_test.cpp
│   ├── custom_file_source_test.cpp
│   ├── custom_run_loop_test.cpp
│   └── test_main.cpp          # GoogleTest main
└── integration/               # Integration tests
    ├── map_rendering_test.cpp
    ├── slint_integration_test.cpp
    └── test_main.cpp          # GoogleTest main
```

## Test Categories

### Simple Unit Tests (`unit/simple_unit_test.cpp`)
- Basic component construction and destruction
- Resource options access without network calls
- Run loop creation without OpenGL context
- **✅ Safe to run in headless environments**

### Unit Tests (OpenGL Required)

#### SlintMapLibre Tests (`unit/slint_maplibre_test.cpp`)
- Constructor/destructor behavior
- Initialization with various dimensions
- Resize operations
- Mouse interaction handling
- Render method behavior

#### CustomFileSource Tests (`unit/custom_file_source_test.cpp`)
- HTTP/HTTPS resource requests
- Resource validation
- Request cancellation
- Multiple simultaneous requests
- Error handling

#### CustomRunLoop Tests (`unit/custom_run_loop_test.cpp`)
- Run loop creation and management
- Thread safety
- Multiple run loop instances
- Proper cleanup

### Integration Tests

#### Map Rendering Tests (`integration/map_rendering_test.cpp`)
- Basic map rendering functionality
- Consecutive renders
- Rendering after resize operations
- Interactive rendering (with mouse input)
- Performance stress tests

#### Slint Integration Tests (`integration/slint_integration_test.cpp`)
- Slint Image creation from MapLibre output
- OpenGL texture borrowing
- Component lifecycle management
- Multiple component instances
- UI interaction simulation

## Test Utilities

The `test_utils.hpp` and `test_utils.cpp` provide common functionality:

### OpenGLTest Base Class
```cpp
class OpenGLTest : public ::testing::Test {
    // Provides OpenGL context management for tests
};
```

### Mock Components
- `MockFileSource` - For testing without network dependencies
- Helper functions for image comparison and test data generation

### Utility Functions
- `compareImages()` - Compare rendered image output
- `createTestTexture()` - Generate test texture data
- `hasValidOpenGLContext()` - Validate OpenGL availability

## Writing New Tests

### Unit Test Example
```cpp
#include "../test_utils.hpp"
#include "your_component.hpp"
#include <gtest/gtest.h>

class YourComponentTest : public ::testing::Test {
protected:
    void SetUp() override {
        component = std::make_unique<YourComponent>();
    }
    
    std::unique_ptr<YourComponent> component;
};

TEST_F(YourComponentTest, BasicFunctionality) {
    EXPECT_TRUE(component->initialize());
    EXPECT_NO_THROW(component->doSomething());
}
```

### Integration Test Example
```cpp
#include "../test_utils.hpp"
#include "slint_maplibre.hpp"
#include <gtest/gtest.h>

class YourIntegrationTest : public test::OpenGLTest {
protected:
    void SetUp() override {
        test::OpenGLTest::SetUp();
        // Additional setup
    }
};

TEST_F(YourIntegrationTest, ComponentInteraction) {
    // Test interaction between components
}
```

## CI/CD Integration

### Quick Start for CI
```bash
# Minimal test run (no display required)
cmake --build . --target simple-unit-tests
./tests/simple-unit-tests

# Full test run with virtual display
xvfb-run -a -s "-screen 0 1024x768x24" ctest --output-on-failure
```

### GitHub Actions Example
```yaml
name: Tests
on: [push, pull_request]

jobs:
  # Quick tests without OpenGL
  simple-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive
      
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake build-essential
      
      - name: Build and test (simple)
        run: |
          mkdir build && cd build
          cmake .. -DCMAKE_BUILD_TYPE=Release
          cmake --build . --target simple-unit-tests
          ./tests/simple-unit-tests

  # Full tests with OpenGL
  full-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive
      
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake build-essential libgl1-mesa-dev xvfb
      
      - name: Build and test (full)
        run: |
          mkdir build && cd build
          cmake .. -DCMAKE_BUILD_TYPE=Release
          cmake --build .
          xvfb-run -a ctest --output-on-failure
```

## Headless Testing

For CI environments without display:

```bash
# Use Xvfb for headless OpenGL
xvfb-run -a -s "-screen 0 1024x768x24" ctest --output-on-failure

# Or use EGL for headless rendering (if available)
export MESA_GL_VERSION_OVERRIDE=3.3
export MESA_GLSL_VERSION_OVERRIDE=330
```

## Test Execution Status

### ✅ Working Tests
- `simple-unit-tests` - All tests pass in any environment
- Basic component construction and API validation

### ⚠️ Requires OpenGL Context
- `unit-tests` - Full unit test suite (requires display or xvfb)
- `integration-tests` - Full integration tests (requires display or xvfb)
- MapLibre Native's own test suites

### 🔧 Example Test Results
```bash
$ ./tests/simple-unit-tests
[==========] Running 4 tests from 2 test suites.
[----------] 1 test from SimpleCustomRunLoopTest
[ RUN      ] SimpleCustomRunLoopTest.Construction
[       OK ] SimpleCustomRunLoopTest.Construction (0 ms)
[----------] 3 tests from SimpleCustomFileSourceTest
[ RUN      ] SimpleCustomFileSourceTest.Construction
[       OK ] SimpleCustomFileSourceTest.Construction (0 ms)
[ RUN      ] SimpleCustomFileSourceTest.OptionsAccess
[       OK ] SimpleCustomFileSourceTest.OptionsAccess (0 ms)
[ RUN      ] SimpleCustomFileSourceTest.CanRequestSimple
[       OK ] SimpleCustomFileSourceTest.CanRequestSimple (0 ms)
[  PASSED  ] 4 tests.
```

## Troubleshooting

### Common Issues

1. **OpenGL Context Not Available**
   - **Solution**: Use `simple-unit-tests` for basic validation
   - Use headless rendering solutions (xvfb) for full tests
   - Ensure proper graphics drivers are installed
   - Check that test environment supports OpenGL 3.3+

2. **Network-Dependent Tests Failing**
   - Tests should use mock file sources when possible
   - Consider using local test tile servers for integration tests

3. **Memory Leaks in Tests**
   - Ensure proper cleanup in test teardown
   - Use smart pointers and RAII patterns
   - Run tests with AddressSanitizer in development

4. **libcurl Version Warnings**
   - Warning: `no version information available` is cosmetic
   - Does not affect test functionality
   - Related to static vs shared library linking

### Test Performance

- Use `--gtest_filter` to run specific tests during development
- Parallel test execution: `ctest -j$(nproc)`
- Profile tests that take too long with timing assertions

## Future Improvements

- Add visual regression testing with reference images
- Implement property-based testing for complex interactions
- Add performance benchmarking tests
- Integrate with coverage reporting tools
- Add tests for different OpenGL contexts and versions