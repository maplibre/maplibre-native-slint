#import <Foundation/Foundation.h>
#import <QuartzCore/CAMetalLayer.h>
#include <QuartzCore/CAMetalLayer.hpp>
#include <QuartzCore/CAMetalDrawable.hpp>
#include <Metal/MTLDevice.hpp>
#include <Metal/MTLCommandQueue.hpp>
#include <Metal/MTLCommandBuffer.hpp>
#include <Metal/MTLRenderCommandEncoder.hpp>
#include <Metal/MTLRenderPass.hpp>

#include "slint_metal_maplibre.hpp"
#include "slint_metal_renderer_backend.hpp"
#include <mbgl/renderer/renderer.hpp>
#include <mbgl/map/map.hpp>
#include <mbgl/map/map_options.hpp>
#include <mbgl/map/map_observer.hpp>
#include <mbgl/storage/resource_options.hpp>
#include <mbgl/style/style.hpp>
#include <mbgl/util/run_loop.hpp>
#include <mbgl/renderer/renderer_frontend.hpp>
#include <chrono>
#include <mbgl/renderer/update_parameters.hpp>
#include <mbgl/actor/scheduler.hpp>
#include <mbgl/renderer/renderer_observer.hpp>
#include <mbgl/gfx/backend_scope.hpp>
#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif
// (Optional) C helpers could go here
#ifdef __cplusplus
}
#endif

namespace slint_metal {

// Private impl will later host MapLibre objects; currently empty.
namespace {
// Minimal RendererFrontend that directly owns mbgl::Renderer and updates it synchronously.
class SlintMetalRendererFrontend : public mbgl::RendererFrontend {
public:
    SlintMetalRendererFrontend(std::unique_ptr<mbgl::Renderer> renderer_, mbgl::gfx::RendererBackend& backend_)
        : renderer(std::move(renderer_)), backend(backend_) {}

    void reset() override { renderer.reset(); }
    void setObserver(mbgl::RendererObserver& observer) override { if (renderer) renderer->setObserver(&observer); }
    void update(std::shared_ptr<mbgl::UpdateParameters> params) override {
        // Store parameters for later rendering
        pendingParameters = std::move(params);
        // Don't render immediately - wait for explicit renderNow() call
    }
    const mbgl::TaggedScheduler& getThreadPool() const override { return tagged(); }
    static const mbgl::TaggedScheduler& tagged() {
        static mbgl::TaggedScheduler sched{ mbgl::Scheduler::GetBackground(), mbgl::util::SimpleIdentity() };
        return sched;
    }
    
    // Method to perform synchronous rendering with stored parameters
    void renderNow() {
        if (renderer && pendingParameters) {
            // Create BackendScope for Metal context management
            const mbgl::gfx::BackendScope scope(backend, mbgl::gfx::BackendScope::ScopeType::Implicit);
            renderer->render(pendingParameters);
            // Don't reset parameters - they might be needed for next frame
        }
    }
    
    bool hasParameters() const { return pendingParameters != nullptr; }
    mbgl::Renderer* get() { return renderer.get(); }
private:
    std::unique_ptr<mbgl::Renderer> renderer;
    std::shared_ptr<mbgl::UpdateParameters> pendingParameters;
    mbgl::gfx::RendererBackend& backend;
};
} // namespace

struct SlintMetalMapLibre::Impl : public mbgl::MapObserver, public mbgl::RendererObserver {
    std::unique_ptr<mbgl::util::RunLoop> runLoop;
    std::unique_ptr<slint_metal::MetalRendererBackend> backend;
    std::unique_ptr<SlintMetalRendererFrontend> frontend;
    std::unique_ptr<mbgl::Map> map;
    mbgl::MapOptions mapOptions;
    mbgl::ResourceOptions resourceOptions;
    mbgl::ClientOptions clientOptions;
    bool styleLoaded = false;

    Impl() {
        // Configure resource options for proper tile loading
        resourceOptions.withCachePath("cache.sqlite")
                      .withAssetPath(".");
        
        // Configure client options
        clientOptions.withName("SlintMetalMapLibre")
                    .withVersion("1.0.0");
        
        runLoop = std::make_unique<mbgl::util::RunLoop>();
    }

    void ensureMap(CA::MetalLayer* layer, int w, int h, float pixelRatio) {
        if (!backend) {
            backend = std::make_unique<slint_metal::MetalRendererBackend>(layer);
        }
        // Backend needs physical pixel dimensions for the drawable
        uint32_t physicalWidth = static_cast<uint32_t>(w * pixelRatio);
        uint32_t physicalHeight = static_cast<uint32_t>(h * pixelRatio);
        backend->setSize({physicalWidth, physicalHeight});
        
        if (!frontend) {
            auto renderer = std::make_unique<mbgl::Renderer>(*backend, pixelRatio, std::nullopt);
            frontend = std::make_unique<SlintMetalRendererFrontend>(std::move(renderer), *backend);
            // Register this Impl as the renderer observer
            frontend->setObserver(*this);
        }
        if (!map) {
            // Map uses logical dimensions with pixel ratio
            mapOptions.withMapMode(mbgl::MapMode::Continuous)
                     .withSize({static_cast<uint32_t>(w), static_cast<uint32_t>(h)})
                     .withPixelRatio(pixelRatio)
                     .withConstrainMode(mbgl::ConstrainMode::None)
                     .withViewportMode(mbgl::ViewportMode::Default);
            map = std::make_unique<mbgl::Map>(*frontend, *this, mapOptions, resourceOptions, clientOptions);
            // Start with a closer zoom to ensure tiles load
            mbgl::CameraOptions cam; 
            cam.withCenter(mbgl::LatLng{48.8566,2.3522}); 
            cam.withZoom(5.0);  // Increased from 2.0 for better initial tile loading
            map->jumpTo(cam);
        } else { 
            map->setSize({static_cast<uint32_t>(w), static_cast<uint32_t>(h)}); 
        }
    }

    // mbgl::MapObserver
    // MapObserver
    void onDidFinishLoadingStyle() override {
        styleLoaded = true;
        std::cout << "[MapObserver] Style loaded successfully!" << std::endl;
        if (owner && owner->m_request_frame) { 
            owner->m_request_frame(); 
        }
    }
    void onDidFinishRenderingFrame(const mbgl::MapObserver::RenderFrameStatus& status) override {
        // Only log significant events to reduce spam
        static int frameCount = 0;
        static int continuousRepaints = 0;
        
        if (++frameCount % 60 == 0 || status.placementChanged) {
            std::cout << "[MapObserver] Frame " << frameCount << " - needsRepaint: " << status.needsRepaint 
                      << ", placementChanged: " << status.placementChanged << std::endl;
        }
        
        // Stop continuous rendering after a reasonable number of frames
        if (status.needsRepaint) {
            continuousRepaints++;
            // Only request more frames if we haven't been continuously repainting forever
            if (continuousRepaints < 120 && owner && owner->m_request_frame) { 
                owner->m_request_frame(); 
            }
        } else {
            continuousRepaints = 0;  // Reset counter when no repaint needed
        }
    }
    void onDidFailLoadingMap(mbgl::MapLoadError error, const std::string& message) override {
        std::cout << "[MapObserver] Failed to load map error " << static_cast<int>(error) << ": " << message << std::endl;
        // Try a different style
        if (owner && owner->m_impl && owner->m_impl->map) {
            std::cout << "[MapObserver] Trying fallback style" << std::endl;
            owner->m_impl->map->getStyle().loadURL("https://demotiles.maplibre.org/style.json");
        }
    }
    void onDidFailLoadingTile(const std::string& tileId, const std::string& error) {
        std::cout << "[MapObserver] Failed to load tile " << tileId << ": " << error << std::endl;
        // Don't spam the console with tile loading errors
        static int tileErrorCount = 0;
        if (++tileErrorCount <= 5) {
            std::cout << "[MapObserver] Tile loading errors detected - check network connectivity" << std::endl;
        }
    }
    
    // mbgl::RendererObserver methods
    void onInvalidate() override {
        std::cout << "[Observer] Invalidated - requesting frame" << std::endl;
        if (owner && owner->m_request_frame) {
            owner->m_request_frame();
        }
    }
    
    void onResourceError(std::exception_ptr error) override {
        try {
            if (error) {
                std::rethrow_exception(error);
            }
        } catch (const std::exception& e) {
            std::cerr << "[RendererObserver] Resource error: " << e.what() << std::endl;
        }
    }
    
    void onWillStartRenderingMap() override {
        std::cout << "[RendererObserver] Will start rendering map" << std::endl;
    }
    
    void onWillStartRenderingFrame() override {
        // Don't log every frame to avoid spam
        static int frameCount = 0;
        if (++frameCount % 60 == 0) {
            std::cout << "[RendererObserver] Will start rendering frame " << frameCount << std::endl;
        }
    }
    
    void onDidFinishRenderingFrame(mbgl::RendererObserver::RenderMode mode,
                                   bool repaint,
                                   bool placementChanged,
                                   const mbgl::gfx::RenderingStats& stats) override {
        // Log occasionally to track rendering progress
        static int frameCount = 0;
        static int continuousRepaints = 0;
        
        if (++frameCount % 60 == 0) {
            std::cout << "[RendererObserver] Finished rendering frame " << frameCount 
                      << " (mode: " << (mode == mbgl::RendererObserver::RenderMode::Full ? "Full" : "Partial")
                      << ", repaint: " << repaint
                      << ", placement: " << placementChanged << ")" << std::endl;
        }
        
        // Stop continuous rendering after a reasonable number of frames
        if (repaint) {
            continuousRepaints++;
            if (continuousRepaints < 120 && owner && owner->m_request_frame) { 
                owner->m_request_frame(); 
            }
        } else {
            continuousRepaints = 0;  // Reset counter when not continuously repainting
        }
    }
    
    SlintMetalMapLibre* owner = nullptr;
};

SlintMetalMapLibre::SlintMetalMapLibre() = default;
SlintMetalMapLibre::~SlintMetalMapLibre() {
    // Release retained Objective-C objects safely.
    if (m_layerPtr) {
        // ARC handles actual release since we only stored raw pointer.
        m_layerPtr = nullptr;
    }
    if (m_impl) { m_impl->owner = nullptr; }
}

void SlintMetalMapLibre::initialize(int logicalWidth, int logicalHeight, float devicePixelRatio) {
    std::cout << "[SlintMetalMapLibre] initialize(" << logicalWidth << "," << logicalHeight << ", dpr=" << devicePixelRatio << ")" << std::endl;
    m_logicalWidth = logicalWidth;
    m_logicalHeight = logicalHeight;
    m_dpr = devicePixelRatio <= 0.f ? 1.f : devicePixelRatio;
    ensureLayer();
    resize(logicalWidth, logicalHeight, m_dpr);
    // Provide a default style - use a simple test style
    if (m_pendingStyleUrl.empty()) {
        // Use MapLibre demo tiles style
        m_pendingStyleUrl = "https://demotiles.maplibre.org/style.json";
    }
    // Kick off first frame if requested
    if (m_request_frame) m_request_frame();
}

void SlintMetalMapLibre::ensureLayer() {
    if (m_layerPtr) return;
    @autoreleasepool {
        MTL::Device* dev = MTL::CreateSystemDefaultDevice();
        if (!dev) {
            std::cerr << "[SlintMetalMapLibre] Failed to create Metal device!" << std::endl;
            return;
        }
        CA::MetalLayer* layer = CA::MetalLayer::layer();
        layer->setDevice(dev);
        layer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
        layer->setFramebufferOnly(false);
        
        // Set contents scale using Objective-C API for HiDPI support
        // metal-cpp doesn't expose setContentsScale, so we use ObjC
        ::CAMetalLayer* objcLayer = (__bridge ::CAMetalLayer*)layer;
        objcLayer.contentsScale = m_dpr;
        
        std::cout << "[SlintMetalMapLibre] Created CAMetalLayer with device: " << dev 
                  << " contentsScale: " << m_dpr << std::endl;
        
        m_layerPtr = static_cast<void*>(layer);
    }
}

void SlintMetalMapLibre::resize(int logicalWidth, int logicalHeight, float devicePixelRatio) {
    std::cout << "[SlintMetalMapLibre] resize(" << logicalWidth << "," << logicalHeight << ", dpr=" << devicePixelRatio << ")" << std::endl;
    m_logicalWidth = logicalWidth;
    m_logicalHeight = logicalHeight;
    m_dpr = devicePixelRatio <= 0.f ? 1.f : devicePixelRatio;
    if (!m_layerPtr) return;
    @autoreleasepool {
        CA::MetalLayer* layer = static_cast<CA::MetalLayer*>(m_layerPtr);
        
        // Update contents scale using Objective-C API for HiDPI
        ::CAMetalLayer* objcLayer = (__bridge ::CAMetalLayer*)layer;
        objcLayer.contentsScale = m_dpr;
        
        // Set drawable size to physical pixels
        CGSize drawableSize = CGSizeMake((CGFloat)m_logicalWidth * m_dpr,
                                         (CGFloat)m_logicalHeight * m_dpr);
        layer->setDrawableSize(drawableSize);
        std::cout << "[SlintMetalMapLibre] Updated layer contentsScale=" << m_dpr 
                  << " drawableSize=" << drawableSize.width << "x" << drawableSize.height << std::endl;
    }
}

void SlintMetalMapLibre::setStyleUrl(const std::string &url) {
    std::cout << "[SlintMetalMapLibre] setStyleUrl: " << url << std::endl;
    m_pendingStyleUrl = url;
    if (m_impl && m_impl->map) {
        std::cout << "[SlintMetalMapLibre] Loading style immediately" << std::endl;
        try {
            m_impl->map->getStyle().loadURL(url);
            m_styleApplied = true;
            std::cout << "[SlintMetalMapLibre] Style loadURL called successfully" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[SlintMetalMapLibre] Exception loading style: " << e.what() << std::endl;
        }
    } else {
        std::cout << "[SlintMetalMapLibre] Style URL set for later loading" << std::endl;
    }
}

bool SlintMetalMapLibre::render_once() {
    if (!m_layerPtr) {
        std::cout << "[SlintMetalMapLibre] render_once() - No layer pointer, skipping" << std::endl;
        return false;
    }
    
    // TEST: Simple Metal clear without MapLibre to verify rendering works
    static bool testMode = false;  // Keep disabled for normal operation
    if (testMode) {
        @autoreleasepool {
            CA::MetalLayer* layer = static_cast<CA::MetalLayer*>(m_layerPtr);
            CA::MetalDrawable* drawable = layer->nextDrawable();
            if (!drawable) {
                std::cout << "[TEST] No drawable available" << std::endl;
                return false;
            }
            
            MTL::Device* device = layer->device();
            if (!device) {
                std::cout << "[TEST] No device" << std::endl;
                return false;
            }
            
            MTL::CommandQueue* commandQueue = device->newCommandQueue();
            MTL::CommandBuffer* commandBuffer = commandQueue->commandBuffer();
            
            MTL::RenderPassDescriptor* renderPass = MTL::RenderPassDescriptor::alloc()->init();
            renderPass->colorAttachments()->object(0)->setTexture(drawable->texture());
            renderPass->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
            renderPass->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
            renderPass->colorAttachments()->object(0)->setClearColor(MTL::ClearColor{1.0, 0.0, 1.0, 1.0}); // Magenta
            
            MTL::RenderCommandEncoder* encoder = commandBuffer->renderCommandEncoder(renderPass);
            encoder->endEncoding();
            
            commandBuffer->presentDrawable(drawable);
            commandBuffer->commit();
            
            std::cout << "[TEST] Rendered magenta frame" << std::endl;
            
            renderPass->release();
            encoder->release();
            commandBuffer->release();
            commandQueue->release();
        }
        return true;
    }
    
    // Original MapLibre rendering code
    @autoreleasepool {
        CA::MetalLayer* layer = static_cast<CA::MetalLayer*>(m_layerPtr);
        if (!m_impl) { 
            std::cout << "[SlintMetalMapLibre] render_once() - Creating impl" << std::endl;
            m_impl = std::make_unique<Impl>(); 
            m_impl->owner = this; 
        }
        m_impl->ensureMap(layer, m_logicalWidth, m_logicalHeight, m_dpr);
        if (!m_impl->map) {
            std::cout << "[SlintMetalMapLibre] render_once() - No map, skipping" << std::endl;
            return false;
        }
        if (!m_styleApplied && !m_pendingStyleUrl.empty()) { 
            std::cout << "[SlintMetalMapLibre] render_once() - Loading style: " << m_pendingStyleUrl << std::endl;
            m_impl->map->getStyle().loadURL(m_pendingStyleUrl); 
            m_styleApplied = true; 
        }
    std::cout << "[SlintMetalMapLibre] render_once() -> processing frame" << std::endl;
    try {
        // Process any pending RunLoop tasks to handle map updates
        m_impl->runLoop->runOnce();
        
        // Trigger map to generate update parameters
        m_impl->map->triggerRepaint();
        
        // Process RunLoop again to handle the repaint request
        m_impl->runLoop->runOnce();
        
        // Now render if we have parameters
        if (m_impl->frontend->hasParameters()) {
            std::cout << "[SlintMetalMapLibre] Rendering frame with parameters" << std::endl;
            m_impl->frontend->renderNow();
        } else {
            std::cout << "[SlintMetalMapLibre] No update parameters available yet" << std::endl;
        }
    } catch (const std::exception& e) { 
        std::cerr << "[SlintMetalMapLibre] render_once() - Exception: " << e.what() << std::endl; 
        return false; 
    }
    }
    return true;
}


// --- Interaction methods (basic, similar semantics to CPU path) ---
void SlintMetalMapLibre::handle_mouse_press(float x, float y) {
    std::cout << "[Interaction] Mouse press at (" << x << ", " << y << ")" 
              << " (previous position was " << m_last_x << ", " << m_last_y << ")" << std::endl;
    // For a new drag, always reset to the press position
    m_last_x = x; 
    m_last_y = y; 
    m_pressed = true;
    m_move_count = 0;  // Reset move counter for new drag
}
void SlintMetalMapLibre::handle_mouse_release(float x, float y) { 
    std::cout << "[Interaction] Mouse release at (" << x << ", " << y << ")" << std::endl;
    // Always reset state on release
    m_pressed = false; 
    m_move_count = 0;
    // Update position to release point to maintain continuity
    m_last_x = x;
    m_last_y = y;
}
void SlintMetalMapLibre::handle_mouse_move(float x, float y, bool pressed) {
    if (!m_impl || !m_impl->map) {
        std::cout << "[Interaction] No map available for mouse move" << std::endl;
        return; 
    }
    
    // If we're not pressed but Slint thinks we are, sync the state
    if (!pressed && m_pressed) {
        std::cout << "[Interaction] Implicit mouse release detected" << std::endl;
        handle_mouse_release(x, y);
        return;
    }
    
    if (pressed && m_pressed) {
        m_move_count++;
        
        // Skip the first move after mouse_press to avoid jump
        // This is because Slint calls mouse_press after threshold, then immediately mouse_move
        if (m_move_count == 1) {
            std::cout << "[Interaction] First move after press, resetting position to (" << x << ", " << y << ")" << std::endl;
            m_last_x = x;
            m_last_y = y;
            return;
        }
        
        double dx = x - m_last_x; double dy = y - m_last_y; 
        
        // Only move if there's actual movement to avoid unnecessary updates
        if (std::abs(dx) > 0.1 || std::abs(dy) > 0.1) {
            std::cout << "[Interaction] Mouse move by (" << dx << ", " << dy << ")" << std::endl;
            
            // Use BackendScope for Metal context management during map operations
            try {
                const mbgl::gfx::BackendScope scope(*m_impl->backend, mbgl::gfx::BackendScope::ScopeType::Implicit);
                m_impl->map->moveBy({dx, dy});
                std::cout << "[Interaction] Map moved successfully" << std::endl;
                
                // Only update last position after successful move
                m_last_x = x; 
                m_last_y = y;
            } catch (const std::exception& e) {
                std::cerr << "[Interaction] Exception during map move: " << e.what() << std::endl;
                return;
            }
            
            // Request frame update during drag to prevent freezing
            if (m_request_frame) {
                std::cout << "[Interaction] Requesting frame update" << std::endl;
                m_request_frame();
            } else {
                std::cout << "[Interaction] No frame request callback available" << std::endl;
            }
        } else {
            // Update position even for small movements
            m_last_x = x; m_last_y = y;
        }
    }
}
void SlintMetalMapLibre::handle_double_click(float x, float y, bool shift) {
    if (!m_impl || !m_impl->map) return; auto cam = m_impl->map->getCameraOptions(); double currentZoom = cam.zoom.value_or(0.0); double delta = shift ? -1.0 : 1.0; double targetZoom = std::min(m_max_zoom, std::max(m_min_zoom, currentZoom + delta));
    mbgl::CameraOptions next; next.withCenter(m_impl->map->latLngForPixel(mbgl::ScreenCoordinate{x,y})); next.withZoom(targetZoom); m_impl->map->jumpTo(next);
    // Request frame update after zoom
    if (m_request_frame) m_request_frame();
}
void SlintMetalMapLibre::handle_wheel_zoom(float x, float y, float dy) {
    if (!m_impl || !m_impl->map) return;
    constexpr double step = 1.2;
    // Invert the zoom direction: scroll up (dy > 0) zooms in, scroll down (dy < 0) zooms out
    double scale = (dy > 0.f) ? step : (1.0/step);
    m_impl->map->scaleBy(scale, mbgl::ScreenCoordinate{x,y});
    // Request frame update after zoom
    if (m_request_frame) m_request_frame();
}
void SlintMetalMapLibre::fly_to(const std::string& location) {
    if (!m_impl || !m_impl->map) return;
    
    mbgl::LatLng target;
    double targetZoom = 12.0; // Good zoom level for city view
    
    if (location == "paris") {
        target = mbgl::LatLng{48.8566, 2.3522};
    } else if (location == "new_york") {
        target = mbgl::LatLng{40.7128, -74.0060};
    } else if (location == "tokyo") {
        target = mbgl::LatLng{35.6895, 139.6917};
    } else {
        return; // Unknown location
    }
    
    // Create camera options for the fly-to animation
    mbgl::CameraOptions cameraOptions;
    cameraOptions.withCenter(target);
    cameraOptions.withZoom(targetZoom);
    
    // Create animation options for smooth flight
    mbgl::AnimationOptions animationOptions;
    animationOptions.duration = std::chrono::milliseconds(2000); // 2 second animation
    
    // Use flyTo for smooth animated transition
    m_impl->map->flyTo(cameraOptions, animationOptions);
    
    std::cout << "[Interaction] Flying to " << location << std::endl;
    
    // Request frame update to start the animation
    if (m_request_frame) m_request_frame();
}

} // namespace slint_metal
