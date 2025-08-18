# Building on macOS Apple Silicon

This guide provides step-by-step instructions for building the MapLibre Native + Slint integration project on macOS with Apple Silicon (M1/M2/M3/M4).

## System Requirements

- macOS 12.0 (Monterey) or later
- Apple Silicon Mac (M1/M2/M3/M4)
- At least 8GB RAM (16GB recommended for parallel builds)
- At least 4GB free disk space
- Internet connection for downloading dependencies
- Xcode Command Line Tools

## Step 1: Install System Dependencies

First, ensure Xcode Command Line Tools are installed:

```bash
xcode-select --install
```

Install Homebrew if not already installed:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

Install the required system packages:

```bash
brew install cmake pkg-config ninja bazel
brew install icu4c jpeg libpng zlib curl openssl libuv glfw
brew install llvm swift
```

## Step 2: Install Rust

Slint requires Rust for compilation. Install Rust using rustup:

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
source ~/.cargo/env
```

Verify the installation:

```bash
rustc --version
cargo --version
```

## Step 3: Slint Dependencies (Optional)

**NEW**: Slint is now automatically downloaded and built by CMake if not found system-wide. You can either:

### Option A: Use Automatic Download (Recommended)

Skip this step - CMake will automatically download and build Slint during project configuration.

### Option B: Install System-wide (For Multiple Projects)

If you plan to use Slint in multiple projects, install it system-wide:

```bash
# Install slint-viewer via cargo
cargo install slint-viewer

# Build Slint from source
cd /tmp
git clone https://github.com/slint-ui/slint.git
cd slint
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
sudo make install
```

## Step 4: Clone and Prepare the Project

```bash
# Clone the repository
git clone https://github.com/yuiseki/maplibre-native-slint.git
cd maplibre-native-slint

# Initialize and update submodules
git submodule update --init --recursive
```

## Step 5: Build the Project

```bash
# Create build directory
mkdir build && cd build

# Set up environment variables for Homebrew libraries
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
export CMAKE_Swift_COMPILER=/opt/homebrew/opt/swift/bin/swiftc
source ~/.cargo/env
export PKG_CONFIG_PATH="/opt/homebrew/opt/curl/lib/pkgconfig:/opt/homebrew/lib/pkgconfig:$PKG_CONFIG_PATH"

# Configure with CMake (will auto-download Slint if needed)
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DMLN_WITH_OPENGL=ON \
         -DMLN_DARWIN_USE_LIBUV=ON \
         -DBUILD_TESTS=OFF \
         -G Ninja

# Build the project (use all available cores)
ninja maplibre-slint-example
```

**Note**: The build process will automatically download and compile Slint, which may take 10-15 minutes on the first run.

## Step 6: Verify the Build

Check that the build artifacts were created:

```bash
ls -la
```

You should see:

- `maplibre-slint-example` - Example executable (ARM64 Mach-O binary)
- Various static libraries for MapLibre Native and dependencies
- Generated Slint UI headers

Verify the executable format:

```bash
file maplibre-slint-example
```

Expected output: `maplibre-slint-example: Mach-O 64-bit executable arm64`

## Step 7: Test the Application

**Note**: The application requires a graphical environment to run properly.

```bash
./maplibre-slint-example
```

**Known Issue**: The current implementation may encounter runtime issues. This is expected for the initial build and requires further debugging for proper GUI display.

## Troubleshooting

### Common Build Issues

**Swift compiler not found:**

```bash
# Ensure Swift is properly installed and accessible
export CMAKE_Swift_COMPILER=/opt/homebrew/opt/swift/bin/swiftc
# Or use system Swift
export CMAKE_Swift_COMPILER=/usr/bin/swiftc
```

**OpenGL ES headers not found:**
This is automatically handled by the macOS-specific build configuration that uses OpenGL instead of OpenGL ES.

**CMake cannot find libraries:**

```bash
# Ensure Homebrew paths are in PKG_CONFIG_PATH
export PKG_CONFIG_PATH="/opt/homebrew/opt/curl/lib/pkgconfig:/opt/homebrew/opt/icu4c@77/lib/pkgconfig:/opt/homebrew/opt/jpeg/lib/pkgconfig:/opt/homebrew/lib/pkgconfig:$PKG_CONFIG_PATH"
```

**Bazel not found:**

```bash
brew install bazel
```

**Out of memory during build:**

```bash
# Use fewer parallel jobs (adjust number as needed)
ninja -j4 maplibre-slint-example
```

### Runtime Issues

**Segmentation fault on startup:**
This is a known issue with the current implementation. The application successfully builds but requires additional debugging for proper runtime execution in the macOS environment.

**Graphics/Display issues:**
Ensure you're running in a proper graphical environment (not over SSH without X11 forwarding).

## Platform-Specific Notes

### macOS vs Linux Differences

1. **OpenGL vs OpenGL ES**: macOS uses standard OpenGL headers (`OpenGL/gl3.h`) instead of OpenGL ES (`GLES3/gl3.h`)
2. **Swift Dependency**: MapLibre Native on macOS requires Swift compiler for platform-specific code
3. **Framework Linking**: macOS uses system frameworks (CoreGraphics, CoreLocation, etc.)
4. **Build Tools**: Uses Ninja instead of Make for better performance on macOS

### Apple Silicon Specifics

- All dependencies are compiled natively for ARM64 architecture
- Homebrew automatically handles ARM64 vs x86_64 differences
- No Rosetta 2 translation required for any components

## Performance Notes

- Build time: ~15-25 minutes on Apple Silicon (M1/M2/M3)
- The build uses significant disk space (~2GB) due to MapLibre Native and Rust dependencies
- Memory usage can peak at 4-6GB during parallel compilation
- Subsequent builds are much faster (~2-3 minutes) with incremental compilation

## Next Steps

After successful build:

1. Debug the runtime segmentation fault issue
2. Explore the source code in `src/` and `examples/`
3. Modify the Slint UI in `examples/map_window.slint`
4. Check the integration code in `src/slint_maplibre.cpp`
5. Consider implementing macOS-specific display handling

## macOS Version Compatibility

This guide is specifically tested on:

- **macOS 15.0 (Sequoia)** with Apple Silicon - ✅ Confirmed working
- **macOS 14.0 (Sonoma)** with Apple Silicon - Should work
- **macOS 13.0 (Ventura)** with Apple Silicon - Should work
- **macOS 12.0 (Monterey)** with Apple Silicon - May work with older Xcode

For Intel Macs, additional configuration may be required for some Homebrew packages.

## Known Limitations

1. **Runtime Stability**: Application builds successfully but has runtime segmentation faults that need debugging
2. **GUI Integration**: Slint UI integration with macOS windowing system needs refinement
3. **Code Signing**: May require additional configuration for distribution

## Development Tips

- Use `ninja` instead of `make` for faster incremental builds
- Consider using `ccache` for even faster recompilation
- Build with `-DCMAKE_BUILD_TYPE=Debug` for debugging runtime issues
- Use Xcode or lldb for debugging segmentation faults
