# MapLibre Native + Slint Integration

This project demonstrates the integration of MapLibre Native (C++ mapping library) with Slint UI framework for creating interactive map applications.

## Prerequisites

Before building, ensure you have the following dependencies installed:

### System Dependencies
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y build-essential cmake git pkg-config
sudo apt install -y libgl1-mesa-dev libgles2-mesa-dev
sudo apt install -y libunistring-dev
sudo apt install -y libicu-dev
sudo apt install -y libcurl4-openssl-dev
sudo apt install -y libssl-dev
```

### Rust (for Slint)
```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source ~/.cargo/env
```

### Slint Framework
Slint needs to be built from source and installed system-wide:

```bash
# Install slint-viewer via cargo
cargo install slint-viewer

# Clone and build Slint from source
cd /tmp
git clone https://github.com/slint-ui/slint.git
cd slint
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
sudo ldconfig
```

## Building the Project

### 1. Clone the Repository
```bash
git clone https://github.com/yuiseki/maplibre-native-slint.git
cd maplibre-native-slint
```

### 2. Initialize Submodules
```bash
git submodule update --init --recursive
```

### 3. Create Build Directory
```bash
mkdir build
cd build
```

### 4. Configure with CMake
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
```

### 5. Build the Project
```bash
cmake --build .
```

## Running the Application

After successful build, you can run the example application:

```bash
./build/maplibre-slint-example
```

**Note**: This application requires a graphical environment (X11 display). If running on a headless system, you'll need to set up X11 forwarding or use a virtual display.

## Project Structure

- `src/` - Main source code for the MapLibre-Slint integration
- `examples/` - Example applications
- `platform/` - Platform-specific implementations (RunLoop, FileSource)
- `vendor/` - MapLibre Native submodule
- `CMakeLists.txt` - Build configuration

## Features

- **OpenGL Integration**: Renders MapLibre Native output to OpenGL textures
- **Slint UI Integration**: Displays maps within Slint user interfaces
- **Custom File Source**: HTTP-based tile and resource loading
- **Touch/Mouse Interaction**: Interactive map navigation
- **Cross-platform**: Supports Linux, Windows, and macOS

## Technical Details

### API Compatibility
This project has been updated to work with the latest MapLibre Native APIs:
- Modern RunLoop API using composition instead of inheritance
- Updated ResourceOptions without deprecated methods
- Current HeadlessFrontend construction patterns
- Proper OpenGL context management

### Rendering Pipeline
1. MapLibre Native renders to an OpenGL framebuffer
2. The framebuffer texture is passed to Slint as a `BorrowedOpenGLTexture`
3. Slint displays the texture within its UI components
4. User interactions are forwarded back to MapLibre Native

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

## License

This project integrates multiple components with different licenses:
- MapLibre Native: BSD License
- Slint: GPL-3.0-only OR LicenseRef-Slint-Royalty-free-2.0 OR LicenseRef-Slint-Software-3.0
- Project code: [Your chosen license]