# MapLibre Native Slint - C++ Implementation

This directory hosts the C++ version of the MapLibre Native + Slint example. It mirrors the Rust implementation under `rust/` but uses the C++ MapLibre API and the Slint C++ bindings.

## Build

From the repository root:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Run

```bash
./build/maplibre-slint-example
```

## Files

- `main.cpp` — application entry point and UI wiring
- `map_window.slint` — Slint UI definition that generates `map_window.h`
- `src/slint_maplibre_headless.*` — MapLibre headless integration and rendering
- `platform/custom_file_source.*` — optional HTTP file source using CPR
