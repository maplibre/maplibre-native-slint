cmake_minimum_required(VERSION 3.21)
if(APPLE)
  project(maplibre-native-slint LANGUAGES C CXX OBJC OBJCXX Swift)
else()
  project(maplibre-native-slint LANGUAGES C CXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add testing support (default OFF)
option(BUILD_TESTS "Build tests" OFF)
option(MLNS_WITH_SLINT_GL "Enable Slint OpenGL zero-copy path" ON)
# Force-fetch Slint from source to control enabled renderers (avoids picking an installed Slint without Skia)
option(MLNS_FORCE_FETCH_SLINT "Fetch/build Slint from source instead of find_package" ON)

# Make FetchContent available early (keep for other fetches if needed)
include(FetchContent)
# Explicitly enable Objective-C / ObjC++ / Swift so that Xcode does not treat .m as C
if(APPLE)
  enable_language(C)
  enable_language(OBJC)
  enable_language(OBJCXX)
  enable_language(Swift)
else()
  enable_language(C)
endif()

# Prevent subprojects from enabling their tests (avoid CMP0002 / duplicate test targets)
# Save top-level BUILD_TESTING (if defined) and force subprojects to not enable tests.
if(NOT DEFINED _MAPLIBRE_SAVE_BUILD_TESTING)
  set(_MAPLIBRE_SAVE_BUILD_TESTING "${BUILD_TESTING}" CACHE INTERNAL "Saved top-level BUILD_TESTING")
endif()
if(NOT DEFINED _MAPLIBRE_SAVE_BUILD_TESTS)
  set(_MAPLIBRE_SAVE_BUILD_TESTS "${BUILD_TESTS}" CACHE INTERNAL "Saved top-level BUILD_TESTS")
endif()
# Force subprojects to not create their own test targets during FetchContent/AddSubdirectory.
set(BUILD_TESTING OFF CACHE BOOL "Disable building tests for subprojects" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "Disable building tests for subprojects" FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "Disable building tests for subprojects" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "Disable building tests for subprojects" FORCE)
# (Also turn OFF other flags that subprojects might enable here)

# --- IMPORTANT ---
# Do NOT FetchContent mapbox/geometry or mapbox/variant here unless you are absolutely
# sure about version compatibility. Prefer using the vendored headers inside
# vendor/maplibre-native to avoid API / duplicate-target conflicts.

# Ensure macOS builds target Apple Silicon only and do so before adding subprojects
if(APPLE)
  # Build only arm64 to avoid accidental x86_64 slices from Xcode's default "Standard Architectures"
  set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures for macOS" FORCE)
  # Match the deployment target used elsewhere, but set early so subprojects inherit it
  set(CMAKE_OSX_DEPLOYMENT_TARGET "14.3" CACHE STRING "" FORCE)
  # For Xcode: explicitly exclude x86_64 to prevent multi-arch attempts
  set(CMAKE_XCODE_ATTRIBUTE_EXCLUDED_ARCHS[sdk=macosx*] "x86_64")
endif()

# Disable MapLibre Native tests/benches/render-tests in the vendored tree
set(MLN_WITH_TESTS OFF CACHE BOOL "Disable MapLibre tests" FORCE)
set(MLN_WITH_RENDER_TESTS OFF CACHE BOOL "Disable MapLibre render tests" FORCE)
set(MLN_WITH_BENCHMARKS OFF CACHE BOOL "Disable MapLibre benchmarks" FORCE)
# (Older CMake options used in some revisions; harmless if unknown)
set(MBGL_WITH_TESTS OFF CACHE BOOL "" FORCE)
set(MBGL_WITH_RENDER_TESTS OFF CACHE BOOL "" FORCE)
set(MBGL_WITH_BENCHMARKS OFF CACHE BOOL "" FORCE)

# Add MapLibre (will use vendored dependencies inside vendor/maplibre-native)
add_subdirectory(vendor/maplibre-native EXCLUDE_FROM_ALL)

# (If you previously saved BUILD_TESTING, restore it now so the rest of your top-level
# project can enable tests again if you want)
if(DEFINED _MAPLIBRE_SAVE_BUILD_TESTING)
  set(BUILD_TESTING ${_MAPLIBRE_SAVE_BUILD_TESTING} CACHE BOOL "Restore top-level BUILD_TESTING" FORCE)
  unset(_MAPLIBRE_SAVE_BUILD_TESTING CACHE)
endif()
if(DEFINED _MAPLIBRE_SAVE_BUILD_TESTS)
  set(BUILD_TESTS ${_MAPLIBRE_SAVE_BUILD_TESTS} CACHE BOOL "Restore top-level BUILD_TESTS" FORCE)
  unset(_MAPLIBRE_SAVE_BUILD_TESTS CACHE)
endif()


#
# Slint: configure features (enable Skia/OpenGL on Windows for robust GPU)
# These must be set before FetchContent_MakeAvailable(Slint)
if (WIN32)
  set(SLINT_FEATURE_BACKEND_WINIT ON CACHE BOOL "" FORCE)
  set(SLINT_FEATURE_RENDERER_SKIA ON CACHE BOOL "" FORCE)
  set(SLINT_FEATURE_RENDERER_SKIA_OPENGL ON CACHE BOOL "" FORCE)
  # Keep FemtoVG available too (we might compare later)
  set(SLINT_FEATURE_RENDERER_FEMTOVG ON CACHE BOOL "" FORCE)
endif()

#
# Slint: prefer FetchContent so we can enable Skia/OpenGL features on Windows
#
if (MLNS_FORCE_FETCH_SLINT)
  message(STATUS "Using FetchContent for Slint (MLNS_FORCE_FETCH_SLINT=ON)")
  set(FETCHCONTENT_QUIET FALSE)
  FetchContent_Declare(
    Slint
    GIT_REPOSITORY https://github.com/slint-ui/slint.git
    GIT_TAG release/1
    SOURCE_SUBDIR api/cpp
  )
  FetchContent_MakeAvailable(Slint)
else()
  find_package(Slint REQUIRED)
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
    src/slint_maplibre_headless.cpp
    platform/custom_file_source.cpp
)

# Optional Slint OpenGL zero-copy library (scaffold). This does not change the
# default rendering path yet; it provides a target we can evolve without
# breaking the current build. The example links it when enabled so symbols are
# available, guarded by compile-time macros.
if(MLNS_WITH_SLINT_GL)
  add_library(slint_maplibre_gl STATIC
    src/slint_maplibre_gl.cpp
  )
  target_include_directories(slint_maplibre_gl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_compile_features(slint_maplibre_gl PUBLIC cxx_std_20)
  target_link_libraries(slint_maplibre_gl PUBLIC Slint::Slint mbgl-core ${OPENGL_LIBRARIES})
  target_compile_definitions(slint_maplibre_gl PUBLIC MLNS_WITH_SLINT_GL=1)
  if (WIN32)
    target_compile_definitions(slint_maplibre_gl PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN _USE_MATH_DEFINES)
  endif()

  target_link_libraries(maplibre-slint-example PRIVATE slint_maplibre_gl)
  target_compile_definitions(maplibre-slint-example PRIVATE MLNS_WITH_SLINT_GL=1)
endif()

if (WIN32)
  target_compile_definitions(maplibre-slint-example
    PRIVATE NOMINMAX _USE_MATH_DEFINES WIN32_LEAN_AND_MEAN)

  if (MSVC)
    target_compile_options(maplibre-slint-example PRIVATE /utf-8 /MP)
  endif()

  find_package(libuv CONFIG REQUIRED)

  if (TARGET libuv::uv_a)
    set(ML_UV_TARGET libuv::uv_a)
  else()
    set(ML_UV_TARGET libuv::uv)
  endif()

  target_link_libraries(maplibre-slint-example PRIVATE ${ML_UV_TARGET})
endif()

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

# Platform-specific handling for GLES vs OpenGL
if(APPLE)
    # macOS specific settings
    set(CMAKE_OSX_ARCHITECTURES "arm64")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.3")

    # Find Metal framework for MapLibre (optional)
    find_library(METAL_FRAMEWORK Metal)

    # Use desktop OpenGL on macOS; no GLES lookup
    set(GLES3_FOUND TRUE)
    set(GLES3_INCLUDE_DIRS "")
    set(GLES3_LIBRARIES "")
elseif(WIN32)
    # On Windows we use desktop OpenGL (opengl32); skip pkg-config glesv2 entirely
    set(GLES3_FOUND TRUE)
    set(GLES3_INCLUDE_DIRS "")
    set(GLES3_LIBRARIES "")
else()
    # Linux and others: try GLES via pkg-config, otherwise fall back to OpenGL
    pkg_check_modules(GLES3 glesv2)
    if(NOT GLES3_FOUND)
        message(STATUS "GLESv2 not found, falling back to desktop OpenGL")
        set(GLES3_FOUND TRUE)
        set(GLES3_INCLUDE_DIRS "")
        set(GLES3_LIBRARIES "")
    endif()
endif()

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
        $<$<PLATFORM_ID:Darwin>:${METAL_FRAMEWORK}>
)

# On Windows, copy the Slint DLL next to the application binary so that it's found
if (WIN32)
    add_custom_command(TARGET maplibre-slint-example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:maplibre-slint-example> $<TARGET_FILE_DIR:maplibre-slint-example>
        COMMAND_EXPAND_LISTS)
endif()

# Add testing support
enable_testing()

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
