cmake_minimum_required(VERSION 3.20)
project(mln-ffi-example CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MLN_FFI_TARGET "linux-x64-egl" CACHE STRING
    "Prebuilt artifact to link against")
set_property(CACHE MLN_FFI_TARGET PROPERTY STRINGS
    "linux-x64-egl" "linux-x64-vulkan" "linux-arm64-egl" "linux-arm64-vulkan")

# Reuse a Slint that has already been built rather than building it again. The
# repository's own build tree has one under build/_deps once cpp/ has been
# configured; point this elsewhere to use a different one.
set(MLN_FFI_SLINT_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../build/_deps/slint-build"
    CACHE PATH "Directory holding libslint_cpp.so and slint-compiler")
set(MLN_FFI_SLINT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../build/_deps/slint-src"
    CACHE PATH "Slint source checkout, for its C++ API headers")

set(prefix "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/maplibre-native-c-${MLN_FFI_TARGET}")
if(NOT EXISTS "${prefix}/include/maplibre_native_c.h")
    message(FATAL_ERROR
        "No prebuilt artifact at ${prefix}.\n"
        "Run: MLN_FFI_TARGET=${MLN_FFI_TARGET} ffi/scripts/fetch-artifact.sh")
endif()

set(slint_compiler "${MLN_FFI_SLINT_BUILD_DIR}/slint-compiler")
set(slint_library "${MLN_FFI_SLINT_BUILD_DIR}/libslint_cpp.so")
foreach(needed "${slint_compiler}" "${slint_library}")
    if(NOT EXISTS "${needed}")
        message(FATAL_ERROR
            "Missing ${needed}.\n"
            "Configure the main project once so Slint is built, or set "
            "MLN_FFI_SLINT_BUILD_DIR to a tree that has it.")
    endif()
endforeach()

if(MLN_FFI_TARGET MATCHES "vulkan$")
    set(backend_source ../smoke/backend_vulkan.c)
else()
    set(backend_source ../smoke/backend_egl.c)
endif()

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/app.h"
    COMMAND "${slint_compiler}" "${CMAKE_CURRENT_SOURCE_DIR}/app.slint"
            -o "${CMAKE_CURRENT_BINARY_DIR}/app.h"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/app.slint"
    COMMENT "Compiling app.slint"
    VERBATIM)

add_executable(mln-ffi-example main.cpp ${backend_source} "${CMAKE_CURRENT_BINARY_DIR}/app.h")
target_include_directories(mln-ffi-example PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../smoke"
    "${prefix}/include"
    "${MLN_FFI_SLINT_SRC_DIR}/api/cpp/include"
    "${MLN_FFI_SLINT_BUILD_DIR}/generated_include"
)
target_link_libraries(mln-ffi-example PRIVATE
    "${prefix}/lib/libmaplibre-native-c.so"
    "${slint_library}"
)

if(MLN_FFI_TARGET MATCHES "vulkan$")
    find_package(Vulkan REQUIRED)
    target_link_libraries(mln-ffi-example PRIVATE Vulkan::Vulkan)
else()
    find_package(OpenGL REQUIRED COMPONENTS EGL)
    target_link_libraries(mln-ffi-example PRIVATE OpenGL::EGL)
endif()

set_target_properties(mln-ffi-example PROPERTIES
    BUILD_RPATH "${prefix}/lib;${MLN_FFI_SLINT_BUILD_DIR}"
    INSTALL_RPATH "${prefix}/lib;${MLN_FFI_SLINT_BUILD_DIR}"
)
message(STATUS "mln-ffi-example: ${MLN_FFI_TARGET} via ${backend_source}")
