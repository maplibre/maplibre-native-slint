# MapLibre Native + Slint Reference Implementation

This repository is a working reference for using [MapLibre Native](https://github.com/maplibre/maplibre-native) inside [Slint](https://slint.dev/) applications.

Its scope is running the two together from C++, and there are two ways to do that. The default renders on WebGPU and works on Linux, macOS and Windows. Where OpenGL is available, a second path hands Slint a borrowed GL texture instead, with no copy through host memory. Which one you can use is decided by what Slint accepts, described in [docs/rendering-paths.md](docs/rendering-paths.md). If you need a combination outside those two, reach for [maplibre-native-rs](https://github.com/maplibre/maplibre-native-rs) directly rather than bending this one.

The important thing here is not packaging polish. The important thing is that the combination actually works today across desktop platforms, with a reusable Slint component surface in [`src/`](src/).

## Supported Path and Experiments

Two directories are the repository:

- [`src/`](src/) is the reusable Slint component API.
- [`cpp/`](cpp/) is the backend that implements it, and the demo that uses it. It needs the MapLibre Native git submodule, so clone with `--recursive`.

Everything under [`experiments/`](experiments/) is an experiment. It is not supported, it may be broken at any moment, and it will be deleted if it leads nowhere:

- [`experiments/rust/`](experiments/rust/) reaches MapLibre through `maplibre-native-rs`. It is not a way to build this without C++: the crate builds MapLibre Native from source underneath.
- [`experiments/ffi/`](experiments/ffi/) explores the experimental C API from `maplibre-native-ffi`.

Use the path this repository supports rather than assembling your own beside it. In particular, do not stand up a GL or EGL context of your own next to the one Slint's renderer owns.

Contributors and coding agents should also read [AGENTS.md](AGENTS.md) and [AI_POLICY.md](AI_POLICY.md).

## What This Repository Is

- A reusable Slint component library centered on [`src/maplibre.slint`](src/maplibre.slint)
- A reusable C++ backend **library target** (`maplibre-native-slint::mbgl-slint`) you can link from your own CMake app
- A canonical C++ backend integration that works on Linux, Windows, and macOS
- A practical reference for people who want to build their own Slint + MapLibre app
- A place to validate backend choices such as WebGPU (`wgpu-native`) and Metal/OpenGL fallbacks

## What This Repository Is Not

- Not yet a polished end-user SDK
- Not yet an installable, versioned package (no `find_package` / system install yet). You consume it via `FetchContent` / `add_subdirectory`, see [Use It In Your Own App](#use-it-in-your-own-app)
- Not a "everything is magically wired for you" drop-in. You still write the small `MMapAdapter` wiring in your own `main` (see [`cpp/main.cpp`](cpp/main.cpp))

Today, the most honest way to describe this repository is:

> If you want to build a Slint application that embeds MapLibre, this repository shows a real cross-platform way to do it.

## Current Recommendation

If you want something that works today, use the C++ path as the reference implementation.

- The reusable Slint API lives in [`src/`](src/)
- The authoritative backend wiring lives in [`cpp/main.cpp`](cpp/main.cpp)
- The demo shell lives in [`cpp/map_window.slint`](cpp/map_window.slint)

The Rust demo exists to mirror the same Slint component contract, but it depends on [`maplibre-native-rs`](https://github.com/maplibre/maplibre-native-rs) and its current 0.8.x API surface. It is still only practical on Linux today. Treat it as an experimental companion, not the primary integration path.

## Quick Start

Platform-specific build guides:

- Linux: [Ubuntu 24.04 Build Guide](docs/build_guides/Linux_Ubuntu_24.md)
- Windows: [Windows 11 Build Guide](docs/build_guides/Windows_11.md)
- macOS: [macOS Apple Silicon Build Guide](docs/build_guides/macOS_Apple_Silicon.md)

Typical Linux build:

```bash
git clone https://github.com/maplibre/maplibre-native-slint.git
cd maplibre-native-slint
git submodule update --init --recursive

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

./build/cpp/maplibre-slint-example
```

The default build prefers the WebGPU backend with `wgpu-native` when available.

For Windows and macOS specifics, use the platform guides above.

## Reusable Slint Surface

The public Slint entrypoint is [`src/maplibre.slint`](src/maplibre.slint):

```slint
import { MMapView, MMapAdapter } from "@maplibre-native-slint/maplibre.slint";
```

The key exported symbols are:

- `MMapView`: the reusable visual map component
- `MMapAdapter`: the global bridge between the Slint UI and a native backend

Minimal UI usage looks like this:

```slint
import { MMapView } from "@maplibre-native-slint/maplibre.slint";

export component App inherits Window {
    preferred-width: 800px;
    preferred-height: 600px;

    map := MMapView {
        style-url: "https://demotiles.maplibre.org/style.json";
        center-lat: 35.6895;
        center-lon: 139.6917;
        zoom: 10;
    }
}
```

That is the reusable UI layer.

What still needs to be provided by the host application is the native backend wiring for `MMapAdapter`. The canonical example of that wiring is [`cpp/main.cpp`](cpp/main.cpp).

## Use It In Your Own App

The repository is consumable directly from another CMake project, with no system install needed. Fetch it and link the reusable backend target `maplibre-native-slint::mbgl-slint`, which publicly propagates MapLibre Native, Slint, cpr, the GL/WebGPU libraries, and the backend headers:

```cmake
include(FetchContent)
FetchContent_Declare(
  maplibre-native-slint
  GIT_REPOSITORY https://github.com/maplibre/maplibre-native-slint.git
  GIT_TAG <pin-a-commit>
)
FetchContent_MakeAvailable(maplibre-native-slint)

add_executable(my-app main.cpp)

# Import the reusable Slint components via the @maplibre-native-slint alias.
slint_target_sources(my-app my.slint
  LIBRARY_PATHS maplibre-native-slint=${maplibre-native-slint_SOURCE_DIR}/src)

target_link_libraries(my-app PRIVATE maplibre-native-slint::mbgl-slint)
```

Your `main.cpp` wires the Slint `MMapAdapter` callbacks to a `SlintMapLibre`
instance (from `slint_maplibre_headless.hpp`, provided by the target). Copy
[`cpp/main.cpp`](cpp/main.cpp) as the starting point.

### Backend selection

The default build uses WebGPU (`wgpu-native`). To use OpenGL instead, disable
WebGPU **and** select a backend explicitly. A bare `-DMLN_WITH_WEBGPU=OFF`
fails fast with a message telling you to pick one:

```bash
cmake -B build -DMLN_WITH_WEBGPU=OFF -DMLN_WITH_OPENGL=ON
```

### Slint provisioning

A system-installed Slint is used if found, otherwise Slint is built from source.
A system Slint built against a foreign Qt/ICU can bake its `RUNPATH` into your
binary and break portability, so force a self-contained build with:

```bash
cmake -B build -DMLN_SLINT_USE_SYSTEM=OFF
```

For a fully self-contained, no-Qt result (kiosk / embedded), combine it with the
winit + FemtoVG Slint backend:

```bash
cmake -B build \
  -DMLN_WITH_WEBGPU=OFF -DMLN_WITH_OPENGL=ON \
  -DMLN_SLINT_USE_SYSTEM=OFF \
  -DSLINT_FEATURE_BACKEND_QT=OFF \
  -DSLINT_FEATURE_BACKEND_WINIT=ON \
  -DSLINT_FEATURE_RENDERER_FEMTOVG=ON
```

## Architecture

### UI contract

- [`src/m-map-view.slint`](src/m-map-view.slint) defines the reusable map component
- [`src/m-map-adapter.slint`](src/m-map-adapter.slint) defines the backend bridge contract
- [`src/maplibre.slint`](src/maplibre.slint) is the public entrypoint

### Canonical backend

- [`cpp/src/slint_maplibre_headless.cpp`](cpp/src/slint_maplibre_headless.cpp) holds the current production-grade backend logic
- It is packaged as the `mbgl-slint` library target (alias `maplibre-native-slint::mbgl-slint`) so apps and tests link it instead of recompiling the sources
- [`cpp/main.cpp`](cpp/main.cpp) wires `MMapAdapter` to that backend
- [`cpp/map_window.slint`](cpp/map_window.slint) is a demo shell showing how to use the reusable component

### Experimental backend

- [`experiments/rust/main.slint`](experiments/rust/main.slint) mirrors the same Slint component contract as the C++ demo
- [`experiments/rust/src/maplibre.rs`](experiments/rust/src/maplibre.rs) wires `MMapAdapter` to `maplibre-native-rs`
- This path is useful for experimentation on Linux, but it is not the repository's primary story today

## Rendering Paths

How a MapLibre frame reaches a Slint surface, which of the two build targets to
use, what Slint will and will not accept as a texture, the platform status and
the build backend flags all live in
[docs/rendering-paths.md](docs/rendering-paths.md).

The short version: Slint accepts a borrowed texture only from OpenGL, so the
OpenGL path can hand its frame over directly while the WebGPU default copies
through host memory. That is a property of the toolkit, not a shortcut taken
here.

## Project Structure

- [`src/`](src/) - reusable Slint component API
- [`cpp/`](cpp/) - canonical C++ backend integration and demo app
- [`experiments/`](experiments/) - experiments, not part of the supported path:
  - [`experiments/rust/`](experiments/rust/) - Linux-oriented experimental Rust backend integration
  - [`experiments/ffi/`](experiments/ffi/) - exploration of the experimental C API from `maplibre-native-ffi`
- [`vendor/`](vendor/) - MapLibre Native and other vendored dependencies
- [`docs/build_guides/`](docs/build_guides/) - platform-specific build guides
- [`docs/testing.md`](docs/testing.md) - testing instructions

## Testing

Relevant test/documentation entrypoints:

- [Testing Guide](docs/testing.md)
- [Testing Overview](docs/testing_overview.md)

For the Rust backend specifically:

- use Rust 1.90 or newer (`maplibre-native-rs` 0.8.x requires it)
- on Linux, the default backend is OpenGL and `cargo test` builds `maplibre-native` from source through `maplibre-native-rs`
- renderer integration tests are opt-in via `MAPLIBRE_NATIVE_SLINT_RUN_RENDERER_TESTS=1` so CI can stay headless by default

For day-to-day validation, the most important checks are:

- the C++ demo builds and launches on Linux, Windows, and macOS
- the reusable Slint contract in [`src/`](src/) stays compatible with both demo shells
- the Rust demo remains aligned with the same `MMapView` / `MMapAdapter` contract on Linux

## Roadmap

Near-term goals:

- Keep the reusable Slint API in [`src/`](src/) and the `mbgl-slint` target stable enough for direct consumption
- Keep the C++ backend as the authoritative cross-platform reference

Longer-term possibilities:

- an installable/exported package (`find_package(maplibre-native-slint)`) so downstream apps do not need `FetchContent`
- better packaging so users do not need to think about the C++ toolchain
- a lower-overhead rendering path that avoids GPU-to-CPU readback

These are goals, not promises. The current value of this repository is that it already demonstrates a real working integration.

## Troubleshooting

### Build issues

- Run `git submodule update --init --recursive`
- Follow the platform-specific build guide for your OS
- On Linux and Windows with WebGPU, make sure LLVM/libclang is available for `bindgen`

### Runtime issues

- Make sure your machine has network access for style and tile loading
- On Linux, ensure a graphical session is available if you are launching the desktop demo directly

## Community

- MapLibre Native Slack: [#maplibre-native](https://osmus.slack.com/archives/C01G4G39862)
- OSM US Slack invite: [slack.openstreetmap.us](https://slack.openstreetmap.us/)
- MapLibre website: [maplibre.org](https://maplibre.org/)

## License

Copyright (c) 2025 MapLibre contributors.

This project is licensed under the BSD 2-Clause License. See [LICENSE](LICENSE).

This repository integrates multiple components with their own licenses:

- MapLibre Native: BSD
- Slint: GPL-3.0-only OR LicenseRef-Slint-Royalty-free-2.0 OR LicenseRef-Slint-Software-3.0
