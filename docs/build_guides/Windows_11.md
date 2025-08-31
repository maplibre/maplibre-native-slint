````markdown
# Building on Windows 11

This guide walks you through building the **MapLibre Native + Slint** integration on Windows 11 with MSVC, CMake, Ninja, and vcpkg (manifest mode).

---

## System Requirements

- Windows 11 (x64)
- **Visual Studio 2022** (Desktop development with C++) / MSVC toolchain
- **CMake** (3.24+ recommended)
- **Ninja** (1.11+)
- **Git**
- **Rust** (stable) via rustup
- **vcpkg** (manifest mode; the project contains `vcpkg.json`)

> Rendering backend: **OpenGL (WGL)**

---

## 1) Install prerequisites

1. **Visual Studio 2022**

   - Install _Desktop development with C++_ workload.

2. **CMake / Ninja / Git**

   - Install from official installers or your preferred package manager.

3. **Rust (required by Slint)**

   - Install with rustup (the standard Rust toolchain manager on Windows).

4. **vcpkg**
   - Clone to a convenient path (e.g. `C:\src\vcpkg`) and bootstrap.

> Tip: It’s easiest to build from the **“x64 Native Tools Command Prompt for VS 2022”** so MSVC env vars are set.
> Note: VSCode integrated terminal does not work as expected...

---

## 2) Clone the repository

```bat
git clone https://github.com/maplibre/maplibre-native-slint.git
cd maplibre-native-slint
git submodule update --init --recursive
```

---

## 3) Configure vcpkg environment variables

```bat
:: Disable VS bundled vcpkg for this session only (to avoid warnings)
set VCPKG_ROOT=
:: Set overlay triplets to match the project (important)
set VCPKG_OVERLAY_TRIPLETS=%cd%\vendor\maplibre-native\platform\windows\vendor\vcpkg-custom-triplets
```

## 4) Configure with CMake (vcpkg manifest mode)

> The project uses **vcpkg manifest mode**. You **don’t** need to pass package names to `vcpkg install`; CMake will drive vcpkg using `vcpkg.json`.

```bat
rmdir /s /q build-ninja

cmake -S . -B build-ninja -G "Ninja" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DMLNS_WITH_SLINT_GL=ON ^
  -DMLNS_FORCE_FETCH_SLINT=ON ^
  -DCMAKE_CXX_COMPILER_LAUNCHER=C:\ccache\ccache-4.11.3-windows-x86_64\ccache.exe ^
  -DCMAKE_TOOLCHAIN_FILE=C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows ^
  -DVCPKG_HOST_TRIPLET=x64-windows
```

- On first configure, vcpkg will install the dependencies declared in `vcpkg.json`.
- Slint (C++ API) is **auto-fetched/built** by CMake if not found system-wide.

---

## 5) Build

```bat
cmake --build build-ninja -j
```

---

## 6) Run

The example app and DLLs are placed in the build directory:

```bat
set MLNS_DISABLE_GL_ZEROCOPY=1
set SLINT_RENDERER=software

build-ninja\maplibre-slint-example.exe
```

or enable GL renderer:

```bat
set MLNS_DISABLE_GL_ZEROCOPY=0
set SLINT_RENDERER=gl
set MLNS_FORCE_DESKTOP_GL=1
set MLNS_GL_SAFE_MODE=2

build-ninja\maplibre-slint-example.exe
```

---

## 7) Expected build outputs

- `maplibre-slint-example.exe` — Example application
- `_deps\slint-build\slint_cpp.dll` — Slint runtime (copied next to the executable)
- `slint_generated_map_window_1.cpp` / `map_window.h` — Generated Slint UI sources
- `vendor\maplibre-native\mbgl-*.lib` — MapLibre Native static libs from the build

---

## Notes

- **OpenGL headers** are provided by vcpkg (header-only _opengl-registry_, _egl-registry_).
- MSVC requires defining `_USE_MATH_DEFINES` before including `<cmath>` to get constants like `M_PI`. This project’s build already defines it, but it’s good to know.

---

## Troubleshooting

### “In manifest mode, `vcpkg install` does not support individual package arguments”

- Cause: running `vcpkg install zlib ...` in a manifest project.
- Fix: don’t pass package names. Let CMake drive vcpkg, or run `vcpkg install` **without arguments** in the repo root (it will read `vcpkg.json`).

### Unresolved externals for `uv_*` (libuv)

- Ensure your app target links **libuv**. If you customize or add targets, link the vcpkg CMake target for libuv, e.g.:

  ```cmake
  find_package(libuv CONFIG REQUIRED)
  target_link_libraries(your-target PRIVATE
      $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>)
  ```

  Check the vcpkg **usage** file under `vcpkg_installed/x64-windows/share/libuv/usage` for the exact target names exported by your version.

### Missing OpenGL / GLES headers

- Make sure your `vcpkg.json` includes (or your install contains) header-only ports:

  - `opengl-registry` (provides `GL/glcorearb.h`)
  - `egl-registry` (provides `EGL/egl.h`)

### Slint not found

- The build system will **auto-fetch** Slint if it’s not installed system-wide. If you prefer system-wide installs, follow Slint’s C++ docs and ensure `slint-cpp` is visible via CMake/PKG.

---

## Performance tips

- First build can take a while (MapLibre Native + Slint).
- Using Ninja and a recent MSVC significantly speeds up incremental builds.
- SSD and more RAM help with link times.

---

## Next steps

1. Start the sample: `maplibre-slint-example.exe`
2. Tweak the UI in `examples/map_window.slint`
3. Explore integration code in `src/slint_maplibre.cpp`
4. Adjust dependencies in `vcpkg.json` if you add features
````
