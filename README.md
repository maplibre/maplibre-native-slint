# MapLibre Native + Slint Integration

This project demonstrates the integration of MapLibre Native (C++ mapping library) with Slint UI framework for creating interactive map applications.

## Quick Start

For detailed build instructions, see the platform-specific guides:

- **Linux**: [Ubuntu 24.04 Build Guide](docs/build/Linux_Ubuntu_24.md)
- **macOS**: [macOS Build Guide](docs/build/macOS_Apple_Silicon.md)
- **Windows**: [Windows Build Guide](docs/build/Windows.md) _(coming soon)_

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
git clone https://github.com/yuiseki/maplibre-native-slint.git
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

- **CPU-based Rendering**: Renders MapLibre Native output to a CPU pixel buffer for integration with Slint.
- **Slint UI Integration**: Displays maps within Slint user interfaces
- **Custom File Source**: HTTP-based tile and resource loading
- **Touch/Mouse Interaction**: Interactive map navigation (Partial, in progress)
- **Cross-platform**: Supports Linux, Windows, and macOS

## Technical Details

### API Compatibility

This project has been updated to work with the latest MapLibre Native APIs:

- Modern RunLoop API using composition instead of inheritance
- Updated ResourceOptions without deprecated methods
- Current HeadlessFrontend construction patterns
- Proper OpenGL context management

### Rendering Pipeline

The current rendering pipeline is CPU-based:

1. MapLibre Native's `HeadlessFrontend` renders the map to an in-memory pixel buffer (`mbgl::PremultipliedImage`).
2. This image data is converted from a premultiplied alpha format to a non-premultiplied format.
3. The pixel data is copied into a `slint::SharedPixelBuffer`.
4. Slint uses this buffer to render the map image within its UI.
5. User interactions from the Slint UI are captured and forwarded to the MapLibre Native map instance.

## To Do

- [ ] Implement a more efficient GPU-based rendering pipeline using `BorrowedOpenGLTexture` to avoid CPU-GPU data transfer overhead.
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
