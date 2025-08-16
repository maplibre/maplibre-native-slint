# Test configuration for maplibre-native-slint

# Use GoogleTest from MapLibre Native
find_package(Threads REQUIRED)

# Create test library
add_library(maplibre-slint-test STATIC
    test_utils.cpp
    test_utils.hpp
)

target_include_directories(maplibre-slint-test
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/platform
        ${CMAKE_SOURCE_DIR}/vendor/maplibre-native/include
        ${CMAKE_SOURCE_DIR}/vendor/maplibre-native/test/include
)

target_link_libraries(maplibre-slint-test
    PUBLIC
        mbgl-vendor-googletest
        maplibre-native-slint
        Threads::Threads
)

# Simple unit tests (no OpenGL required)
add_executable(simple-unit-tests
    unit/simple_unit_test.cpp
    unit/test_main.cpp
)

# Full unit tests (require OpenGL)
add_executable(maplibre-unit-tests
    unit/slint_maplibre_test.cpp
    unit/custom_file_source_test.cpp
    unit/custom_run_loop_test.cpp
    unit/test_main.cpp
)

target_link_libraries(simple-unit-tests
    PRIVATE
        maplibre-slint-test
)

target_link_libraries(maplibre-unit-tests
    PRIVATE
        maplibre-slint-test
)

# Integration tests
add_executable(integration-tests
    integration/map_rendering_test.cpp
    integration/slint_integration_test.cpp
    integration/test_main.cpp
)

target_link_libraries(integration-tests
    PRIVATE
        maplibre-slint-test
        map_window_ui
)

# Add test targets
add_test(NAME simple-unit-tests COMMAND simple-unit-tests)
add_test(NAME maplibre-unit-tests COMMAND maplibre-unit-tests)
add_test(NAME integration-tests COMMAND integration-tests)

# Enable testing
enable_testing()