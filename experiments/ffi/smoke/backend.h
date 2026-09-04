#pragma once

// One smoke test, two graphics backends. maplibre-native-ffi ships a separate
// prebuilt per backend (linux-x64-egl, linux-x64-vulkan) and each takes a
// different context descriptor, so the host-side setup is what varies and the
// map flow in smoke.c is shared.

#include <maplibre_native_c.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Human-readable name of the backend this build links against. */
const char* backend_name(void);

/**
 * Brings up the host graphics context, standing in for the one Slint's
 * renderer owns. Returns 0 on success.
 */
int backend_setup(void);

/** Attaches an owned-texture render session of the given size to the map. */
mln_status backend_attach(mln_map map, unsigned width, unsigned height,
                          mln_render_session* out_session);

/**
 * Prints whatever "the host's context" means for this backend, so the caller
 * can compare before and after tearing the session down. EGL has a current
 * context per thread that a careless backend can clobber; Vulkan has no such
 * thread-current state, so there the print is informational only.
 */
void backend_report_host_context(const char* when);

/**
 * Releases whatever this backend made current on the calling thread, so a UI
 * toolkit can bring up its own context on a thread it expects to own. A no-op
 * where the backend has no thread-current state.
 */
void backend_release_current(void);

/**
 * Makes this backend's context current for a render, remembering whatever the
 * host had current, and puts the host's back afterwards. This is the same
 * save-and-restore cpp/'s HostGLContextGuard does, on this side of the C
 * boundary: the released artifact does not manage context currency itself.
 * No-ops where the backend has no thread-current state.
 */
void backend_begin_render(void);
void backend_end_render(void);

#ifdef __cplusplus
}
#endif
