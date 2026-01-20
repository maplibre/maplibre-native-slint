add_executable(maplibre-slint-example
    main.cpp
    src/slint_maplibre_headless.cpp
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
        ${CMAKE_CURRENT_SOURCE_DIR}/map_window.slint
    )
else()
    message(FATAL_ERROR "slint_target_sources command is not available. Ensure Slint's CMake integration is present.")
endif()

# Include dirs for this project (Slint::Slint will propagate its own include dirs)
target_include_directories(maplibre-slint-example
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/platform
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
