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
sudo apt install -y build-essential cmake git pkg-config
sudo apt install -y libgl1-mesa-dev libgles2-mesa-dev
sudo apt install -y libunistring-dev
sudo apt install -y libicu-dev
sudo apt install -y libcurl4-openssl-dev
sudo apt install -y libssl-dev
sudo apt install -y curl
```

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

```bash
# Configure with CMake (will auto-download Slint if needed)
cmake -B build -DCMAKE_BUILD_TYPE=Release -G Ninja .

# Build the project (use all available cores)
cmake --build build
```

**Note**: If you chose Option A above, you'll see a message like "Slint could not be located in the CMake module search path. Downloading it from Git and building it locally" during the cmake configuration step. This is normal and expected.

## Step 6: Verify the Build

Check that the build artifacts were created:

```bash
ls -la build/
```

You should see:
- `libmaplibre-native-slint.so` - Main library
- `maplibre-slint-example` - Example executable
- `map_window.h` - Generated Slint UI header

## Step 7: Test the Application

If you have a graphical environment:
```bash
./build/maplibre-slint-example
```

For headless testing (will show expected display errors):
```bash
./build/maplibre-slint-example 2>&1 | head -20
```

## Troubleshooting

### Common Build Issues

**CMake cannot find Slint:**
```bash
# Make sure Slint is properly installed
sudo ldconfig
pkg-config --list-all | grep slint
```

**Missing OpenGL headers:**
```bash
sudo apt install -y mesa-common-dev
```

**Linker errors with libcurl:**
This is a known warning and doesn't affect functionality.

**Out of memory during build:**
```bash
# Use fewer parallel jobs
cmake --build . -j2
```

### Runtime Issues

**Display connection errors on headless systems:**
This is expected behavior. The application requires a graphical environment or X11 forwarding.

**Permission denied for network requests:**
Check firewall settings and network connectivity.

## Performance Notes

- Build time: ~10-15 minutes on a modern system
- The build uses significant disk space (~1.5GB) due to MapLibre Native
- Using `-j$(nproc)` speeds up compilation but uses more memory

## Next Steps

After successful build:
1. Run the example application in a graphical environment
2. Explore the source code in `src/` and `examples/`
3. Modify the Slint UI in `examples/map_window.slint`
4. Check the integration code in `src/slint_maplibre.cpp`

## Ubuntu Version Notes

This guide is specifically tested on Ubuntu 24.04 LTS. For other Ubuntu versions:
- Ubuntu 22.04: Should work with the same instructions
- Ubuntu 20.04: May need newer CMake version (`snap install cmake`)
- Older versions: Not recommended due to outdated dependencies