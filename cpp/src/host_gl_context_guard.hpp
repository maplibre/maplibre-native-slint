#pragma once

namespace mbgl_slint {

// Preserves the windowing toolkit's GL context across calls into MapLibre.
//
// The WebGPU backend enumerates *every* wgpu backend while creating its
// instance, and wgpu's GLES probe binds and then unbinds an EGL context on the
// calling thread -- eglMakeCurrent(..., EGL_NO_CONTEXT) -- without restoring
// whatever was current before. Slint's femtovg renderer makes its context
// current once when the window is created and assumes it stays that way, so a
// MapLibre call issued from inside a Slint layout or draw pass leaves femtovg
// talking to no context at all: glGenTextures() writes back 0 and femtovg
// panics with "Unable to create Texture object".
//
// Capturing the current context on entry and rebinding it on exit keeps that
// churn invisible to the host toolkit. The guard is a no-op when no GL context
// is current, when the context survived the call unchanged, or when the
// process has no EGL/GLX/WGL loaded at all.
class HostGLContextGuard {
public:
    HostGLContextGuard();
    ~HostGLContextGuard();

    HostGLContextGuard(const HostGLContextGuard&) = delete;
    HostGLContextGuard& operator=(const HostGLContextGuard&) = delete;

private:
    void* egl_display = nullptr;
    void* egl_context = nullptr;
    void* egl_draw = nullptr;
    void* egl_read = nullptr;

    void* glx_display = nullptr;
    void* glx_context = nullptr;
    unsigned long glx_draw = 0;
    unsigned long glx_read = 0;

    void* wgl_device_context = nullptr;
    void* wgl_context = nullptr;
};

}  // namespace mbgl_slint
