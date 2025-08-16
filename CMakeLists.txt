cmake_minimum_required(VERSION 3.21)
project(maplibre-native-slint LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Make FetchContent available early (keep for other fetches if needed)
include(FetchContent)

# Prevent subprojects from enabling their tests (avoid CMP0002 / duplicate test targets)
# Save top-level BUILD_TESTING (if defined) and force subprojects to not enable tests.
if(NOT DEFINED _MAPLIBRE_SAVE_BUILD_TESTING)
  set(_MAPLIBRE_SAVE_BUILD_TESTING "${BUILD_TESTING}" CACHE INTERNAL "Saved top-level BUILD_TESTING")
endif()
# Force subprojects to not create their own test targets during FetchContent/AddSubdirectory.
set(BUILD_TESTING OFF CACHE BOOL "Disable building tests for subprojects" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "Disable building tests for subprojects" FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "Disable building tests for subprojects" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "Disable building tests for subprojects" FORCE)
# (その他、サブプロジェクトが使いそうなフラグをここで OFF にしておく)

# --- IMPORTANT ---
# Do NOT FetchContent mapbox/geometry or mapbox/variant here unless you are absolutely
# sure about version compatibility. Prefer using the vendored headers inside
# vendor/maplibre-native to avoid API / duplicate-target conflicts.

# Add MapLibre (will use vendored dependencies inside vendor/maplibre-native)
add_subdirectory(vendor/maplibre-native)

# (If you previously saved BUILD_TESTING, restore it now so the rest of your top-level
# project can enable tests again if you want)
if(DEFINED _MAPLIBRE_SAVE_BUILD_TESTING)
  set(BUILD_TESTING ${_MAPLIBRE_SAVE_BUILD_TESTING} CACHE BOOL "Restore top-level BUILD_TESTING" FORCE)
  unset(_MAPLIBRE_SAVE_BUILD_TESTING CACHE)
endif()


#
# Slint: try to find it, otherwise fetch & build it via FetchContent
#
find_package(Slint QUIET)

if (NOT TARGET Slint::Slint)
  message(STATUS "Slint CMake target not found. Downloading Slint (api/cpp) via FetchContent and building locally.")
  # show progress for debugging if needed
  set(FETCHCONTENT_QUIET FALSE)

  FetchContent_Declare(
    Slint
    GIT_REPOSITORY https://github.com/slint-ui/slint.git
    GIT_TAG release/1
    SOURCE_SUBDIR api/cpp
  )

  FetchContent_MakeAvailable(Slint)
endif()

# Ensure FetchContent actually populated the Slint content if we expected it
# (FetchContent_GetProperties produces Slint_POPULATED when Slint was declared as above)
FetchContent_GetProperties(Slint)
if(NOT Slint_POPULATED AND NOT TARGET Slint::Slint)
    message(FATAL_ERROR "Slint was not populated and Slint::Slint target is not available.")
endif()

# make CPR available before any target that links to cpr::cpr
# prefer vendored copy if present, otherwise fall back to find_package
set(CPR_USE_SYSTEM_CURL ON CACHE BOOL "Use system-provided curl" FORCE)

if (NOT TARGET cpr::cpr)
  if (EXISTS "${CMAKE_SOURCE_DIR}/vendor/cpr/CMakeLists.txt")
    message(STATUS "Adding vendored CPR via add_subdirectory(vendor/cpr)")
    add_subdirectory(vendor/cpr)            # will create cpr::cpr
  else()
    message(STATUS "Trying find_package(cpr REQUIRED)")
    find_package(cpr REQUIRED)              # requires cpr installed on system
  endif()
endif()

# --- executable ---
add_executable(maplibre-slint-example
    examples/main.cpp
    src/slint_maplibre.cpp
    platform/custom_file_source.cpp
    platform/custom_run_loop.cpp
)

# Associate .slint files with the target (only if the helper is available)
if (COMMAND slint_target_sources)
    slint_target_sources(maplibre-slint-example
        examples/map_window.slint
    )
else()
    message(FATAL_ERROR "slint_target_sources command is not available. Ensure Slint's CMake integration is present.")
endif()

# Find system deps
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLES3 REQUIRED glesv2)

# Include dirs for this project (Slint::Slint will propagate its own include dirs)
target_include_directories(maplibre-slint-example
    PRIVATE
        src
        platform
        ${CMAKE_SOURCE_DIR}/vendor/maplibre-native/include
        ${CMAKE_BINARY_DIR}/vendor/maplibre-native/include
        ${CMAKE_CURRENT_BINARY_DIR}
        ${GLES3_INCLUDE_DIRS}
)

# Link libraries. Linking Slint::Slint will also propagate include dirs and other usage requirements.
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
