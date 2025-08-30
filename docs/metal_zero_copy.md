# Metal Zero-Copy Integration (Slint + MapLibre Native)

## Overview

This implementation provides zero-copy Metal rendering for MapLibre Native integrated with Slint UI framework on macOS. MapLibre Native renders directly into a CAMetalLayer embedded in the Slint window, eliminating CPU buffer copying and providing optimal performance on Apple Silicon.

## Architecture

### Key Components

- **`src/metal/slint_metal_maplibre.mm/.hpp`**: Core integration managing CAMetalLayer, MapLibre Metal backend, Renderer, and Map instances
- **`src/metal/slint_metal_renderer_backend.mm/.hpp`**: Metal renderer backend implementing MapLibre's gfx::RendererBackend interface
- **`src/metal/metal_cpp_symbols.mm`**: Metal-cpp symbol definitions
- **`examples/main.mm`**: Objective-C++ entry point that attaches the Metal layer to NSWindow and manages the run loop
- **`examples/map_window.slint`**: Slint UI with map controls and Metal layer embedding

### Frame Scheduling

The implementation uses a CFRunLoopTimer for consistent 60Hz frame scheduling:

1. MapLibre signals frame requests via `onInvalidate()` or style/tile loading events
2. Frame requests set an atomic flag for deferred rendering
3. CFRunLoopTimer fires at 60Hz to check the flag and render frames
4. Rendering happens outside active event dispatch to prevent re-entrancy issues

### Zero-Copy Rendering Pipeline

1. CAMetalLayer provides drawable textures directly to MapLibre
2. MapLibre Metal backend renders directly into the drawable's texture
3. No intermediate CPU buffers or texture uploads required
4. Direct presentation to display via Metal's present mechanism

## Features

- **Zero-copy rendering**: Direct Metal framebuffer rendering without CPU buffer copying
- **HiDPI/Retina support**: Sharp rendering with proper device pixel ratio handling
- **60Hz frame scheduling**: Smooth animations via CFRunLoopTimer
- **Interactive controls**: Pan, zoom, and fly-to navigation
- **Style switching**: Support for multiple map styles
- **Proper drawable management**: Automatic drawable acquisition and presentation

## Building (macOS / Apple Silicon)

### Prerequisites

```bash
# Install build tools
brew install cmake ninja

# Clone and prepare repository
git clone https://github.com/maplibre/maplibre-native-slint.git
cd maplibre-native-slint
git submodule update --init --recursive
```

### Build Commands

```bash
# Configure with Ninja (recommended for faster builds)
# Note: MLN_WITH_OPENGL=OFF is required to avoid hybrid backend issues
cmake -B build-metal -DCMAKE_BUILD_TYPE=Release -DMLN_WITH_METAL=ON -DMLN_WITH_OPENGL=OFF -DSLINT_MAPLIBRE_USE_METAL=ON -G Ninja .

# Build the project
ninja -C build-metal

# Run the example
./build-metal/maplibre-slint-example
```

**Important**: The `MLN_WITH_OPENGL=OFF` flag is essential on macOS. When both OpenGL and Metal are enabled, MapLibre's `DepthMode` struct includes an additional `range` field that must be initialized, which can cause compilation errors in Metal-specific code. Using Metal-only configuration ensures a clean build.

### Alternative: Xcode Build (for debugging)

```bash
# Configure with Xcode generator
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DMLN_WITH_METAL=ON -DMLN_WITH_OPENGL=OFF -DSLINT_MAPLIBRE_USE_METAL=ON -G Xcode .

# Build via Xcode
cmake --build build

# Run the example
./build/Debug/maplibre-slint-example
```

## Runtime Behavior

### Console Output

The application provides detailed logging:
- `[SlintMetalMapLibre] initialize(w,h, dpr=X)`: Initial setup with dimensions and DPR
- `[SlintMetalMapLibre] Created CAMetalLayer`: Layer creation confirmation
- `[SlintMetalMapLibre] render_once()`: Frame rendering events
- `[MapObserver] Style loaded successfully!`: Map style loading confirmation
- `[Interaction] Mouse press/move`: User interaction events

### Interactive Controls

- **Click and drag**: Pan the map
- **Scroll wheel**: Zoom in/out (scroll up = zoom in)
- **Double-click**: Zoom in at location
- **Shift+double-click**: Zoom out at location
- **Fly-to buttons**: Navigate to predefined cities

## Implementation Details

### RendererObserver API

The implementation uses MapLibre's current RendererObserver API:
```cpp
void onWillStartRenderingFrame() override
void onDidFinishRenderingFrame(RenderMode mode, bool repaint, bool placementChanged, const RenderingStats& stats) override
void onDidFinishLoadingStyle() override
```

### HiDPI Support

Proper Retina display support via:
- Setting CAMetalLayer's `contentsScale` property to match display DPR
- Using physical pixel dimensions (logical × DPR) for backend sizing
- Passing device pixel ratio through the rendering pipeline

### Drawable Management

- Single drawable acquisition per frame (prevents resource exhaustion)
- Proper drawable presentation after rendering
- Automatic drawable release via RAII

## Troubleshooting

### Blank or Black Window
- Verify network connectivity for tile loading
- Check console for style loading errors
- Ensure Metal is supported on your Mac

### Blurry Rendering
- Verify DPR is correctly detected (should be 2.0 for Retina displays)
- Check that `contentsScale` is being set on CAMetalLayer
- Ensure physical pixel dimensions are used for rendering

### Build Failures
- Ensure all submodules are initialized: `git submodule update --init --recursive`
- Verify Ninja is installed: `brew install ninja`
- Clean build directory if switching configurations: `rm -rf build-metal`

### Performance Issues
- Check console for excessive frame requests
- Verify 60Hz timer is functioning correctly
- Monitor for drawable acquisition failures

## Future Enhancements

- **CVDisplayLink integration**: Optional vsync-synchronized rendering
- **Multi-window support**: Multiple map instances with separate Metal layers
- **Gesture recognizers**: Native macOS gesture support
- **Performance metrics**: Frame timing and rendering statistics
- **Custom render passes**: Support for user-defined Metal shaders

## Technical Requirements

- macOS 14.0 (Sonoma) or later
- Apple Silicon Mac (M1/M2/M3/M4)
- Metal 2.1+ support
- Xcode Command Line Tools

## License

Follows project root licensing (BSD 2-Clause).
