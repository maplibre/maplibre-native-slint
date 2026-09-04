# maplibre-native-ffi (experiment)

> This is an experiment. Nothing here is guaranteed to work.
>
> - It is not wired into this repository's build or CI, and it does not replace `cpp/`.
> - It sits alongside `experiments/rust/` as an experiment, and will be deleted if it leads nowhere.
> - It stands on [maplibre-native-ffi](https://github.com/maplibre/maplibre-native-ffi),
>   itself an experimental C API whose ABI is declared unstable while
>   `mln_c_version()` returns `0`.
> - There is an unsolved problem right now: it does not coexist with Slint's
>   default GL renderer. See [Where it is stuck](#where-it-is-stuck).
> - Everything below was tried on one Linux x86_64 machine and nowhere else.

A place to find out whether the C API from
[maplibre-native-ffi](https://github.com/maplibre/maplibre-native-ffi) could
stand in for the C++ backend in `cpp/`.

- `smoke/` goes from style load to CPU readback with no window involved.
- `example/` puts those pixels in a Slint window as a `slint::Image`.

## Running it

EGL and Vulkan are both selectable. maplibre-native-ffi ships a separate
prebuilt per backend, so picking the artifact brings the matching host-side
setup with it.

```bash
# smoke, EGL
experiments/ffi/scripts/fetch-artifact.sh
cmake -S experiments/ffi/smoke -B experiments/ffi/build-egl -DMLN_FFI_TARGET=linux-x64-egl
cmake --build experiments/ffi/build-egl && ./experiments/ffi/build-egl/mln-ffi-smoke

# smoke, Vulkan
MLN_FFI_TARGET=linux-x64-vulkan experiments/ffi/scripts/fetch-artifact.sh
cmake -S experiments/ffi/smoke -B experiments/ffi/build-vulkan -DMLN_FFI_TARGET=linux-x64-vulkan
cmake --build experiments/ffi/build-vulkan && ./experiments/ffi/build-vulkan/mln-ffi-smoke

# the Slint example, which for now needs Slint on its software renderer
cmake -S experiments/ffi/example -B experiments/ffi/build-example -DMLN_FFI_TARGET=linux-x64-egl
cmake --build experiments/ffi/build-example
SLINT_BACKEND=winit-software ./experiments/ffi/build-example/mln-ffi-example
```

The example does not build Slint. It uses the `libslint_cpp.so` and
`slint-compiler` under `MLN_FFI_SLINT_BUILD_DIR`, which defaults to this
repository's `build/_deps/slint-build`, so configuring `cpp/` once is enough.

`linux-arm64-egl` and `linux-arm64-vulkan` select the same way, untried.
`experiments/ffi/third_party` and `experiments/ffi/build*` are not tracked.

## Results

On one machine (Ubuntu, NVIDIA RTX 3060):

| | EGL | Vulkan |
| --- | --- | --- |
| `smoke` | renders (15311 colours) | renders (15306 colours) |
| `example`, Slint software renderer | renders | untried |
| `example`, Slint femtovg (GL) | attach fails | untried |

The example loads a style, shows the map, and takes camera commands through
`mln_map_jump_to()`. `MLN_FFI_AUTOZOOM=1` jumps to zoom 3 at frame 200, which
exercises the camera path without driving the UI.

A note on the success check. Counting opaque pixels proves nothing, because a
background-only frame is opaque too. The first version of this test used
"alpha is not zero" and duly reported a sheet of ocean blue as a success. It
now counts distinct colours: single digits for a background, over ten thousand
for a map.

## What this shows

1. Nothing here builds maplibre-native. The prebuilt is a self-contained 14 MB
   `libmaplibre-native-c.so` whose only shared dependencies are libc, libm,
   libpthread and libdl, with a pkg-config file alongside it. The full
   `vendor/maplibre-native` build drops out entirely.
2. `mln_texture_read_premultiplied_rgba8()` gives the host-memory pixels a
   `slint::Image` needs, filling the role `HeadlessFrontend::readStillImage()`
   plays today.
3. `mln_runtime_pump()` and `mln_runtime_poll_event()` map cleanly onto the
   current `run_map_loop()` and MapObserver pair, with events arriving as an
   enum rather than virtual calls.

## Where it is stuck

The big one is that this does not coexist with Slint's GL renderer.

- With `SLINT_BACKEND=winit-software` everything works. In that configuration
  no EGL context is ever current on the thread, so there is nothing to collide
  with.
- With the default femtovg renderer, `mln_opengl_owned_texture_attach()`
  returns `MLN_STATUS_NATIVE_ERROR`. The same call succeeds before the Slint
  window is created, so the trigger is Slint's renderer initialising. While
  narrowing that down it also turned out that once Slint is up, `eglMakeCurrent`
  fails even on an EGL context created fresh afterwards.

The example currently creates a separate EGL context for MapLibre and passes it
as the share context, which is almost certainly the wrong shape. It should
either share the context Slint already owns, or let Slint own the texture and
use `mln_opengl_borrowed_texture_attach()`. That is the next thing to try.

## Things worth knowing

- The ABI is unstable while `mln_c_version()` returns `0`.
- The published prebuilt (`core/v0.202608.0`) is behind `main`, and the
  difference matters:
  - the second argument of `mln_render_session_render_update()` is `bool*` in
    the release and `mln_render_result*` on `main`;
  - GL context ownership (`mln_opengl_context_ownership`, shared or dedicated)
    exists on `main` but not in the released headers. The released library
    renders on whatever context it finds and does not manage currency itself,
    so the problem `HostGLContextGuard` solves stays on this side of the
    boundary in this version.
- `share_context` in `mln_opengl_owned_texture_descriptor` is required.
  Passing `EGL_NO_CONTEXT` returns `MLN_STATUS_INVALID_ARGUMENT`.
- There is no desktop Linux WebGPU prebuilt. EGL and Vulkan are published, and
  WebGPU only for emscripten. This repository defaults to WebGPU, so moving
  toward this route would change that assumption.
- `MAP_IDLE` never arrives in `MLN_MAP_MODE_CONTINUOUS`. `MAP_LOADING_FINISHED`
  is the signal that the style and its first tiles are in.
- The Vulkan path has the host create and lend a VkInstance, VkPhysicalDevice,
  VkDevice and graphics queue. With no ICD installed there are zero physical
  devices and it fails.
- Resizing the example's window does not resize the map.

## Next

1. Coexist with Slint's GL renderer, by sharing its context or rendering into a
   texture it owns.
2. Follow window resizes, and connect mouse pan and wheel zoom.
3. With those done, `cpp/`, `experiments/rust/` and `experiments/ffi/` can be compared side by side.
