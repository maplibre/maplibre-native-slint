cmake_minimum_required(VERSION 3.21)
project(maplibre-native-slint)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(vendor/maplibre-native)
add_subdirectory(vendor/cpr)

# Find Slint or download it automatically
find_package(Slint QUIET)
if (NOT Slint_FOUND)
  message("Slint could not be located in the CMake module search path. Downloading it from Git and building it locally")
  include(FetchContent)
  FetchContent_Declare(
    Slint
    GIT_REPOSITORY https://github.com/slint-ui/slint.git
    # `release/1` will auto-upgrade to the latest Slint >= 1.0.0 and < 2.0.0
    GIT_TAG release/1
    SOURCE_SUBDIR api/cpp
  )
  FetchContent_MakeAvailable(Slint)
endif (NOT Slint_FOUND)

add_library(map_window_ui)
slint_target_sources(map_window_ui examples/map_window.slint)
target_link_libraries(map_window_ui PUBLIC Slint::Slint)

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
        ${CMAKE_SOURCE_DIR}/vendor/maplibre-native/include
        ${CMAKE_BINARY_DIR}/vendor/maplibre-native/include
        ${CMAKE_CURRENT_BINARY_DIR}
        ${GLES3_INCLUDE_DIRS}
)

target_link_libraries(maplibre-native-slint
    PUBLIC
        Slint::Slint
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

# On Windows, copy the Slint DLL next to the application binary so that it's found
if (WIN32)
    add_custom_command(TARGET maplibre-slint-example POST_BUILD 
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:maplibre-slint-example> $<TARGET_FILE_DIR:maplibre-slint-example> 
        COMMAND_EXPAND_LISTS)
endif()

# Add testing support
option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
