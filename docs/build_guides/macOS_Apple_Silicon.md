# Building on macOS (Apple Silicon)

This guide provides step-by-step instructions for building the MapLibre Native + Slint integration project on macOS with Apple Silicon (M1/M2/M3/M4).

## System Requirements

- macOS 14.0 (Sonoma) or later
- Apple Silicon Mac (M1/M2/M3/M4)
- Xcode Command Line Tools
- Homebrew
- Rust

## Step 1: Install Dependencies

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
brew install cmake pkg-config clang-format ninja
brew install bazelisk webp libuv webp icu4c jpeg-turbo glfw
brew link icu4c --force
```

## Step 2: Install Rust

Slint requires the Rust toolchain. Install it using rustup:

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
source ~/.cargo/env
```

## Step 3: Clone and Prepare the Project

```bash
# Clone the repository
git clone https://github.com/maplibre/maplibre-native-slint.git
cd maplibre-native-slint

# Initialize and update submodules
git submodule update --init --recursive
```

## Step 4: Build the Project

The project supports two build configurations:

### Option A: Metal Zero-Copy Rendering (Recommended)

This uses the Metal backend with zero-copy rendering for optimal performance on Apple Silicon:

```bash
# Configure with Ninja (faster builds)
cmake -B build-metal -DCMAKE_BUILD_TYPE=Release -DMLN_WITH_METAL=ON -DMLN_WITH_OPENGL=OFF -DSLINT_MAPLIBRE_USE_METAL=ON -G Ninja .

# Build the project
ninja -C build-metal
```

### Option B: Xcode Project (for debugging)

For development and debugging with Xcode:

```bash
# Configure with Xcode generator
cmake -B build -DCMAKE_BUILD_TYPE=Release -DMLN_WITH_METAL=ON -DMLN_WITH_OPENGL=OFF -DSLINT_MAPLIBRE_USE_METAL=ON -G Xcode .

# Build the project
cmake --build build
```

The build process can take 10-15 minutes on the first run. Subsequent builds will be much faster.

## Step 5: Run the Application

After a successful build, run the executable:

### For Ninja builds (Metal zero-copy):
```bash
./build-metal/maplibre-slint-example
```

### For Xcode builds:
```bash
./build/Debug/maplibre-slint-example
```

This will launch a window displaying the map with:
- **Zero-copy Metal rendering** - Direct framebuffer rendering without CPU buffer copying
- **HiDPI/Retina support** - Sharp rendering on high-resolution displays
- **Interactive controls**:
  - Click and drag to pan the map
  - Scroll up/down to zoom in/out
  - Double-click to zoom in
  - Shift+double-click to zoom out
  - Use the fly-to buttons to jump to different cities

## Troubleshooting

- **Build Fails**: Ensure all dependencies are installed correctly and that your Rust environment is set up (`source ~/.cargo/env`).
- **`cmake` command not found**: Make sure you have installed `cmake` via Homebrew and that your shell can find it.
- **`ninja` command not found**: Install Ninja via Homebrew: `brew install ninja`
- **Linker Errors**: If you encounter linker errors, try cleaning the build directory (`rm -rf build-metal` or `rm -rf build`) and re-running the build process from Step 4.
- **Blurry Rendering**: The Metal zero-copy build includes HiDPI support. If you still see blurry rendering, ensure you're using the `build-metal` configuration.
- **Inverted Zoom**: The latest build fixes zoom direction. Scroll up to zoom in, scroll down to zoom out.
