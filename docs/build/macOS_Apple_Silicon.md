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
brew install cmake pkg-config
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
git clone https://github.com/yuiseki/maplibre-native-slint.git
cd maplibre-native-slint

# Initialize and update submodules
git submodule update --init --recursive
```

## Step 4: Build the Project

The project is configured to build with the Metal rendering backend.

```bash
# Configure with CMake. This will also download and build Slint automatically.
cmake -B build -DCMAKE_BUILD_TYPE=Release -DMLN_WITH_METAL=ON -DMLN_WITH_OPENGL=OFF -G Xcode .

# Build the project
cmake --build build
```

The build process can take 10-15 minutes on the first run. Subsequent builds will be much faster.

## Step 5: Run the Application

After a successful build, the executable will be located in the `build/Debug` directory.

```bash
./build/Debug/maplibre-slint-example
```

This will launch a window displaying the map. You can interact with it using your mouse to pan and zoom.

## Troubleshooting

- **Build Fails**: Ensure all dependencies are installed correctly and that your Rust environment is set up (`source ~/.cargo/env`).
- **`cmake` command not found**: Make sure you have installed `cmake` via Homebrew and that your shell can find it.
- **Linker Errors**: If you encounter linker errors, try cleaning the build directory (`rm -rf build`) and re-running the build process from Step 4.
