#include "slint_gl_backend.hpp"

#include <EGL/egl.h>
#include <mbgl/gfx/backend_scope.hpp>
#include <mbgl/renderer/renderer.hpp>

void SlintGLRenderableResource::bind() {
    backend.setFramebufferBinding(backend.fbo());
    backend.setViewport(0, 0, backend.getSize());
}

mbgl::gl::ProcAddress SlintGLBackend::getExtensionFunctionPointer(
    const char* name) {
    return reinterpret_cast<mbgl::gl::ProcAddress>(eglGetProcAddress(name));
}

void SlintGLFrontend::render() {
    if (!renderer || !updateParameters)
        return;

    mbgl::gfx::BackendScope guard{backend,
                                  mbgl::gfx::BackendScope::ScopeType::Implicit};

    // Copy the shared pointer to keep params alive across render().
    auto params = updateParameters;
    renderer->render(params);
}
