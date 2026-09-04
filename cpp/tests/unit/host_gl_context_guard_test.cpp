#include "host_gl_context_guard.hpp"

#include <EGL/egl.h>
#include <gtest/gtest.h>

#include "slint_maplibre_headless.hpp"

// Exercises the RAII contract directly, without a window or a wgpu instance:
// an off-screen EGL context stands in for the one Slint's renderer makes
// current when it creates its window, and an explicit unbind stands in for
// wgpu's GLES probe.
class HostGLContextGuardTest : public ::testing::Test {
protected:
    void SetUp() override {
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display == EGL_NO_DISPLAY) {
            GTEST_SKIP() << "No EGL display available.";
        }

        EGLint major = 0;
        EGLint minor = 0;
        if (!eglInitialize(display, &major, &minor)) {
            display = EGL_NO_DISPLAY;
            GTEST_SKIP() << "eglInitialize() failed.";
        }

        if (!eglBindAPI(EGL_OPENGL_ES_API)) {
            GTEST_SKIP() << "eglBindAPI(EGL_OPENGL_ES_API) failed.";
        }

        const EGLint config_attribs[] = {EGL_RENDERABLE_TYPE,
                                         EGL_OPENGL_ES2_BIT, EGL_SURFACE_TYPE,
                                         EGL_PBUFFER_BIT, EGL_NONE};
        EGLint num_configs = 0;
        EGLConfig config = nullptr;
        if (!eglChooseConfig(display, config_attribs, &config, 1,
                             &num_configs) ||
            num_configs != 1) {
            GTEST_SKIP() << "No suitable EGL config.";
        }

        const EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2,
                                          EGL_NONE};
        context =
            eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribs);
        if (context == EGL_NO_CONTEXT) {
            GTEST_SKIP() << "eglCreateContext() failed.";
        }

        const EGLint surface_attribs[] = {EGL_WIDTH, 8, EGL_HEIGHT, 8,
                                          EGL_NONE};
        surface = eglCreatePbufferSurface(display, config, surface_attribs);
        if (surface == EGL_NO_SURFACE) {
            GTEST_SKIP() << "eglCreatePbufferSurface() failed.";
        }

        ASSERT_TRUE(eglMakeCurrent(display, surface, surface, context));
        ASSERT_EQ(eglGetCurrentContext(), context);
    }

    void TearDown() override {
        if (display == EGL_NO_DISPLAY) {
            return;
        }
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (surface != EGL_NO_SURFACE) {
            eglDestroySurface(display, surface);
        }
        if (context != EGL_NO_CONTEXT) {
            eglDestroyContext(display, context);
        }
    }

    EGLDisplay display = EGL_NO_DISPLAY;
    EGLContext context = EGL_NO_CONTEXT;
    EGLSurface surface = EGL_NO_SURFACE;
};

// The failure this guard exists for: something unbinds the context mid-scope
// and never puts it back.
TEST_F(HostGLContextGuardTest, RebindsAContextUnboundInsideTheScope) {
    {
        mbgl_slint::HostGLContextGuard guard;

        ASSERT_TRUE(eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                                   EGL_NO_CONTEXT));
        ASSERT_EQ(eglGetCurrentContext(), EGL_NO_CONTEXT);
    }

    EXPECT_EQ(eglGetCurrentContext(), context);
    EXPECT_EQ(eglGetCurrentSurface(EGL_DRAW), surface);
    EXPECT_EQ(eglGetCurrentSurface(EGL_READ), surface);
}

TEST_F(HostGLContextGuardTest, LeavesAnUntouchedContextBound) {
    { mbgl_slint::HostGLContextGuard guard; }

    EXPECT_EQ(eglGetCurrentContext(), context);
    EXPECT_EQ(eglGetCurrentSurface(EGL_DRAW), surface);
}

// Nothing current on entry means nothing to restore: the guard must not bind
// a context of its own behind the caller's back.
TEST_F(HostGLContextGuardTest, StaysOutOfTheWayWithNoCurrentContext) {
    ASSERT_TRUE(eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                               EGL_NO_CONTEXT));

    {
        mbgl_slint::HostGLContextGuard guard;
        ASSERT_TRUE(eglMakeCurrent(display, surface, surface, context));
    }

    EXPECT_EQ(eglGetCurrentContext(), context);
}

TEST_F(HostGLContextGuardTest, NestsWithoutLosingTheContext) {
    {
        mbgl_slint::HostGLContextGuard outer;
        {
            mbgl_slint::HostGLContextGuard inner;
            ASSERT_TRUE(eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                                       EGL_NO_CONTEXT));
        }
        EXPECT_EQ(eglGetCurrentContext(), context);
    }

    EXPECT_EQ(eglGetCurrentContext(), context);
}

// End to end over the calls that reach the WebGPU backend. Construction is
// what clobbered the context in the original crash; destruction tears the
// backend down again, and as a member of SlintMapLibre the frontend outlives
// the destructor body unless it is reset explicitly.
TEST_F(HostGLContextGuardTest, MapLifecycleKeepsTheHostContextBound) {
    {
        SlintMapLibre map;
        map.initialize(64, 64);
        EXPECT_EQ(eglGetCurrentContext(), context)
            << "initialize() left the host without its context";

        map.run_map_loop();
        EXPECT_EQ(eglGetCurrentContext(), context)
            << "run_map_loop() left the host without its context";

        map.resize(128, 128);
        EXPECT_EQ(eglGetCurrentContext(), context)
            << "resize() left the host without its context";
    }

    EXPECT_EQ(eglGetCurrentContext(), context)
        << "~SlintMapLibre() left the host without its context";
}
