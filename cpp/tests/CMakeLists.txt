# Test configuration for maplibre-native-slint

# Use GoogleTest from MapLibre Native
find_package(Threads REQUIRED)

# Common sources to be tested
set(MAPLIBRE_SLINT_SOURCES
    ${CMAKE_SOURCE_DIR}/cpp/src/slint_maplibre_headless.cpp
    ${CMAKE_SOURCE_DIR}/cpp/platform/custom_file_source.cpp
)

# Common include directories for tests
set(MAPLIBRE_SLINT_TEST_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/cpp/src
    ${CMAKE_SOURCE_DIR}/cpp/platform
    ${CMAKE_SOURCE_DIR}/vendor/maplibre-native/include
    ${CMAKE_SOURCE_DIR}/vendor/maplibre-native/test/include
    ${CMAKE_BINARY_DIR}/vendor/maplibre-native/include
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Common libraries for tests
set(MAPLIBRE_SLINT_TEST_LIBRARIES
    mbgl-vendor-googletest
    Threads::Threads
    Slint::Slint
    mbgl-core
    cpr::cpr
    ${GLES3_LIBRARIES}
    ${OPENGL_LIBRARIES}
    $<$<PLATFORM_ID:Darwin>:${METAL_FRAMEWORK}>
)

# Unit tests (require OpenGL for some parts)
add_executable(unit-tests
    unit/custom_file_source_test.cpp
    unit/slint_maplibre_headless_test.cpp
    unit/integration_test.cpp
    unit/test_main.cpp
    ${MAPLIBRE_SLINT_SOURCES}
)
target_include_directories(unit-tests PRIVATE ${MAPLIBRE_SLINT_TEST_INCLUDE_DIRS})
target_link_libraries(unit-tests PRIVATE ${MAPLIBRE_SLINT_TEST_LIBRARIES})

# Add test targets
add_test(NAME unit-tests COMMAND unit-tests)
