cmake_minimum_required(VERSION 3.20)
project(mln-ffi-smoke C)

# maplibre-native-ffi ships one prebuilt per graphics backend. Pick the target
# and the matching host-side setup comes with it.
set(MLN_FFI_TARGET "linux-x64-egl" CACHE STRING
    "Prebuilt artifact to link against")
set_property(CACHE MLN_FFI_TARGET PROPERTY STRINGS
    "linux-x64-egl" "linux-x64-vulkan" "linux-arm64-egl" "linux-arm64-vulkan")

set(prefix "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/maplibre-native-c-${MLN_FFI_TARGET}")
if(NOT EXISTS "${prefix}/include/maplibre_native_c.h")
    message(FATAL_ERROR
        "No prebuilt artifact at ${prefix}.\n"
        "Run: MLN_FFI_TARGET=${MLN_FFI_TARGET} ffi/scripts/fetch-artifact.sh")
endif()

if(MLN_FFI_TARGET MATCHES "vulkan$")
    set(backend_source backend_vulkan.c)
elseif(MLN_FFI_TARGET MATCHES "egl$")
    set(backend_source backend_egl.c)
else()
    message(FATAL_ERROR "No host-side setup for target ${MLN_FFI_TARGET}")
endif()

add_executable(mln-ffi-smoke smoke.c ${backend_source})
target_include_directories(mln-ffi-smoke PRIVATE "${prefix}/include")
target_link_libraries(mln-ffi-smoke PRIVATE "${prefix}/lib/libmaplibre-native-c.so")

if(MLN_FFI_TARGET MATCHES "vulkan$")
    find_package(Vulkan REQUIRED)
    target_link_libraries(mln-ffi-smoke PRIVATE Vulkan::Vulkan)
else()
    find_package(OpenGL REQUIRED COMPONENTS EGL)
    target_link_libraries(mln-ffi-smoke PRIVATE OpenGL::EGL)
endif()

set_target_properties(mln-ffi-smoke PROPERTIES
    BUILD_RPATH "${prefix}/lib"
    INSTALL_RPATH "${prefix}/lib"
)
message(STATUS "mln-ffi-smoke: ${MLN_FFI_TARGET} via ${backend_source}")
