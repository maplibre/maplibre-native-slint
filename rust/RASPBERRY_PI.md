# Running on Raspberry Pi / embedded Linux (KMS, software GL)

This guide describes how to run the Rust demo on a Raspberry Pi (or any
console-only Linux box with no X11/Wayland), rendering directly to a
DRM/KMS display such as a small SPI/HDMI LCD.

This is the **software GL** path. It is not the only one available on a Pi:
the C++ zero-copy GL backend renders on the Pi 4's V3D GPU over HDMI, and
[`cpp/README.md`](../cpp/README.md#raspberry-pi-notes) documents that setup.
Which one applies is decided by the display output path, not by the board
(see below).

Verified on:

- **Build host:** Raspberry Pi 5, Debian 12 (bookworm)
- **Display host:** Raspberry Pi 4, Debian 13 (trixie), 480x320 SPI/HDMI LCD
- Both `aarch64`. Map rendered live, including a **PMTiles** planet source.

## Why special handling is needed

- maplibre-native renders into an offscreen buffer here and creates **its
  own** EGL context to do it. On the **surfaceless** EGL platform there is
  no hardware display to bind that context to, so it must be created with
  **Mesa's software rasterizer (llvmpipe)**. Without this, maplibre-native
  aborts with `eglInitialize() failed`.
- There is no X11/Wayland on a console box, so Slint must use its
  **linuxkms** backend (the `-noseat` variant, which does not depend on
  `libseat`/logind).

### Software GL is a property of this configuration, not of the board

It is tempting to read the above as "the Pi's GPU cannot run
maplibre-native". It cannot, in this configuration. It can in another:

| Display output path | GL context | Result |
|---|---|---|
| SPI panel, or any panel with no KMS GL path | own context, `EGL_PLATFORM=surfaceless` | software GL (llvmpipe), pixels read back |
| HDMI on `vc4-kms-v3d`, via GBM/DRM | context borrowed from Slint's linuxkms backend | **V3D hardware GL**, composited zero-copy |

The second row is the C++ zero-copy backend, verified on a Raspberry Pi 4
(Debian trixie, aarch64) with an HDMI panel on the console over DRM/KMS;
see [`cpp/README.md`](../cpp/README.md#raspberry-pi-notes).

So what decides it is whether a KMS/DRM/V3D path to the screen actually
exists, and whether the GL context comes from it. An SPI panel has no such
path at all, by hardware design, whatever the SoC. HDMI on a Pi 4 usually
does, but "HDMI" alone is not the guarantee -- `vc4-kms-v3d` has to be in
use and the context has to come from that display.

## Build

```bash
cd rust
cargo build --release --features linuxkms-noseat
```

## Run

Run from the **active virtual terminal / console** (the process needs DRM
master on the KMS device):

```bash
SLINT_BACKEND=linuxkms-noseat \
EGL_PLATFORM=surfaceless \
LIBGL_ALWAYS_SOFTWARE=1 \
GALLIUM_DRIVER=llvmpipe \
./target/release/maplibre_native_slint
```

- `SLINT_BACKEND=linuxkms-noseat` — Slint draws straight to DRM/KMS (falls
  back to its software renderer when no GL surface is available).
- `EGL_PLATFORM=surfaceless` + `LIBGL_ALWAYS_SOFTWARE=1` +
  `GALLIUM_DRIVER=llvmpipe` — make maplibre-native's EGL context initialize
  via Mesa software GL (no display server, no hardware GLES3 required).

### Optional environment overrides (initial style / camera)

The demo reads these on startup so you can change the view without
rebuilding (handy for kiosk/headless setups):

| Variable | Effect | Example |
|---|---|---|
| `MAPLIBRE_STYLE_URL` | Initial style URL | `https://tile.openstreetmap.jp/styles/osm-bright-ja/style.json` |
| `MAPLIBRE_LAT` | Initial latitude | `35.68` |
| `MAPLIBRE_LON` | Initial longitude | `139.76` |
| `MAPLIBRE_ZOOM` | Initial zoom | `9` |

PMTiles styles work as-is — point `MAPLIBRE_STYLE_URL` at a style whose
source is `pmtiles://https://.../planet.pmtiles`.

## Cross-distro binaries (build host OS != display host OS)

If you build on one Debian release and run on another, a few versioned
system libraries will be missing on the target (different SONAMEs / runtime
version checks). Either build on the target, or bundle the build host's
copies and point `LD_LIBRARY_PATH` at them. Libraries observed to need
bundling for bookworm→trixie:

```
libicuuc.so.72  libicui18n.so.72  libicudata.so.72   # ICU 72 vs 76
libpng16.so.16                                        # runtime version assert
libuv.so.1
```

```bash
LD_LIBRARY_PATH="$HOME/mls-libs" SLINT_BACKEND=linuxkms-noseat \
  EGL_PLATFORM=surfaceless LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=llvmpipe \
  ./maplibre_native_slint
```

Do **not** bundle the GPU/display stack (libEGL/libGL/Mesa/libdrm/libgbm) —
those must come from the target so they match its kernel/driver.
