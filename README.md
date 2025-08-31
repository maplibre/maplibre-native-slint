# MapLibre Native + Slint Integration

This project demonstrates the integration of [MapLibre Native](https://github.com/maplibre/maplibre-native) (C++ mapping library) with Slint UI framework for creating interactive map applications.

## Quick Start

For detailed build instructions, see the platform-specific guides:

- **Linux**: [Ubuntu 24.04 Build Guide](docs/build/Linux_Ubuntu_24.md)
- **macOS**: [macOS Build Guide](docs/build/macOS_Apple_Silicon.md)
- **Windows**: [Windows Build Guide](docs/build/Windows.md) _(coming soon)_
  - For GL zero-copy (Slint Skia OpenGL) on Windows, see [Windows_GL.md](docs/build/Windows_GL.md)

## Screenshots

### Linux desktop

[![Image from Gyazo](https://i.gyazo.com/b2b0b9e0af3a2e8f7342d3db6b3c899f.png)](https://gyazo.com/b2b0b9e0af3a2e8f7342d3db6b3c899f)

### macOS Apple Silicon

[![Image from Gyazo](https://i.gyazo.com/d9506d8ed7d5d30a921624bd7a893c88.jpg)](https://gyazo.com/d9506d8ed7d5d30a921624bd7a893c88)

## Prerequisites

- C++20 compatible compiler
- CMake 3.21 or later
- Rust toolchain (for Slint auto-download)
- OpenGL/GLES development headers
- Network connectivity for downloading dependencies

## Basic Build Process

```bash
# Install dependencies (see platform guides for details)
# Clone and prepare
git clone https://github.com/maplibre/maplibre-native-slint.git
cd maplibre-native-slint
git submodule update --init --recursive

# Build (Slint will be automatically downloaded if needed)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run
./maplibre-slint-example
```

**Note**: Slint dependencies are now automatically resolved! CMake will download and build Slint if it's not found system-wide.

## Project Structure

- `src/` - Main source code for the MapLibre-Slint integration
- `examples/` - Example applications
- `platform/` - Platform-specific implementations (RunLoop, FileSource)
- `vendor/` - MapLibre Native submodule
- `CMakeLists.txt` - Build configuration

## Features

- **Hybrid GPU-CPU Rendering**: MapLibre Native renders on GPU, with pixel data transferred to CPU for Slint integration.
- **Slint UI Integration**: Displays maps within Slint user interfaces
- **Custom File Source**: HTTP-based tile and resource loading
- **Touch/Mouse Interaction**: Interactive map navigation (Partial, in progress)
- **Cross-platform**: Supports Linux, Windows, and macOS

## Platform and Backend Support

### Graphics Backend Features

This project supports multiple rendering backends, automatically selected based on the target platform:

- **OpenGL/GLES** (primary on Linux/Windows): Uses HeadlessFrontend with CPU-based rendering
- **Metal** (primary on macOS): Native Metal framework integration for Apple Silicon
- **Vulkan**: Not currently implemented

### Platform Support Matrix

The following platform and graphics backend combinations are supported and tested:

| Platform        | OpenGL/GLES | Metal | Vulkan | CI Status |
|----------------|-------------|-------|---------|-----------|
| Linux x86_64   | ✅          | ❌    | ❌      | ✅        |
| Linux ARM64    | 🟨*         | ❌    | ❌      | ❌        |
| Windows x86_64 | ✅          | ❌    | ❌      | ❌        |
| Windows ARM64  | 🟨*         | ❌    | ❌      | ❌        |
| macOS ARM64    | ❌          | ✅    | ❌      | ❌        |
| macOS x86_64   | ❌          | 🟨*   | ❌      | ❌        |

**Legend:**
- ✅ **Fully Supported**: Tested and working
- 🟨 **Experimental**: Should work but not extensively tested
- ❌ **Not Supported**: Not implemented or not compatible

**Notes:**
- \* Architecture should work but hasn't been extensively tested
- CI currently runs only on Ubuntu x86_64 with Xvfb for headless testing
- macOS builds explicitly target ARM64 only (x86_64 is excluded)
- Windows builds target x64 architecture via MSVC toolchain

### Build Configuration Options

The project automatically detects and configures the appropriate backend:

```bash
# Linux/Windows (OpenGL/GLES)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# macOS (Metal backend)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DMLN_WITH_METAL=ON -DMLN_WITH_OPENGL=OFF -G Xcode
```

## Technical Details

### API Compatibility

This project has been updated to work with the latest MapLibre Native APIs:

- Modern RunLoop API using composition instead of inheritance
- Updated ResourceOptions without deprecated methods
- Current HeadlessFrontend construction patterns
- Proper OpenGL context management

### Rendering Pipeline

The current rendering pipeline involves GPU-to-CPU data transfer:

1. MapLibre Native's `HeadlessFrontend` renders the map using GPU (OpenGL/Metal/etc.) to an internal framebuffer.
2. The GPU-rendered image is read back to CPU memory as `mbgl::PremultipliedImage`.
3. This CPU image data is converted from premultiplied alpha format to non-premultiplied format.
4. The pixel data is copied into a `slint::SharedPixelBuffer` for CPU-based rendering.
5. Slint displays the map image within its UI using the CPU pixel buffer.
6. User interactions from the Slint UI are captured and forwarded to the MapLibre Native map instance.

**Performance Note**: This GPU-to-CPU data transfer creates overhead. Future improvements could use `BorrowedOpenGLTexture` for direct GPU texture sharing.

## To Do

- [ ] Implement a more efficient GPU-based rendering pipeline using `BorrowedOpenGLTexture` to eliminate GPU-to-CPU data transfer overhead.
- [ ] Fully implement and stabilize touch and mouse interactions (zooming, panning, rotation).
- [ ] Add more examples and improve documentation.

## Troubleshooting

### Build Issues

- Ensure all dependencies are installed
- Check that Slint is properly installed system-wide
- Verify OpenGL/GLES development headers are available

### Runtime Issues

- For headless environments, set up X11 forwarding or virtual display
- Check that OpenGL drivers are properly installed
- Verify network connectivity for map tile loading

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test the build process
5. Submit a pull request

## Community

- **Slack:** Join the conversation on the [#maplibre-native](https://osmus.slack.com/archives/C01G4G39862) channel on OSM-US Slack. [Click here for an invite](https://slack.openstreetmap.us/).
- **Website:** [maplibre.org](https://maplibre.org/)

## License

Copyright (c) 2025 MapLibre contributors.

This project is licensed under the BSD 2-Clause License. See the [LICENSE](LICENSE) file for details.

This project integrates multiple components with different licenses:

- MapLibre Native: BSD License
- Slint: GPL-3.0-only OR LicenseRef-Slint-Royalty-free-2.0 OR LicenseRef-Slint-Software-3.0
