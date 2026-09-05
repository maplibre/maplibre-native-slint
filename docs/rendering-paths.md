# Rendering paths

This repository has two ways of getting a MapLibre frame onto a Slint surface.
They are separate build targets and they do not share code. Which one you can
use is decided by a constraint in Slint, so that comes first.

## What Slint can accept

Slint's C++ `Image` takes a borrowed GPU texture through exactly one entry
point:

```cpp
slint::Image::create_from_borrowed_gl_2d_rgba_texture(...)
```

That is the whole surface as of Slint 1.17. There is no equivalent for a WebGPU,
Vulkan or Metal texture. Everything else reaches Slint as pixels in host memory,
through `slint::SharedPixelBuffer`.

Two things follow, and both surprise people:

- A frame can only skip the copy when MapLibre renders it with OpenGL. The
  texture handed over has to be a GL texture, because that is the only kind
  Slint will take.
- While MapLibre renders on the WebGPU backend, which is the default here, there
  is no way to hand Slint the result directly. `readStillImage()` and a copy are
  not a shortcut anyone took; they are the only route available.

So "WebGPU" and "no copy" cannot both be true today. This repository's default
is the first one.

## The two paths

### `mbgl-slint`, the reusable library

- `cpp/src/slint_maplibre_headless.cpp`, exported as
  `maplibre-native-slint::mbgl-slint`, used by `cpp/main.cpp`.
- MapLibre renders into `mbgl::HeadlessFrontend`. The frame comes back through
  `readStillImage()` as an `mbgl::PremultipliedImage`, is copied into a
  `slint::SharedPixelBuffer`, and reaches the UI as `MMapAdapter.frame`.
  `MMapView` displays it and forwards interactions back.
- WebGPU by default. Linux, macOS and Windows.
- This is what the README tells you to link, and what an application outside
  this repository should use.

### `maplibre-slint-gl`, the OpenGL example

- `cpp/main_gl.cpp` with `cpp/src/slint_map_gl.cpp` and
  `cpp/src/slint_gl_backend.cpp`, built as an executable rather than a library.
- MapLibre renders into an FBO on a GL context shared with Slint's renderer, and
  the colour texture is handed over with
  `create_from_borrowed_gl_2d_rgba_texture`. No readback.
- OpenGL, and in practice Linux with KMS. It was built for a Raspberry Pi 4 on
  the vc4 connector, where the V3D GPU is usable.
- It is example code, not a packaged target. Projects using it include its
  sources rather than linking a library.

Pick the first unless you are on a device where the second one's constraints are
already true. If you need a combination neither offers, use
[maplibre-native-rs](https://github.com/maplibre/maplibre-native-rs) directly
rather than bending this repository around it.

## What the display output decides

Slint's constraint above says what a frame has to be. A second constraint, on
the device, says whether a GL context worth handing over exists at all -- and it
is not the SoC that decides it.

The temptation on a Raspberry Pi is to conclude that the GPU cannot run
maplibre-native, because the software-GL setup is the one most people meet
first. It cannot in that configuration. It can in another:

| Display output path | GL context | Result |
|---|---|---|
| SPI panel, or any panel with no KMS GL path | its own, on `EGL_PLATFORM=surfaceless` | software GL (llvmpipe), pixels read back |
| HDMI on `vc4-kms-v3d`, through GBM/DRM | borrowed from Slint's linuxkms backend | V3D hardware GL, handed over with no copy |

The difference is where the context comes from. A backend that creates its own
context on the surfaceless platform has no hardware display to bind it to, so
Mesa's software rasterizer is the only thing left. A backend that borrows the
context Slint's linuxkms backend already made through GBM/DRM gets the GPU
behind that display.

Neither direction generalises. An SPI panel has no KMS GL path at all, by
hardware design, whatever the SoC. HDMI on a Pi 4 usually does, but "HDMI" on
its own is not the guarantee: `vc4-kms-v3d` has to be in use, and the context
has to come from that display.

`maplibre-slint-gl` is the second row, verified on a Raspberry Pi 4 (Debian
trixie, aarch64) with an HDMI panel on the console over DRM/KMS; see
[`cpp/README.md`](../cpp/README.md#raspberry-pi-notes). The first row is what
[`experiments/rust/RASPBERRY_PI.md`](../experiments/rust/RASPBERRY_PI.md)
describes.

## Platform status

Reusable Slint component with the C++ backend:

- Linux x86_64: good. Regularly exercised, and the best-supported development path.
- Windows x64: good. Working desktop path.
- macOS Apple Silicon: good. Working desktop path.

Reusable Slint component with the Rust backend, under `experiments/rust`:

- Linux x86_64: experimental. The best place to validate the Rust path.
- Windows x64 and macOS Apple Silicon: not practical, blocked by
  `maplibre-native-rs` maturity.

This is why the C++ backend remains the canonical reference implementation here.

## Build backends

WebGPU through `wgpu-native` is the default on Linux, Windows and macOS.

```bash
# Default desktop build
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Explicit WebGPU / wgpu-native
cmake -B build -DCMAKE_BUILD_TYPE=Release -DMLN_WITH_WEBGPU=ON -DMLN_WEBGPU_IMPL_WGPU=ON

# macOS Metal fallback / comparison
cmake -B build -DCMAKE_BUILD_TYPE=Release -DMLN_WITH_METAL=ON -DMLN_WITH_WEBGPU=OFF -G Xcode

# Explicit OpenGL fallback
cmake -B build -DCMAKE_BUILD_TYPE=Release -DMLN_WITH_WEBGPU=OFF -DMLN_WITH_OPENGL=ON
```

WebGPU defaults to on, so deselecting it is what picks another backend. Passing
`-DMLN_WITH_OPENGL=OFF` on its own leaves WebGPU enabled and does not produce
the Metal build it looks like it asks for.

## What would change this

If Slint gains a way to accept a texture from WebGPU, the default path could
hand its frame over directly and the copy would go. Until then, the choice
described above is the whole picture, and an integration that tries to avoid the
copy on the default path is working against the toolkit rather than around a
missing optimisation here.
