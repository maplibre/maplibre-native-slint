# MapLibre Native Slint - Rust Implementation

A Rust-based interactive map viewer using [MapLibre Native](https://github.com/maplibre/maplibre-native) for rendering and [Slint](https://slint.dev/) for the user interface.

## Acknowledgments

This implementation is based on the excellent work from [slintmaplibretest](https://gitlab.com/Murmele/slintmaplibretest) by Murmele. Special thanks for providing the foundation and reference implementation.

## Prerequisites

- Rust toolchain (edition 2024 or later)
- CMake (required for building MapLibre Native)
- C++ compiler (required for building MapLibre Native)
- OpenGL or Metal development libraries (platform-dependent)

## Building

To build the project:

```bash
cd rust
cargo build
```

For a release build with optimizations:

```bash
cargo build --release
```

## Running

After building, you can run the application:

### Development build

```bash
cargo run
```

Or directly:

```bash
./target/debug/maplibre_native_slint
```

### Release build

```bash
cargo run --release
```

Or directly:

```bash
./target/release/maplibre_native_slint
```

## Features

- Interactive map rendering using MapLibre Native
- Slint-based modern UI
- Mouse/trackpad navigation support
- Default map style from [MapLibre Demo Tiles](https://demotiles.maplibre.org/)

## Project Structure

```
rust/
├── build.rs           # Build script for Slint compilation
├── main.slint         # Slint UI definition
├── Cargo.toml         # Rust dependencies
└── src/
    ├── main.rs        # Application entry point
    ├── maplibre.rs    # MapLibre integration logic
    └── maplibre/
        └── headless.rs # MapLibre headless rendering implementation
```

## Dependencies

- `slint = "1.14.1"` - UI framework
- `maplibre_native = "0.4.1"` - Map rendering engine
- `slint-build = "1.14.1"` - Build-time dependency for Slint compilation

## Troubleshooting

### Display issues on headless environments

This application requires a graphical display. On headless systems or SSH sessions, you may encounter errors like:

```
qt.qpa.xcb: could not connect to display
```

Ensure you have:
- X11 forwarding enabled (for SSH: `ssh -X`)
- A running display server
- Proper `DISPLAY` environment variable set

## License

See the main project LICENSE file.
