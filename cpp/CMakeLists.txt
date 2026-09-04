# --- Reusable backend library: mbgl-slint ---
# The canonical headless MapLibre Native <-> Slint backend, packaged as a
# linkable target so downstream applications can consume it instead of
# recompiling these sources. Public dependencies and include directories
# propagate to anything that links it, so a consumer only needs:
#
#   target_link_libraries(my-app PRIVATE maplibre-native-slint::mbgl-slint)
#   slint_target_sources(my-app my.slint
#       LIBRARY_PATHS maplibre-native-slint=${maplibre-native-slint_SOURCE_DIR}/src)
#
add_library(mbgl-slint STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/slint_maplibre_headless.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/host_gl_context_guard.cpp
)
add_library(maplibre-native-slint::mbgl-slint ALIAS mbgl-slint)

target_include_directories(mbgl-slint PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/include
    ${PROJECT_BINARY_DIR}/vendor/maplibre-native/include
)

target_link_libraries(mbgl-slint PUBLIC
    Slint::Slint
    mbgl-core
    cpr::cpr
)

# HostGLContextGuard resolves EGL/GLX entry points through dlopen/dlsym so it
# adds no link-time dependency on a GL stack that may not be in use.
target_link_libraries(mbgl-slint PRIVATE ${CMAKE_DL_LIBS})

if(MLN_WITH_WEBGPU)
    target_compile_definitions(mbgl-slint PUBLIC MLN_WITH_WEBGPU)
    # WebGPU backend: link wgpu-native via the mbgl-vendor-wgpu target.
    if(TARGET mbgl-vendor-wgpu)
        target_link_libraries(mbgl-slint PUBLIC mbgl-vendor-wgpu)
    endif()
else()
    # Traditional OpenGL/Metal backend.
    target_include_directories(mbgl-slint PUBLIC ${GLES3_INCLUDE_DIRS})
    target_link_libraries(mbgl-slint PUBLIC
        ${GLES3_LIBRARIES}
        ${OPENGL_LIBRARIES}
        $<$<PLATFORM_ID:Darwin>:${METAL_FRAMEWORK}>
    )
endif()

if(WIN32)
    target_compile_definitions(mbgl-slint
        PUBLIC NOMINMAX _USE_MATH_DEFINES WIN32_LEAN_AND_MEAN)
    if(MSVC)
        target_compile_options(mbgl-slint PRIVATE /utf-8 /MP)
    endif()
    # libuv is required by the MapLibre Native backend on Windows.
    find_package(libuv CONFIG REQUIRED)
    if(TARGET libuv::uv_a)
        target_link_libraries(mbgl-slint PUBLIC libuv::uv_a)
    else()
        target_link_libraries(mbgl-slint PUBLIC libuv::uv)
    endif()
endif()

# --- Reference application (headless backend) ---
add_executable(maplibre-slint-example main.cpp)

# Associate the .slint UI. The reusable component surface in src/ is imported
# via the "@maplibre-native-slint" library alias (see map_window.slint).
if(COMMAND slint_target_sources)
    slint_target_sources(maplibre-slint-example
        ${CMAKE_CURRENT_SOURCE_DIR}/map_window.slint
        LIBRARY_PATHS maplibre-native-slint=${PROJECT_SOURCE_DIR}/src
    )
else()
    message(FATAL_ERROR "slint_target_sources command is not available. Ensure Slint's CMake integration is present.")
endif()

# Only the generated Slint header (map_window.h) is local to this target;
# everything else propagates from mbgl-slint.
target_include_directories(maplibre-slint-example PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(maplibre-slint-example PRIVATE maplibre-native-slint::mbgl-slint)

# On Windows, copy the Slint DLL next to the application binary so that it's found
if (WIN32)
    add_custom_command(TARGET maplibre-slint-example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:maplibre-slint-example> $<TARGET_FILE_DIR:maplibre-slint-example>
        COMMAND_EXPAND_LISTS)

    # Copy wgpu_native.dll when using the WebGPU backend
    if(MLN_WITH_WEBGPU AND DEFINED WGPU_LIBRARY)
        string(REPLACE ".lib" ".dll" _wgpu_dll "${WGPU_LIBRARY}")
        if(EXISTS "${_wgpu_dll}")
            add_custom_command(TARGET maplibre-slint-example POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_wgpu_dll}"
                    "$<TARGET_FILE_DIR:maplibre-slint-example>"
                COMMENT "Copying wgpu_native.dll to output directory"
            )
        endif()
    endif()
endif()



# --- Zero-copy OpenGL example (maplibre-slint-gl) ---
# Renders MapLibre Native into an FBO in Slint's GL context (no readback).
# Requires the OpenGL backend (mbgl::gl::RendererBackend) and is Linux-only
# (EGL/GLES3 + the linuxkms backend at runtime), so it is skipped otherwise.
if(MLN_WITH_OPENGL AND NOT APPLE AND NOT WIN32)
    add_executable(maplibre-slint-gl
        main_gl.cpp
        src/slint_map_gl.cpp
        src/slint_gl_backend.cpp
    )

    # This target has its own Slint UI (Pi layout); generates gl_map_window.h.
    if (COMMAND slint_target_sources)
        slint_target_sources(maplibre-slint-gl
            ${CMAKE_CURRENT_SOURCE_DIR}/gl_map_window.slint
        )
    else()
        message(FATAL_ERROR "slint_target_sources command is not available.")
    endif()

    target_include_directories(maplibre-slint-gl
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/include
            ${CMAKE_BINARY_DIR}/vendor/maplibre-native/include
            ${CMAKE_CURRENT_BINARY_DIR}
            ${GLES3_INCLUDE_DIRS}
    )

    # mbgl-core is built with -fno-rtti (MLN_WITH_RTTI=OFF); match it so the
    # SlintGLBackend subclass does not require base-class typeinfo symbols.
    target_compile_options(maplibre-slint-gl PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)

    find_library(SLINT_GL_EGL_LIB EGL)

    target_link_libraries(maplibre-slint-gl
        PRIVATE
            Slint::Slint
            mbgl-core
            cpr::cpr
            ${GLES3_LIBRARIES}
            ${OPENGL_LIBRARIES}
    )

    if (SLINT_GL_EGL_LIB)
        target_link_libraries(maplibre-slint-gl PRIVATE ${SLINT_GL_EGL_LIB})
    else()
        target_link_libraries(maplibre-slint-gl PRIVATE EGL)
    endif()
else()
    message(STATUS "maplibre-slint-gl: skipped (requires MLN_WITH_OPENGL on Linux)")
endif()
