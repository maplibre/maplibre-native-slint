cmake_minimum_required(VERSION 3.16)
project(maplibre-native-slint)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(vendor/maplibre-native)
add_subdirectory(vendor/cpr)

find_package(Slint REQUIRED)

add_library(map_window_ui)
SLINT_TARGET_SOURCES(map_window_ui examples/map_window.slint)
target_include_directories(map_window_ui PUBLIC /usr/local/include/slint)

add_library(maplibre-native-slint
    src/slint_maplibre.cpp
    platform/custom_file_source.cpp
    platform/custom_run_loop.cpp
)

find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLES3 REQUIRED glesv2)

target_include_directories(maplibre-native-slint
    PUBLIC
        src
        platform
        /usr/local/include/slint
        ${CMAKE_SOURCE_DIR}/vendor/maplibre-native/include
        ${CMAKE_BINARY_DIR}/vendor/maplibre-native/include
        ${CMAKE_CURRENT_BINARY_DIR}
        ${GLES3_INCLUDE_DIRS}
)

target_link_libraries(maplibre-native-slint
    PUBLIC
        Slint::slint_cpp
        mbgl-core
        cpr::cpr
        ${GLES3_LIBRARIES}
        ${OPENGL_LIBRARIES}
)

add_executable(maplibre-slint-example
    examples/main.cpp
)

target_link_libraries(maplibre-slint-example
    PRIVATE
        map_window_ui
        maplibre-native-slint
)


