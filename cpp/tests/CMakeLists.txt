# Test configuration for maplibre-native-slint

# Use GoogleTest from MapLibre Native
find_package(Threads REQUIRED)

# The backend under test comes from the reusable mbgl-slint library, which
# propagates Slint / mbgl-core / GL include dirs and link deps.
add_executable(unit-tests
    unit/slint_maplibre_headless_test.cpp
    unit/integration_test.cpp
    unit/test_main.cpp
)

target_include_directories(unit-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/vendor/maplibre-native/test/include
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(unit-tests PRIVATE
    maplibre-native-slint::mbgl-slint
    mbgl-vendor-googletest
    Threads::Threads
)

# HostGLContextGuard's contract is testable without a window: bring up an
# off-screen EGL context, unbind it inside a guard's scope and assert it comes
# back. That needs EGL headers and a real libEGL to link against, so the case
# is compiled only where those exist (the guard itself is a no-op on Apple
# platforms and uses WGL on Windows).
if(NOT WIN32 AND NOT APPLE)
    find_package(OpenGL QUIET COMPONENTS EGL)
    find_library(MBGL_SLINT_TEST_EGL_LIB NAMES EGL)
    if(MBGL_SLINT_TEST_EGL_LIB)
        target_sources(unit-tests PRIVATE unit/host_gl_context_guard_test.cpp)
        target_link_libraries(unit-tests PRIVATE ${MBGL_SLINT_TEST_EGL_LIB})
    else()
        message(STATUS
            "unit-tests: host_gl_context_guard_test skipped (libEGL not found)")
    endif()
endif()

# Add test targets
add_test(NAME unit-tests COMMAND unit-tests)
