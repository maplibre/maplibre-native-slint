cmake_minimum_required(VERSION 3.21)

# Check if the generator supports Swift
if(CMAKE_GENERATOR MATCHES "Unix Makefiles|Ninja" AND APPLE)
  # For Unix Makefiles and Ninja on macOS, don't enable Swift
  project(maplibre-native-slint LANGUAGES C CXX OBJC OBJCXX)
  message(STATUS "Using ${CMAKE_GENERATOR} generator - Swift language disabled")
elseif(APPLE)
  project(maplibre-native-slint LANGUAGES C CXX OBJC OBJCXX Swift)
else()
  project(maplibre-native-slint LANGUAGES C CXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enforce static libraries to simplify runtime distribution (avoid missing dylibs like libcpr)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

# Add testing support (default OFF)
option(BUILD_TESTS "Build tests" OFF)

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

set(MLN_WITH_RTTI ON CACHE BOOL "Enable RTTI for MapLibre" FORCE)
# (Older CMake options used in some revisions; harmless if unknown)
set(MBGL_WITH_TESTS OFF CACHE BOOL "" FORCE)
set(MBGL_WITH_RENDER_TESTS OFF CACHE BOOL "" FORCE)
set(MBGL_WITH_BENCHMARKS OFF CACHE BOOL "" FORCE)

set(SLINT_MAPLIBRE_USE_METAL ON CACHE BOOL "Metal zero-copy backend always enabled" FORCE)
if(APPLE)
  message(STATUS "Metal zero-copy backend enabled (CPU fallback removed)")
endif()

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
# Slint: try to find it, otherwise fetch & build it via FetchContent
#
find_package(Slint QUIET)

if (NOT TARGET Slint::Slint)
  message(STATUS "Slint CMake target not found. Downloading Slint (api/cpp) via FetchContent and building locally.")
  # show progress for debugging if needed
  set(FETCHCONTENT_QUIET FALSE)

  # Configure Slint backend based on our Metal-only setting
  if(APPLE AND BUILD_METAL_ONLY)
    message(STATUS "Configuring Slint for Metal-only: disabling OpenGL backend")
    set(SLINT_FEATURE_BACKEND_GL OFF CACHE BOOL "Disable OpenGL backend for Metal-only builds" FORCE)
    # Keep winit backend enabled - it's needed for window creation even with Metal rendering
    set(SLINT_FEATURE_BACKEND_WINIT ON CACHE BOOL "Enable winit backend for window creation" FORCE)
    set(SLINT_FEATURE_RENDERER_FEMTOVG ON CACHE BOOL "Enable femtovg renderer" FORCE)
  endif()

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

  $<$<AND:$<PLATFORM_ID:Darwin>,$<BOOL:${MLN_WITH_METAL}>>:examples/main.mm>
  $<$<NOT:$<AND:$<PLATFORM_ID:Darwin>,$<BOOL:${MLN_WITH_METAL}>>>:examples/main.cpp>
  $<$<NOT:$<AND:$<PLATFORM_ID:Darwin>,$<BOOL:${MLN_WITH_METAL}>>>:src/slint_maplibre_headless.cpp>
  # Legacy CPU fallback (slint_maplibre.cpp) intentionally excluded for Metal zero-copy path
  platform/custom_file_source.cpp

)

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

option(BUILD_METAL_ONLY "On Apple, build only Metal backend and skip OpenGL/GLES lookups" ON)

# Find system deps (OpenGL always needed on macOS for Slint compatibility)
if(APPLE)
  find_package(OpenGL REQUIRED)
  message(STATUS "macOS: OpenGL required for Slint compatibility")
elseif(BUILD_METAL_ONLY)
  message(STATUS "Metal-only build: skipping OpenGL/GLES detection")
else()
  find_package(OpenGL REQUIRED)
  find_package(PkgConfig REQUIRED)
endif()

# Check if we're on macOS and handle OpenGL ES differently
if(APPLE)
  # macOS specific settings
  set(CMAKE_OSX_ARCHITECTURES "arm64")
  set(CMAKE_OSX_DEPLOYMENT_TARGET "14.3")

  # Find Metal frameworks for MapLibre
  find_library(METAL_FRAMEWORK Metal)
  find_library(APPKIT_FRAMEWORK AppKit)
  find_library(QUARTZCORE_FRAMEWORK QuartzCore)

  # Skip GLES/OpenGL libs in Metal-only mode
  if(BUILD_METAL_ONLY)
    set(GLES3_FOUND FALSE)
    set(GLES3_INCLUDE_DIRS "")
    set(GLES3_LIBRARIES "")
  else()
    # Provide empty GLES variables for compatibility if not found
    set(GLES3_FOUND TRUE)
    set(GLES3_INCLUDE_DIRS "")
    set(GLES3_LIBRARIES "")
  endif()
else()
  # Non-Apple: attempt GLES first (if PkgConfig was found)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GLES3 glesv2)
  if(NOT GLES3_FOUND)
    message(WARNING "GLES2 not found, falling back to OpenGL")
    set(GLES3_FOUND TRUE)
    set(GLES3_INCLUDE_DIRS "")
    set(GLES3_LIBRARIES "")
  endif()
endif()

# Include dirs for this project (Slint::Slint will propagate its own include dirs)
target_include_directories(maplibre-slint-example PRIVATE
  src
  src/metal
  platform
  ${CMAKE_SOURCE_DIR}/vendor/maplibre-native/include
  ${CMAKE_BINARY_DIR}/vendor/maplibre-native/include
  ${CMAKE_SOURCE_DIR}/vendor/maplibre-native/src
  ${CMAKE_SOURCE_DIR}/vendor/maplibre-native/vendor/metal-cpp
  ${CMAKE_CURRENT_BINARY_DIR}
  ${GLES3_INCLUDE_DIRS}
)

# Link libraries. Linking Slint::Slint will also propagate include dirs and other usage requirements.
target_link_libraries(maplibre-slint-example
  PRIVATE
    Slint::Slint
    mbgl-core
    cpr::cpr
    $<$<BOOL:${GLES3_LIBRARIES}>:${GLES3_LIBRARIES}>
    $<$<NOT:$<AND:$<PLATFORM_ID:Darwin>,$<BOOL:${BUILD_METAL_ONLY}>>>:${OPENGL_LIBRARIES}>
    $<$<PLATFORM_ID:Darwin>:${OPENGL_LIBRARIES}>
    $<$<PLATFORM_ID:Darwin>:${METAL_FRAMEWORK}>
    $<$<PLATFORM_ID:Darwin>:${APPKIT_FRAMEWORK}>
    $<$<PLATFORM_ID:Darwin>:${QUARTZCORE_FRAMEWORK}>
)

if(APPLE)
  # Ensure dependent dylibs (like cpr) are discoverable at runtime from build tree
  set_target_properties(maplibre-slint-example PROPERTIES
      BUILD_RPATH "@loader_path;@loader_path/../vendor/cpr/cpr/Debug;@executable_path"
      INSTALL_RPATH "@loader_path;@executable_path/../lib")
endif()

if(APPLE)
  message(STATUS "Building with Metal zero-copy backend sources")
  set(METAL_SOURCES
    src/metal/slint_metal_maplibre.mm
    src/metal/slint_metal_renderer_backend.mm
  src/metal/metal_cpp_symbols.mm
  )
  foreach(src ${METAL_SOURCES})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
      target_sources(maplibre-slint-example PRIVATE ${src})
      set_source_files_properties(${src} PROPERTIES COMPILE_FLAGS "-fobjc-arc -frtti")
    endif()
  endforeach()
  target_compile_definitions(maplibre-slint-example PRIVATE SLINT_MAPLIBRE_USE_METAL=1)
  if(BUILD_METAL_ONLY)
    target_compile_definitions(maplibre-slint-example PRIVATE SLINT_METAL_ONLY=1)
  endif()
  # Explicit macOS frameworks needed for winit keyboard APIs (Carbon/HIToolbox) & TIS functions
  target_link_libraries(maplibre-slint-example PRIVATE "-framework Carbon" "-framework ApplicationServices")
endif()

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

# Convenience target to build and run the Metal example (macOS only)
if(APPLE)
  add_custom_target(run-macos-metal
    COMMAND "$<TARGET_FILE:maplibre-slint-example>"
    DEPENDS maplibre-slint-example
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running Metal zero-copy MapLibre + Slint example")
endif()
