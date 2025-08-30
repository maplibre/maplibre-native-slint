// Minimal Metal renderer backend adapted from Qt backend (Option B)
// Provides a Renderable for MapLibre's Metal pipeline without Qt.

#pragma once

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_OSX || TARGET_OS_IPHONE

#include <Metal/MTLCommandBuffer.hpp>
#include <Metal/MTLCommandQueue.hpp>
#include <Metal/MTLDevice.hpp>
#include <Metal/MTLRenderPass.hpp>
#include <QuartzCore/CAMetalDrawable.hpp>
#include <QuartzCore/CAMetalLayer.hpp>
#include <mbgl/gfx/command_encoder.hpp>
#include <mbgl/gfx/context.hpp>
#include <mbgl/gfx/renderable.hpp>
#include <mbgl/mtl/renderable_resource.hpp>
#include <mbgl/mtl/renderer_backend.hpp>
#include <memory>

namespace slint_metal {

class MetalRendererBackend final : public mbgl::mtl::RendererBackend,
                                   public mbgl::gfx::Renderable {
public:
    explicit MetalRendererBackend(CA::MetalLayer* layer);
    ~MetalRendererBackend() override;

    // RendererBackend
    mbgl::gfx::Renderable& getDefaultRenderable() override {
        return static_cast<mbgl::gfx::Renderable&>(*this);
    }
    void activate() override {
    }
    void deactivate() override {
    }
    void updateAssumedState() override {
    }

    void setSize(mbgl::Size size_);
    mbgl::Size getSize() const;

    MTL::Texture* currentDrawableTexture() const {
        return m_currentDrawable;
    }
    void setExternalDrawable(MTL::Texture* tex) {
        m_currentDrawable = tex;
    }

    // No-op for compatibility; Metal path doesn't use FBO IDs the same way.
    void updateFramebuffer(uint32_t /*fbo*/, const mbgl::Size& /*size*/) {
    }

private:
    MTL::Texture* m_currentDrawable{
        nullptr};  // cached drawable texture pointer
};

}  // namespace slint_metal

#endif  // TARGET_OS_OSX || TARGET_OS_IPHONE
#endif  // __APPLE__
