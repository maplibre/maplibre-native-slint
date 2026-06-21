#pragma once

#include <cstdint>
#include <mbgl/gfx/renderable.hpp>
#include <mbgl/gl/renderable_resource.hpp>
#include <mbgl/gl/renderer_backend.hpp>
#include <mbgl/renderer/renderer.hpp>
#include <mbgl/renderer/renderer_frontend.hpp>
#include <mbgl/util/util.hpp>
#include <memory>

// Custom GL backend that renders maplibre-native into an FBO owned by Slint's
// GL context (zero-copy: the FBO color texture is handed to Slint as a borrowed
// texture). Mirrors GLFWGLBackend but targets our FBO instead of framebuffer 0.

class SlintGLBackend;

class SlintGLRenderableResource final : public mbgl::gl::RenderableResource {
public:
    explicit SlintGLRenderableResource(SlintGLBackend& backend_)
        : backend(backend_) {
    }

    void bind() override;
    void swap() override {
    }

private:
    SlintGLBackend& backend;
};

class SlintGLBackend final : public mbgl::gl::RendererBackend,
                             public mbgl::gfx::Renderable {
public:
    explicit SlintGLBackend(mbgl::Size s)
        : mbgl::gl::RendererBackend(mbgl::gfx::ContextMode::Shared),
          mbgl::gfx::Renderable(
              s, std::make_unique<SlintGLRenderableResource>(*this)) {
    }

    ~SlintGLBackend() override = default;

    // gfx::RendererBackend
    mbgl::gfx::Renderable& getDefaultRenderable() override {
        return *this;
    }

    // Our FBO management
    void setFbo(uint32_t f) {
        fbo_ = f;
    }
    uint32_t fbo() const {
        return fbo_;
    }
    mbgl::Size getSize() const {
        return size;
    }
    void setSize(mbgl::Size s) {
        size = s;
    }

protected:
    // gfx::RendererBackend - Slint's context is already current, so no-ops.
    void activate() override {
    }
    void deactivate() override {
    }

    // gl::RendererBackend
    mbgl::gl::ProcAddress getExtensionFunctionPointer(
        const char* name) override;
    void updateAssumedState() override {
        assumeFramebufferBinding(fbo_);
        assumeViewport(0, 0, size);
    }

private:
    uint32_t fbo_ = 0;
};

// RendererFrontend mirroring GLFWRendererFrontend, but render() is driven
// explicitly from Slint's BeforeRendering callback.
class SlintGLFrontend final : public mbgl::RendererFrontend {
public:
    SlintGLFrontend(std::unique_ptr<mbgl::Renderer> renderer_,
                    mbgl::gfx::RendererBackend& backend_)
        : backend(backend_), renderer(std::move(renderer_)) {
    }

    ~SlintGLFrontend() override = default;

    void reset() override {
        renderer.reset();
    }

    void setObserver(mbgl::RendererObserver& observer) override {
        if (renderer)
            renderer->setObserver(&observer);
    }

    void update(std::shared_ptr<mbgl::UpdateParameters> params) override {
        updateParameters = std::move(params);
    }

    const mbgl::TaggedScheduler& getThreadPool() const override {
        return backend.getThreadPool();
    }

    void render();

    mbgl::Renderer* getRenderer() {
        return renderer.get();
    }

private:
    mbgl::gfx::RendererBackend& backend;
    std::unique_ptr<mbgl::Renderer> renderer;
    std::shared_ptr<mbgl::UpdateParameters> updateParameters;
};
