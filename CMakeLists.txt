cmake_minimum_required(VERSION 3.21)
if(APPLE)
  project(maplibre-native-slint LANGUAGES C CXX OBJC OBJCXX Swift)
else()
  project(maplibre-native-slint LANGUAGES C CXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Platform-specific compiler warnings handling ---
# On Linux with modern GCC (13+), Boost Geometry triggers variable shadowing warnings.
# These warnings don't affect functionality but can break builds with -Werror.
# We suppress them here as a sensible default, but users can override if needed.
if(NOT APPLE AND NOT MSVC)
  # Check if the compiler supports -Wno-error=shadow
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag(-Wno-error=shadow COMPILER_SUPPORTS_NO_ERROR_SHADOW)
  if(COMPILER_SUPPORTS_NO_ERROR_SHADOW)
    # Add the flag only if not already set by the user
    if(NOT CMAKE_CXX_FLAGS MATCHES ".*Wno-error=shadow.*")
      string(APPEND CMAKE_CXX_FLAGS " -Wno-error=shadow")
    endif()
  endif()
endif()

# --- Linux-specific pkg-config path setup ---
# On Linux, ensure pkg-config can find system libraries like libuv and glfw3.
# This is especially important on Ubuntu 24.04 where these packages are installed
# in standard system directories but may not be in the default pkg-config path.
if(UNIX AND NOT APPLE)
  set(LINUX_PKGCONFIG_PATHS
    "/usr/lib/x86_64-linux-gnu/pkgconfig"
    "/usr/lib/pkgconfig"
    "/usr/share/pkgconfig"
  )
  foreach(PATH ${LINUX_PKGCONFIG_PATHS})
    if(EXISTS "${PATH}")
      list(APPEND CMAKE_PREFIX_PATH "${PATH}")
      # Also set environment variable for external tools
      # Handle case where PKG_CONFIG_PATH might be empty
      if(DEFINED ENV{PKG_CONFIG_PATH} AND NOT "$ENV{PKG_CONFIG_PATH}" STREQUAL "")
        set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${PATH}")
      else()
        set(ENV{PKG_CONFIG_PATH} "${PATH}")
      endif()
    endif()
  endforeach()
endif()

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

# Find system deps
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)

# --- Linux: Handle pkg-config search for dependencies ---
# On Linux, explicitly search for critical dependencies to provide better error messages
# if they are not found. This helps users understand what needs to be installed.
if(UNIX AND NOT APPLE)
  pkg_check_modules(LIBUV libuv QUIET)
  if(NOT LIBUV_FOUND)
    message(WARNING 
      "libuv not found via pkg-config.\n"
      "This may indicate the libuv development package is not installed.\n"
      "On Ubuntu 24.04, install it with: sudo apt install -y libuv1-dev\n"
      "Ensure PKG_CONFIG_PATH includes /usr/lib/x86_64-linux-gnu/pkgconfig\n"
    )
  endif()
  
  pkg_check_modules(GLFW glfw3 QUIET)
  if(NOT GLFW_FOUND)
    message(WARNING 
      "glfw3 not found via pkg-config.\n"
      "This may indicate the GLFW development package is not installed.\n"
      "On Ubuntu 24.04, install it with: sudo apt install -y libglfw3-dev\n"
      "Ensure PKG_CONFIG_PATH includes /usr/lib/x86_64-linux-gnu/pkgconfig\n"
    )
  endif()
endif()

# Check if we're on macOS and handle OpenGL ES differently
if(APPLE)
  # macOS specific settings
  set(CMAKE_OSX_ARCHITECTURES "arm64")
  set(CMAKE_OSX_DEPLOYMENT_TARGET "14.3")

  # Find Metal framework for MapLibre
  find_library(METAL_FRAMEWORK Metal)

  # On macOS, use OpenGL instead of GLES
  set(GLES3_FOUND TRUE)
  set(GLES3_INCLUDE_DIRS "")
  set(GLES3_LIBRARIES "")
else()
  # On other platforms, try to find GLES
  pkg_check_modules(GLES3 glesv2)
  if(NOT GLES3_FOUND)
    message(WARNING "GLES2 not found, falling back to OpenGL")
    set(GLES3_FOUND TRUE)
    set(GLES3_INCLUDE_DIRS "")
    set(GLES3_LIBRARIES "")
  endif()
endif()

# --- C++ example (lives in cpp/) ---
add_subdirectory(cpp)

# Add testing support
enable_testing()

if(BUILD_TESTS)
  add_subdirectory(cpp/tests)
endif()
