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

# Add test targets
add_test(NAME unit-tests COMMAND unit-tests)
