cmake_minimum_required(VERSION 3.21)

# Check if the generator supports Swift on macOS
if(CMAKE_GENERATOR MATCHES "Unix Makefiles|Ninja" AND APPLE)
  project(maplibre-native-slint LANGUAGES C CXX OBJC OBJCXX)
  message(STATUS "Using ${CMAKE_GENERATOR} generator - Swift language disabled")
elseif(APPLE)
  project(maplibre-native-slint LANGUAGES C CXX OBJC OBJCXX Swift)
else()
  project(maplibre-native-slint LANGUAGES C CXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Prefer static libraries to simplify runtime distribution
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

# Add testing support (default OFF)
option(BUILD_TESTS "Build tests" OFF)
option(SLINT_MAPLIBRE_USE_METAL "Enable Metal zero-copy backend on Apple" OFF)
option(BUILD_METAL_ONLY "On Apple, skip GLES lookups and focus on Metal" ON)
option(MLNS_WITH_SLINT_GL "Enable Slint OpenGL zero-copy path" OFF)
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

# MapLibre options
set(MLN_WITH_RTTI ON CACHE BOOL "Enable RTTI for MapLibre" FORCE)
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
  # Prefer FemtoVG OpenGL on Windows to avoid D3D symbol requirements from skia-safe
  if (MLNS_WITH_SLINT_GL)
  # Enable FemtoVG OpenGL renderer for zero-copy GL path on Windows
  set(SLINT_FEATURE_RENDERER_FEMTOVG ON CACHE BOOL "" FORCE)
  set(SLINT_FEATURE_RENDERER_FEMTOVG_WGPU OFF CACHE BOOL "" FORCE)
  # Keep software renderer available as fallback but prefer GL at runtime
  set(SLINT_FEATURE_RENDERER_SOFTWARE ON CACHE BOOL "" FORCE)
    set(SLINT_FEATURE_RENDERER_SKIA OFF CACHE BOOL "" FORCE)
    set(SLINT_FEATURE_RENDERER_SKIA_OPENGL OFF CACHE BOOL "" FORCE)
  else()
  set(SLINT_FEATURE_RENDERER_FEMTOVG ON CACHE BOOL "" FORCE)
  set(SLINT_FEATURE_RENDERER_SOFTWARE ON CACHE BOOL "" FORCE)
  set(SLINT_FEATURE_RENDERER_FEMTOVG_WGPU OFF CACHE BOOL "" FORCE)
  endif()
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
# Choose entry based on Metal usage on Apple
set(MAPLIBRE_SLINT_EXE_SOURCES
    src/slint_maplibre_headless.cpp
    platform/custom_file_source.cpp)

if(APPLE AND SLINT_MAPLIBRE_USE_METAL)
  list(APPEND MAPLIBRE_SLINT_EXE_SOURCES examples/main.mm)
else()
  list(APPEND MAPLIBRE_SLINT_EXE_SOURCES examples/main.cpp)
endif()

add_executable(maplibre-slint-example ${MAPLIBRE_SLINT_EXE_SOURCES})

# Optional Slint OpenGL zero-copy library (scaffold). This does not change the
# default rendering path yet; it provides a target we can evolve without
# breaking the current build. The example links it when enabled so symbols are
# available, guarded by compile-time macros.
if(MLNS_WITH_SLINT_GL)
  add_library(slint_maplibre_gl STATIC
    src/gl/slint_gl_maplibre.cpp
    src/gl/slint_gl_renderer_backend.cpp
  )
  target_include_directories(slint_maplibre_gl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_compile_features(slint_maplibre_gl PUBLIC cxx_std_20)
  target_link_libraries(slint_maplibre_gl PUBLIC Slint::Slint mbgl-core ${OPENGL_LIBRARIES})
  target_compile_definitions(slint_maplibre_gl PUBLIC MLNS_WITH_SLINT_GL=1)
  if (WIN32)
    target_compile_definitions(slint_maplibre_gl PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN _USE_MATH_DEFINES)
  endif()

  # Link GL path into the example only when requested
  target_link_libraries(maplibre-slint-example PRIVATE slint_maplibre_gl)
  target_compile_definitions(maplibre-slint-example PRIVATE MLNS_WITH_SLINT_GL=1)

  if (WIN32)
    # Pre-check: require MSVC (Visual Studio 2022 toolset) and x64
    if (NOT MSVC)
      message(FATAL_ERROR "MLNS_WITH_SLINT_GL=ON requires MSVC on Windows")
    endif()
    if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
      message(FATAL_ERROR "MLNS_WITH_SLINT_GL=ON requires 64-bit build on Windows")
    endif()

    # Skia/OpenGL renderer (via Slint) and winit on Windows need a few extra SDK libs
    find_library(D3DCompiler_LIB d3dcompiler)
    find_library(D3D12_LIB d3d12)
    find_library(DXGI_LIB dxgi)
    find_library(DXGUID_LIB dxguid)
    find_library(Dwmapi_LIB dwmapi)
    find_library(UxTheme_LIB uxtheme)
    find_library(Imm32_LIB imm32)
    find_library(Comctl32_LIB comctl32)
    find_library(PropSys_LIB propsys)

    set(_win_extra_libs)
    foreach(lib IN ITEMS D3DCompiler_LIB D3D12_LIB DXGI_LIB DXGUID_LIB Dwmapi_LIB UxTheme_LIB Imm32_LIB Comctl32_LIB PropSys_LIB)
      if (${lib})
        list(APPEND _win_extra_libs ${${lib}})
      else()
        message(WARNING "Windows SDK library not found for GL path: ${lib}")
      endif()
    endforeach()

    target_link_libraries(maplibre-slint-example PRIVATE ${_win_extra_libs})

    # Only link Skia if the Skia renderer is enabled
    if (SLINT_FEATURE_RENDERER_SKIA)
      # Try vcpkg's exported Skia targets. Newer ports expose skia::skia via
      # skiaConfig.cmake; older ones expose unofficial::skia::skia. Support both.
      find_package(skia CONFIG QUIET)
      if (TARGET skia::skia)
        set(SKIA_TARGET skia::skia)
      else()
        find_package(unofficial-skia CONFIG REQUIRED)
        if (TARGET unofficial::skia::skia)
          set(SKIA_TARGET unofficial::skia::skia)
        else()
          message(FATAL_ERROR "Skia CMake target not found; expected skia::skia or unofficial::skia::skia")
        endif()
      endif()

      target_link_libraries(maplibre-slint-example PRIVATE ${SKIA_TARGET})
      if (TARGET slint_maplibre_gl)
        target_link_libraries(slint_maplibre_gl PRIVATE ${SKIA_TARGET})
      endif()

      # Skia text rendering via Slint typically needs shaper/paragraph/unicode.
      # These are split as separate module targets by vcpkg's unofficial-skia port.
      find_package(unofficial-skia CONFIG QUIET)
      set(_skia_optional_modules
        unofficial::skia::modules::skshaper
        unofficial::skia::modules::skparagraph
        unofficial::skia::modules::skunicode_icu
        unofficial::skia::modules::skunicode_core)
      foreach(mod ${_skia_optional_modules})
        if (TARGET ${mod})
          target_link_libraries(maplibre-slint-example PRIVATE ${mod})
          if (TARGET slint_maplibre_gl)
            target_link_libraries(slint_maplibre_gl PRIVATE ${mod})
          endif()
        endif()
      endforeach()

      # The Rust crate `skia-safe` exposes a C shim library named `skia-bindings.lib`
      # that provides symbols like C_SkCodec_delete, C_ParagraphBuilder_make, ...
      # Corrosion places it under the Cargo build tree. Discover and link it.
      if (EXISTS "${CMAKE_BINARY_DIR}/cargo")
        file(GLOB_RECURSE _skia_bindings_lib
          LIST_DIRECTORIES FALSE
          "${CMAKE_BINARY_DIR}/cargo/build/*/release/build/*/out/skia/skia-bindings.lib")
        list(LENGTH _skia_bindings_lib _skia_bindings_count)
        if (_skia_bindings_count GREATER 0)
          # If multiple, pick the newest by timestamp
          list(SORT _skia_bindings_lib ORDER DESCENDING)
          list(GET _skia_bindings_lib 0 _skia_bindings_selected)
          message(STATUS "Linking Skia C shim: ${_skia_bindings_selected}")
          target_link_libraries(maplibre-slint-example PRIVATE "${_skia_bindings_selected}")
          if (TARGET slint_maplibre_gl)
            target_link_libraries(slint_maplibre_gl PRIVATE "${_skia_bindings_selected}")
          endif()
        else()
          message(WARNING "skia-bindings.lib not found under cargo build tree; Skia C symbols may be unresolved")
        endif()
      endif()
    endif()
  endif()
endif()

# Metal zero-copy backend sources (Objective-C++)
if(APPLE AND SLINT_MAPLIBRE_USE_METAL)
  message(STATUS "Building with Metal zero-copy backend sources")
  target_compile_definitions(maplibre-slint-example PRIVATE SLINT_MAPLIBRE_USE_METAL=1)
  # Add Metal sources if present
  set(METAL_SOURCES
    src/metal/slint_metal_maplibre.mm
  )
  foreach(src ${METAL_SOURCES})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
      target_sources(maplibre-slint-example PRIVATE ${src})
      set_source_files_properties(${src} PROPERTIES COMPILE_FLAGS "-fobjc-arc")
    endif()
  endforeach()
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
if(APPLE)
  find_package(OpenGL REQUIRED)
elseif(NOT BUILD_METAL_ONLY)
  find_package(OpenGL REQUIRED)
  find_package(PkgConfig REQUIRED)
else()
  find_package(PkgConfig REQUIRED)
endif()

# Platform-specific handling for GLES vs OpenGL
if(APPLE)
    # macOS specific settings
    set(CMAKE_OSX_ARCHITECTURES "arm64")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.3")

    # Find Metal-related frameworks
    find_library(METAL_FRAMEWORK Metal)
    find_library(APPKIT_FRAMEWORK AppKit)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore)

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
        $<$<BOOL:${OPENGL_LIBRARIES}>:${OPENGL_LIBRARIES}>
        $<$<PLATFORM_ID:Darwin>:${METAL_FRAMEWORK}>
        $<$<PLATFORM_ID:Darwin>:${APPKIT_FRAMEWORK}>
        $<$<PLATFORM_ID:Darwin>:${QUARTZCORE_FRAMEWORK}>
)

if(APPLE)
  # Ensure dependent dylibs are discoverable at runtime from build tree
  set_target_properties(maplibre-slint-example PROPERTIES
      BUILD_RPATH "@loader_path;@executable_path"
      INSTALL_RPATH "@loader_path;@executable_path/../lib")
  # Explicit frameworks for keyboard APIs
  target_link_libraries(maplibre-slint-example PRIVATE "-framework Carbon" "-framework ApplicationServices")
endif()

# On Windows, copy the Slint DLL next to the application binary so that it's found
if (WIN32)
    # Skip copying runtime DLLs; this project prefers static linking via vcpkg.
    # When there are no runtime DLLs, the copy command would be empty and fail.
endif()

# Add testing support
enable_testing()

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Convenience target to run app (macOS)
if(APPLE)
  add_custom_target(run-macos-metal
    COMMAND "$<TARGET_FILE:maplibre-slint-example>"
    DEPENDS maplibre-slint-example
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running MapLibre + Slint example")
endif()
