cmake_minimum_required(VERSION 3.21)
project(maplibre-native-slint)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(vendor/maplibre-native)

# Instruct CPR to use system-provided libraries BEFORE adding the subdirectory.
set(CPR_USE_SYSTEM_CURL ON CACHE BOOL "Use system-provided curl" FORCE)
set(CPR_USE_SYSTEM_LIB_PSL ON CACHE BOOL "Use system-provided libpsl" FORCE)
add_subdirectory(vendor/cpr)

# Find Slint or download it automatically
find_package(Slint QUIET)
if (NOT Slint_FOUND)
  message("Slint could not be located in the CMake module search path. Downloading it from Git and building it locally")
  include(FetchContent)
  FetchContent_Declare(
    Slint
    GIT_REPOSITORY https://github.com/slint-ui/slint.git
    GIT_TAG release/1
    SOURCE_SUBDIR api/cpp
  )
  FetchContent_MakeAvailable(Slint)
endif (NOT Slint_FOUND)

FetchContent_GetProperties(Slint)
if(NOT slint_POPULATED)
    message(FATAL_ERROR "Slint not populated")
endif()

add_executable(maplibre-slint-example
    examples/main.cpp
    src/slint_maplibre.cpp
    platform/custom_file_source.cpp
    platform/custom_run_loop.cpp
)

slint_target_sources(maplibre-slint-example
    examples/map_window.slint
)

find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLES3 REQUIRED glesv2)

target_include_directories(maplibre-slint-example
    PRIVATE
        src
        platform
        ${CMAKE_SOURCE_DIR}/vendor/maplibre-native/include
        ${CMAKE_BINARY_DIR}/vendor/maplibre-native/include
        ${CMAKE_CURRENT_BINARY_DIR}
        ${GLES3_INCLUDE_DIRS}
        ${slint_SOURCE_DIR}
)

target_link_libraries(maplibre-slint-example
    PRIVATE
        Slint::Slint
        mbgl-core
        cpr::cpr
        ${GLES3_LIBRARIES}
        ${OPENGL_LIBRARIES}
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
