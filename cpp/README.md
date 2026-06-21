# MapLibre Native Slint - C++ Implementation

This directory hosts the C++ version of the MapLibre Native + Slint example. It mirrors the Rust implementation under `rust/` but uses the C++ MapLibre API and the Slint C++ bindings.

## Build

From the repository root:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Run

```bash
./build/maplibre-slint-example
```

## Files

- `main.cpp` — application entry point and UI wiring
- `map_window.slint` — Slint UI definition that generates `map_window.h`
- `src/slint_maplibre_headless.*` — MapLibre headless integration and rendering
- `platform/custom_file_source.*` — optional HTTP file source using CPR

## Zero-copy OpenGL example (`maplibre-slint-gl`)

`maplibre-slint-example` (above) renders the map with `mbgl::HeadlessFrontend`
and copies every frame back to the CPU (`readStillImage`) before handing it to
Slint. `maplibre-slint-gl` instead renders MapLibre Native **directly into an
OpenGL texture inside Slint's own GL context** and hands that texture to Slint
as a borrowed texture (`slint::Image::create_from_borrowed_gl_2d_rgba_texture`).
There is no GPU->CPU readback and no second GL context, which suits
GPU-constrained devices such as the Raspberry Pi.

How it works:

- A custom `mbgl::gl::RendererBackend` + `mbgl::gfx::Renderable`
  (`src/slint_gl_backend.*`, modelled on the upstream GLFW backend) renders into
  an FBO whose colour texture lives in Slint's GL context. GL entry points are
  loaded via `eglGetProcAddress`; `activate()`/`deactivate()` are no-ops because
  Slint's context is already current inside the rendering-notifier callback.
- `main_gl.cpp` creates the FBO/texture (+ depth/stencil) during
  `RenderingState::RenderingSetup`, renders the map into it during
  `BeforeRendering`, and publishes the texture with
  `create_from_borrowed_gl_2d_rgba_texture(..., BottomLeft)`.
- `gl_map_window.slint` is a small-panel / touch layout (large buttons, a
  right-edge vertical zoom slider + zoom buttons, no pitch/bearing sliders).

### Build

Requires the OpenGL backend (not WebGPU) and Slint's FemtoVG GL renderer. On a
Raspberry Pi running on the console over DRM/KMS, also use the libseat linuxkms
backend:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DMLN_WITH_OPENGL=ON -DMLN_WITH_WEBGPU=OFF -DMLN_WITH_GLFW=OFF \
  -DSLINT_FEATURE_RENDERER_FEMTOVG=ON \
  -DSLINT_FEATURE_BACKEND_LINUXKMS=ON
cmake --build build --target maplibre-slint-gl
```

The target is compiled with `-fno-rtti` to match `mbgl-core`
(`MLN_WITH_RTTI=OFF`).

### Run

```bash
SLINT_BACKEND=linuxkms ./build/cpp/maplibre-slint-gl
```

| Variable | Effect |
|---|---|
| `MAPLIBRE_STYLE_URL` | Initial style URL |
| `MAPLIBRE_WIDTH` / `MAPLIBRE_HEIGHT` | Render size (default: the display resolution) |
| `MAPLIBRE_FLY_MS` | `flyTo` duration in ms for the city buttons (default 2500) |

### Raspberry Pi notes

Verified on a Raspberry Pi 4 (Debian trixie, aarch64, V3D / `vc4-kms-v3d`) with a
480x320 HDMI touch panel (Quimat MPI3508) on the console over DRM/KMS:

- **DRM master**: install/enable `seatd`, add the user to the `video` group, and
  build Slint with `SLINT_FEATURE_BACKEND_LINUXKMS=ON` (libseat). seatd grants
  DRM master without an X11/Wayland session or an active VT.
- **Display mode**: the MPI3508 only advertises standard HDMI modes (it scales
  internally to 480x320); a raw 480x320 CVT signal is rejected. For less
  downscaling, force e.g. `video=HDMI-A-1:720x576@50` in `cmdline.txt`.
- **Touch**: the XPT2046/ADS7846 resistive controller is single-touch (no
  pinch). Enable `dtparam=spi=on` and
  `dtoverlay=ads7846,cs=1,penirq=25,...`, then set a libinput calibration matrix
  via udev, e.g. `ENV{LIBINPUT_CALIBRATION_MATRIX}="0 -1 1 -1 0 1"` for this
  panel/orientation. Resistive panels have a dead zone at the very edges, so
  keep interactive controls inset with a margin (the Pi layout does this).
  Double-tap zooms in (detected in `handle_mouse_press`); zoom out with the
  on-screen zoom buttons/slider.
- maplibre-native renders on the Pi's V3D GPU, composited zero-copy by Slint;
  the `flyTo` camera animation stays smooth.
