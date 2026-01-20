# Building on Ubuntu 24.04 LTS

This guide provides step-by-step instructions for building the MapLibre Native + Slint integration project on Ubuntu 24.04 LTS.

## System Requirements

- Ubuntu 24.04 LTS (Noble Numbat)
- At least 4GB RAM (8GB recommended for parallel builds)
- At least 2GB free disk space
- Internet connection for downloading dependencies

## Step 1: Install System Dependencies

Update your package list and install the required system packages:

```bash
sudo apt update
sudo apt install -y build-essential cmake git pkg-config ninja-build
sudo apt install -y libgl1-mesa-dev libgles2-mesa-dev mesa-common-dev
sudo apt install -y libunistring-dev
sudo apt install -y libicu-dev
sudo apt install -y libcurl4-openssl-dev
sudo apt install -y libssl-dev
sudo apt install -y curl
sudo apt install -y libuv1-dev libglfw3-dev libwebp-dev
```

**Note**: The additional packages (`ninja-build`, `mesa-common-dev`, `libuv1-dev`, `libglfw3-dev`, `libwebp-dev`) are required for optimal build performance and to ensure all MapLibre dependencies are available.

## Step 2: Install Rust

Slint requires Rust for compilation. Install Rust using rustup:

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source "$HOME/.cargo/env"
```

**Note**: The `source` command only sets up the environment for the current terminal session. For a permanent setup, add `source "$HOME/.cargo/env"` to your shell's startup file (e.g., `~/.bashrc` or `~/.profile`) and restart your terminal.

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
make -j$(nproc)
sudo make install
sudo ldconfig
```

Verify the installation:
```bash
pkg-config --modversion slint-cpp
```

## Step 4: Clone and Prepare the Project

```bash
# Clone the repository
git clone https://github.com/maplibre/maplibre-native-slint.git
cd maplibre-native-slint

# Initialize and update submodules
git submodule update --init --recursive
```

## Step 5: Build the Project

### Configure with CMake

Set up the build environment and configure CMake:

```bash
# Set PKG_CONFIG_PATH to ensure pkg-config can find libuv and glfw3
export PKG_CONFIG_PATH="/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/lib/pkgconfig:/usr/share/pkgconfig:$PKG_CONFIG_PATH"

# Configure with CMake using Ninja (recommended for better build performance)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-Wno-error=shadow" -G Ninja .
```

**Alternative**: If Ninja is not available or you prefer Make:

```bash
export PKG_CONFIG_PATH="/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/lib/pkgconfig:/usr/share/pkgconfig:$PKG_CONFIG_PATH"

# Using GNU Make instead of Ninja
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-Wno-error=shadow" .
```

### Build the Project

```bash
# Build the project using all available cores
cmake --build build --parallel $(nproc)
```

**Important Notes**:
- The `PKG_CONFIG_PATH` export is necessary on Ubuntu 24.04 to ensure CMake can locate `libuv` and `glfw3` packages
- The `-Wno-error=shadow` flag prevents compilation errors from Boost Geometry library's variable shadowing warnings
- If you chose Option A for Slint, you'll see a message about "Downloading it from Git and building it locally" during configuration. This is normal and expected.

## Step 6: Verify the Build

Check that the build artifacts were created:

```bash
ls -lh build/cpp/maplibre-slint-example
ls -lh build/cpp/map_window.h
```

You should see:
- `build/cpp/maplibre-slint-example` - Example executable (~13MB, ELF 64-bit LSB pie executable)
- `build/cpp/map_window.h` - Generated Slint UI header (~48KB)

The build artifacts are located in the `build/cpp/` subdirectory.

## Step 7: Test the Application

If you have a graphical environment:
```bash
./build/cpp/maplibre-slint-example
```

For headless testing (will show expected display errors):
```bash
./build/cpp/maplibre-slint-example 2>&1 | head -20
```

## Troubleshooting

### Automatic Build Improvements

This project includes automatic configuration for Ubuntu systems to enable reproducible builds without manual setup:

- **pkg-config paths**: CMakeLists.txt automatically detects and configures standard Linux pkg-config paths (`/usr/lib/x86_64-linux-gnu/pkgconfig`, etc.)
- **Compiler flags**: Automatically adds `-Wno-error=shadow` for GCC to suppress Boost Geometry warnings on modern compilers
- **Dependency checks**: Provides helpful messages if critical dependencies (libuv, glfw3) are missing

With these improvements, **the manual `export PKG_CONFIG_PATH=...` step is no longer required** - CMake handles it automatically.

### Common Build Issues

**CMake cannot find libuv or glfw3:**
```bash
# Ensure PKG_CONFIG_PATH is set before running cmake
export PKG_CONFIG_PATH="/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/lib/pkgconfig:/usr/share/pkgconfig:$PKG_CONFIG_PATH"

# Verify the packages are installed
pkg-config --list-all | grep -E "libuv|glfw3"

# If not found, install them
sudo apt install -y libuv1-dev libglfw3-dev
```

**CMake cannot find Slint:**
```bash
# Make sure Slint is properly installed or can be fetched
sudo ldconfig
pkg-config --list-all | grep slint
```

**Compilation errors with "variable shadowing" warnings:**
```bash
# Use the -Wno-error=shadow flag when configuring CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-Wno-error=shadow" .
```
This is caused by Boost Geometry library using variable shadowing patterns that trigger warnings on modern GCC versions (GCC 13+).

**Missing OpenGL headers:**
```bash
sudo apt install -y mesa-common-dev
```

**Linker errors with libcurl:**
This is a known warning and doesn't affect functionality. If you encounter actual linker errors, verify that libcurl development headers are installed:
```bash
sudo apt install -y libcurl4-openssl-dev
```

**Out of memory during build:**
```bash
# Use fewer parallel jobs
cmake --build build -j2
```

**Build directory already exists from a previous failed attempt:**
```bash
# Clean up and reconfigure
rm -rf build
export PKG_CONFIG_PATH="/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/lib/pkgconfig:/usr/share/pkgconfig:$PKG_CONFIG_PATH"
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-Wno-error=shadow" .
```

### Runtime Issues

**Display connection errors on headless systems:**
This is expected behavior. The application requires a graphical environment or X11 forwarding.

**Permission denied for network requests:**
Check firewall settings and network connectivity.

## Performance Notes

- **Build time**: ~10-20 minutes on a modern 8-core system (depending on system specs)
- **Disk space**: The build uses approximately 1.5-2GB of disk space (sources, build artifacts, and dependencies)
- **Memory**: Parallel compilation with `-j$(nproc)` can use 2-4GB of RAM; reduce parallel jobs if experiencing memory issues
- **Generator choice**: Ninja is faster than Unix Makefiles for incremental builds; use `cmake --build build --parallel $(nproc)` for optimal performance

## Next Steps

After successful build:
1. Run the example application in a graphical environment
2. Explore the source code in `src/` and `examples/`
3. Modify the Slint UI in `cpp/map_window.slint`
4. Check the integration code in `src/slint_maplibre.cpp`

## Ubuntu Version Notes

This guide is specifically tested on Ubuntu 24.04 LTS. For other Ubuntu versions:
- Ubuntu 22.04: Should work with the same instructions
- Ubuntu 20.04: May need newer CMake version (`snap install cmake`)
- Older versions: Not recommended due to outdated dependencies