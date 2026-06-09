# MapLibre Native + Slint Reference Implementation

This repository is a working reference for using [MapLibre Native](https://github.com/maplibre/maplibre-native) inside [Slint](https://slint.dev/) applications.

The important thing here is not packaging polish. The important thing is that the combination actually works today across desktop platforms, with a reusable Slint component surface in [`src/`](src/).

## What This Repository Is

- A reusable Slint component library centered on [`src/maplibre.slint`](src/maplibre.slint)
- A canonical C++ backend integration that works on Linux, Windows, and macOS
- A practical reference for people who want to build their own Slint + MapLibre app
- A place to validate backend choices such as WebGPU (`wgpu-native`) and Metal/OpenGL fallbacks

## What This Repository Is Not

- Not yet a polished end-user SDK
- Not yet a versioned package with stable distribution guarantees
- Not yet a "just import it and everything is magically wired for you" solution

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

## Architecture

### UI contract

- [`src/m-map-view.slint`](src/m-map-view.slint) defines the reusable map component
- [`src/m-map-adapter.slint`](src/m-map-adapter.slint) defines the backend bridge contract
- [`src/maplibre.slint`](src/maplibre.slint) is the public entrypoint

### Canonical backend

- [`cpp/main.cpp`](cpp/main.cpp) wires `MMapAdapter` to the native map engine
- [`cpp/src/slint_maplibre_headless.cpp`](cpp/src/slint_maplibre_headless.cpp) holds the current production-grade backend logic
- [`cpp/map_window.slint`](cpp/map_window.slint) is a demo shell showing how to use the reusable component

### Experimental backend

- [`rust/main.slint`](rust/main.slint) mirrors the same Slint component contract as the C++ demo
- [`rust/src/maplibre.rs`](rust/src/maplibre.rs) wires `MMapAdapter` to `maplibre-native-rs`
- This path is useful for experimentation on Linux, but it is not the repository's primary story today

## Platform Status

### Reusable Slint component + C++ backend

| Platform | Status | Notes |
|---|---|---|
| Linux x86_64 | Good | Regularly exercised; best-supported development path |
| Windows x64 | Good | Working desktop path |
| macOS Apple Silicon | Good | Working desktop path |

### Reusable Slint component + Rust backend

| Platform | Status | Notes |
|---|---|---|
| Linux x86_64 | Experimental | Tracks `maplibre-native-rs` 0.8.x; best place to validate the Rust path |
| Windows x64 | Not practical | Blocked by `maplibre-native-rs` maturity |
| macOS Apple Silicon | Not practical | Blocked by `maplibre-native-rs` maturity |

This is why the C++ backend remains the canonical reference implementation in this repository.

## Rendering Pipeline

The current implementation uses a GPU-to-CPU readback pipeline:

1. MapLibre Native renders into its headless frontend using the selected backend
2. The rendered frame is read back into CPU memory as `mbgl::PremultipliedImage`
3. Pixels are copied into a `slint::SharedPixelBuffer`
4. `MMapAdapter.frame` is updated
5. `MMapView` displays that frame and forwards interactions back to the backend

This is not the final ideal architecture, but it is robust and cross-platform enough to serve as a practical reference.

## Build Backends

Current desktop backend preferences:

- Linux: WebGPU (`wgpu-native`) by default
- Windows: WebGPU (`wgpu-native`) by default
- macOS: WebGPU (`wgpu-native`) by default, with Metal still useful as a comparison/fallback path

Examples:

```bash
# Default desktop build
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Explicit WebGPU / wgpu-native
cmake -B build -DCMAKE_BUILD_TYPE=Release -DMLN_WITH_WEBGPU=ON -DMLN_WEBGPU_IMPL_WGPU=ON

# macOS Metal fallback / comparison
cmake -B build -DCMAKE_BUILD_TYPE=Release -DMLN_WITH_METAL=ON -DMLN_WITH_OPENGL=OFF -G Xcode

# Explicit OpenGL fallback
cmake -B build -DCMAKE_BUILD_TYPE=Release -DMLN_WITH_WEBGPU=OFF -DMLN_WITH_OPENGL=ON
```

## Project Structure

- [`src/`](src/) - reusable Slint component API
- [`cpp/`](cpp/) - canonical C++ backend integration and demo app
- [`rust/`](rust/) - Linux-oriented experimental Rust backend integration
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

- Keep the reusable Slint API in [`src/`](src/) stable enough for copy-and-adapt usage
- Keep the C++ backend as the authoritative cross-platform reference
- Improve documentation for consumers who want to embed the component in their own Slint app

Longer-term possibilities:

- better packaging so users do not need to think about the C++ toolchain
- a cleaner runtime installation story for downstream apps
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
