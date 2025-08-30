#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_OSX || TARGET_OS_IPHONE

#include <QuartzCore/CAMetalLayer.hpp>
#include <QuartzCore/CAMetalDrawable.hpp>
#include <Metal/MTLDevice.hpp>
#include <Metal/MTLCommandQueue.hpp>
#include <Metal/MTLRenderPass.hpp>
#include <Metal/MTLCommandBuffer.hpp>
#include <Metal/MTLBlitPass.hpp>

#include "slint_metal_renderer_backend.hpp"

#include <mbgl/mtl/texture2d.hpp>
#include <mbgl/mtl/renderable_resource.hpp>
#include <iostream>
#include <cassert>

namespace slint_metal {
using namespace mbgl::mtl;

namespace {
class SlintMetalRenderableResource final : public mbgl::mtl::RenderableResource {
public:
    SlintMetalRenderableResource(MetalRendererBackend& backend_, CA::MetalLayer* layer_)
        : backend(backend_), layer(layer_), commandQueue(NS::TransferPtr(backend.getDevice()->newCommandQueue())) {
        assert(layer);
        layer->setDevice(backend.getDevice().get());
    }

    void setBackendSize(mbgl::Size size_) {
        if (size != size_) {
            size = size_;
            layer->setDrawableSize(CGSizeMake(size.width, size.height));
            buffersInvalid = true;
            // Reset drawable on size change for proper zero-copy rendering
            resetDrawable();
        }
    }

    mbgl::Size getSize() const { return size; }

    void bind() override {
        // Don't get a new drawable if we already have one for this frame
        if (surface) {
            return;
        }
        
        // Get a fresh drawable for this frame
        CA::MetalDrawable* tmpDrawable = layer->nextDrawable();
        if (!tmpDrawable) {
            std::cerr << "Metal: nextDrawable() returned nil" << std::endl;
            commandBuffer = NS::RetainPtr(commandQueue->commandBuffer());
            return; // empty frame
        }
        // Store the drawable
        surface = NS::RetainPtr(tmpDrawable);
        // Note: NS::RetainPtr already handles retain/release, no need for additional CFRetain
        MTL::Texture* externalTex = surface->texture();
        backend.setExternalDrawable(externalTex);

        CGSize ds = layer->drawableSize();
        mbgl::Size texSize{static_cast<uint32_t>(ds.width), static_cast<uint32_t>(ds.height)};
        commandBuffer = NS::RetainPtr(commandQueue->commandBuffer());
        renderPassDescriptor = NS::TransferPtr(MTL::RenderPassDescriptor::alloc()->init());
        renderPassDescriptor->colorAttachments()->object(0)->setTexture(externalTex);
        renderPassDescriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
        renderPassDescriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
        // Use a neutral background color that matches typical map backgrounds
        renderPassDescriptor->colorAttachments()->object(0)->setClearColor(MTL::ClearColor{0.95, 0.95, 0.95, 1.0}); // Light gray

        if (buffersInvalid || !depthTexture || !stencilTexture) {
            buffersInvalid = false;
            depthTexture = backend.getContext().createTexture2D();
            depthTexture->setSize(texSize);
            depthTexture->setFormat(mbgl::gfx::TexturePixelType::Depth, mbgl::gfx::TextureChannelDataType::Float);
            depthTexture->setSamplerConfiguration({mbgl::gfx::TextureFilterType::Linear,
                                                   mbgl::gfx::TextureWrapType::Clamp,
                                                   mbgl::gfx::TextureWrapType::Clamp});
            static_cast<Texture2D*>(depthTexture.get())->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite | MTL::TextureUsageRenderTarget);

            stencilTexture = backend.getContext().createTexture2D();
            stencilTexture->setSize(texSize);
            stencilTexture->setFormat(mbgl::gfx::TexturePixelType::Stencil, mbgl::gfx::TextureChannelDataType::UnsignedByte);
            stencilTexture->setSamplerConfiguration({mbgl::gfx::TextureFilterType::Linear,
                                                     mbgl::gfx::TextureWrapType::Clamp,
                                                     mbgl::gfx::TextureWrapType::Clamp});
            static_cast<Texture2D*>(stencilTexture.get())->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite | MTL::TextureUsageRenderTarget);
        }
        if (depthTexture) {
            depthTexture->create();
            if (auto* depthTarget = renderPassDescriptor->depthAttachment()) {
                depthTarget->setTexture(static_cast<Texture2D*>(depthTexture.get())->getMetalTexture());
                depthTarget->setLoadAction(MTL::LoadActionClear);
                depthTarget->setClearDepth(1.0);
                depthTarget->setStoreAction(MTL::StoreActionStore);
            }
        }
        if (stencilTexture) {
            stencilTexture->create();
            if (auto* stencilTarget = renderPassDescriptor->stencilAttachment()) {
                stencilTarget->setTexture(static_cast<Texture2D*>(stencilTexture.get())->getMetalTexture());
                stencilTarget->setLoadAction(MTL::LoadActionClear);
                stencilTarget->setClearStencil(0);
                stencilTarget->setStoreAction(MTL::StoreActionStore);
            }
        }
    }

    void swap() override {
        if (surface && commandBuffer) {
            commandBuffer->presentDrawable(surface.get());
        }
        if (commandBuffer) {
            commandBuffer->commit();
        }
        commandBuffer.reset();
        renderPassDescriptor.reset();
        // NS::RetainPtr handles release automatically
        // Reset drawable after presentation to comply with Metal rules
        // Each drawable can only be presented once
        surface.reset();
        // Clear the cached drawable texture to ensure fresh drawable on next frame
        backend.setExternalDrawable(nullptr);
    }

    // Method to reset drawable when needed (e.g., on resize)
    void resetDrawable() {
        surface.reset();
    }

    const mbgl::mtl::RendererBackend& getBackend() const override { return backend; }
    const MTLCommandBufferPtr& getCommandBuffer() const override { return commandBuffer; }
    MTLBlitPassDescriptorPtr getUploadPassDescriptor() const override { return NS::RetainPtr(MTL::BlitPassDescriptor::alloc()->init()); }
    const MTLRenderPassDescriptorPtr& getRenderPassDescriptor() const override { return renderPassDescriptor; }

private:
    MetalRendererBackend& backend;
    CA::MetalLayer* layer;
    MTLCommandQueuePtr commandQueue;
    MTLCommandBufferPtr commandBuffer;
    MTLRenderPassDescriptorPtr renderPassDescriptor;
    CAMetalDrawablePtr surface;
    mbgl::gfx::Texture2DPtr depthTexture;
    mbgl::gfx::Texture2DPtr stencilTexture;
    mbgl::Size size{0,0};
    bool buffersInvalid = true;
};
} // namespace

MetalRendererBackend::MetalRendererBackend(CA::MetalLayer* layer)
    : mbgl::mtl::RendererBackend(mbgl::gfx::ContextMode::Unique),
      mbgl::gfx::Renderable(mbgl::Size{0,0}, std::make_unique<SlintMetalRenderableResource>(*this, layer)) {}

MetalRendererBackend::~MetalRendererBackend() = default;

void MetalRendererBackend::setSize(mbgl::Size size_) {
    this->getResource<SlintMetalRenderableResource>().setBackendSize(size_);
    // Update the protected size member of Renderable
    this->size = size_;
}

mbgl::Size MetalRendererBackend::getSize() const {
    return this->getResource<SlintMetalRenderableResource>().getSize();
}

} // namespace slint_metal

#endif // TARGET_OS_OSX || TARGET_OS_IPHONE
#endif // __APPLE__
