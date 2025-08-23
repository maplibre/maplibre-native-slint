# Test configuration for maplibre-native-slint

# Use GoogleTest from MapLibre Native
find_package(Threads REQUIRED)

# Common sources to be tested
set(MAPLIBRE_SLINT_SOURCES
    ${CMAKE_SOURCE_DIR}/src/slint_maplibre.cpp
    ${CMAKE_SOURCE_DIR}/platform/custom_file_source.cpp
)

# Common include directories for tests
set(MAPLIBRE_SLINT_TEST_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/platform
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

# Simple unit tests (no OpenGL required)
add_executable(simple-unit-tests
    unit/simple_unit_test.cpp
    unit/test_main.cpp
    ${MAPLIBRE_SLINT_SOURCES}
)
target_include_directories(simple-unit-tests PRIVATE ${MAPLIBRE_SLINT_TEST_INCLUDE_DIRS})
target_link_libraries(simple-unit-tests PRIVATE ${MAPLIBRE_SLINT_TEST_LIBRARIES})

# Full unit tests (require OpenGL)
add_executable(maplibre-unit-tests
    unit/custom_file_source_test.cpp
    unit/simple_unit_test.cpp
    unit/test_main.cpp
    ${MAPLIBRE_SLINT_SOURCES}
)
target_include_directories(maplibre-unit-tests PRIVATE ${MAPLIBRE_SLINT_TEST_INCLUDE_DIRS})
target_link_libraries(maplibre-unit-tests PRIVATE ${MAPLIBRE_SLINT_TEST_LIBRARIES})

# Add test targets
add_test(NAME simple-unit-tests COMMAND simple-unit-tests)
add_test(NAME maplibre-unit-tests COMMAND maplibre-unit-tests)
