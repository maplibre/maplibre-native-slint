#include <EGL/egl.h>
#include <stdio.h>

#include "backend.h"

// The session needs an EGL display, a config and a context to share with, and
// creates its own context from them. We create that share context but never
// make it current: a context made current on the thread the UI toolkit's
// renderer owns is exactly the conflict cpp/ needed HostGLContextGuard for,
// and after Slint's renderer initialises eglMakeCurrent on ours fails anyway.

static EGLDisplay g_display = EGL_NO_DISPLAY;
static EGLConfig g_config = NULL;
static EGLContext g_share_context = EGL_NO_CONTEXT;

const char* backend_name(void) {
    return "EGL";
}

int backend_setup(void) {
    g_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g_display == EGL_NO_DISPLAY) {
        printf("FAIL no EGL display\n");
        return 1;
    }
    if (!eglInitialize(g_display, NULL, NULL)) {
        printf("FAIL eglInitialize (0x%x)\n", eglGetError());
        return 1;
    }
    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        printf("FAIL eglBindAPI (0x%x)\n", eglGetError());
        return 1;
    }

    const EGLint config_attribs[] = {EGL_RENDERABLE_TYPE,
                                     EGL_OPENGL_ES3_BIT,
                                     EGL_SURFACE_TYPE,
                                     EGL_PBUFFER_BIT,
                                     EGL_RED_SIZE,
                                     8,
                                     EGL_GREEN_SIZE,
                                     8,
                                     EGL_BLUE_SIZE,
                                     8,
                                     EGL_ALPHA_SIZE,
                                     8,
                                     EGL_NONE};
    EGLint num_configs = 0;
    if (!eglChooseConfig(g_display, config_attribs, &g_config, 1,
                         &num_configs) ||
        num_configs != 1) {
        printf("FAIL eglChooseConfig (0x%x)\n", eglGetError());
        return 1;
    }

    const EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    g_share_context =
        eglCreateContext(g_display, g_config, EGL_NO_CONTEXT, context_attribs);
    if (g_share_context == EGL_NO_CONTEXT) {
        printf("FAIL eglCreateContext (0x%x)\n", eglGetError());
        return 1;
    }
    return 0;
}

mln_status backend_attach(mln_map map, unsigned width, unsigned height,
                          mln_render_session* out_session) {
    mln_opengl_owned_texture_descriptor descriptor =
        mln_opengl_owned_texture_descriptor_default();
    descriptor.extent.width = width;
    descriptor.extent.height = height;
    descriptor.extent.scale_factor = 1.0;
    descriptor.context.platform = MLN_OPENGL_CONTEXT_PLATFORM_EGL;
    descriptor.context.data.egl.display = g_display;
    descriptor.context.data.egl.config = g_config;
    descriptor.context.data.egl.share_context = g_share_context;
    descriptor.context.data.egl.get_proc_address = (void*)eglGetProcAddress;
    return mln_opengl_owned_texture_attach(map, &descriptor, out_session);
}

void backend_report_host_context(const char* when) {
    printf("thread's current EGL context %s: %p\n", when,
           (void*)eglGetCurrentContext());
}

void backend_release_current(void) {
    eglMakeCurrent(g_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void backend_begin_render(void) {
    // Nothing of ours to make current; the session owns its context.
}

void backend_end_render(void) {
    // Nothing to restore.
}
