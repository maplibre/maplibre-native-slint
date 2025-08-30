// Metal-only example entry point with simplified layer scheduling
#import <AppKit/AppKit.h>
#import <QuartzCore/CAMetalLayer.h>
#import <QuartzCore/CAMetalLayer.hpp>
#import <Metal/Metal.h>
#include <iostream>
#include <memory>
#include <vector>
#include <atomic>
#include <optional>
#include <CoreFoundation/CoreFoundation.h>
#include <mach/mach_time.h>

#include "map_window.h"
#include "metal/slint_metal_maplibre.hpp"

// Find the NSWindow created by Slint
static NSWindow* findNSWindowForSlint(const MapWindow &) {
    @autoreleasepool {
        NSApplication* app = [NSApplication sharedApplication];
        for (NSWindow* window in [app windows]) {
            // Return the first window (assuming it's the Slint window)
            return window;
        }
    }
    return nullptr;
}

struct MetalFrameSchedulerCFRunLoop {
    static std::atomic<bool>& needFrame() { static std::atomic<bool> v{false}; return v; }
    static std::atomic<bool>& rendering() { static std::atomic<bool> v{false}; return v; }
    static std::weak_ptr<slint_metal::SlintMetalMapLibre>& weakMap() { static std::weak_ptr<slint_metal::SlintMetalMapLibre> w; return w; }
    static CFRunLoopTimerRef& timer() { static CFRunLoopTimerRef t = nullptr; return t; }
    static std::atomic<uint64_t>& lastFrameNS() { static std::atomic<uint64_t> v{0}; return v; }
    static uint64_t targetIntervalNS() { return 16'666'666ull; } // 60Hz
    
    static void setup(const std::shared_ptr<slint_metal::SlintMetalMapLibre>& map) {
        weakMap() = map;
        if (timer() == nullptr) {
            CFRunLoopTimerContext ctx{};
            timer() = CFRunLoopTimerCreate(nullptr,
                                          CFAbsoluteTimeGetCurrent() + 0.016, // Start in 16ms
                                          0.016, // Fire every 16ms (60Hz)
                                          0,
                                          0,
                                          [](CFRunLoopTimerRef, void*) {
                                              auto sp = weakMap().lock(); if (!sp) return;
                                              if (!needFrame().load(std::memory_order_relaxed)) return;
                                              
                                              bool expected = false;
                                              if (!rendering().compare_exchange_strong(expected, true)) return;
                                              
                                              // Don't log every frame to reduce console spam
                                              static int renderCount = 0;
                                              if (++renderCount % 60 == 0) {
                                                  std::cout << "[FrameScheduler] Frame " << renderCount << std::endl;
                                              }
                                              
                                              // Clear the needFrame flag before rendering to prevent duplicate renders
                                              needFrame().store(false, std::memory_order_relaxed);
                                              
                                              sp->render_once();
                                              lastFrameNS().store(mach_absolute_time());
                                              rendering().store(false);
                                              
                                              // Check if another frame is needed immediately (for smooth interaction)
                                              if (needFrame().load(std::memory_order_relaxed)) {
                                                  // Schedule another frame immediately if needed
                                                  CFRunLoopTimerSetNextFireDate(timer(), CFAbsoluteTimeGetCurrent());
                                              }
                                          }, &ctx);
            CFRunLoopAddTimer(CFRunLoopGetMain(), timer(), kCFRunLoopCommonModes);
        }
    }
    
    static void request(const std::shared_ptr<slint_metal::SlintMetalMapLibre>& map) {
        // Set the flag atomically to ensure thread safety
        bool expected = false;
        if (needFrame().compare_exchange_strong(expected, true, std::memory_order_relaxed)) {
            // Only schedule if we successfully set the flag (prevents duplicate scheduling)
            setup(map);
        }
    }
    
    static void teardown() {
        if (timer()) {
            CFRunLoopRemoveTimer(CFRunLoopGetMain(), timer(), kCFRunLoopCommonModes);
            CFRelease(timer());
            timer() = nullptr;
        }
    }
};

int main(int argc, char** argv) {
    std::cout << "[main] Starting application (Metal layer embedding)" << std::endl;
    auto main_window = MapWindow::create();
    auto slint_map_libre = std::make_shared<slint_metal::SlintMetalMapLibre>();

    auto initialized = std::make_shared<bool>(false);


    // Frame requests come from MapLibre invalidate callbacks; no periodic tick needed
    // Connect backend frame request to scheduler
    slint_map_libre->set_frame_request_callback([weak = std::weak_ptr<slint_metal::SlintMetalMapLibre>(slint_map_libre)](){ if (auto sp = weak.lock()) { MetalFrameSchedulerCFRunLoop::request(sp); } });
    main_window->global<MapAdapter>().on_style_changed([=](const slint::SharedString& url) { slint_map_libre->setStyleUrl(std::string(url.data(), url.size())); });
    main_window->global<MapAdapter>().on_mouse_press([=](float x, float y) { slint_map_libre->handle_mouse_press(x,y); });
    main_window->global<MapAdapter>().on_mouse_release([=](float x, float y) { slint_map_libre->handle_mouse_release(x,y); });
    main_window->global<MapAdapter>().on_mouse_move([=](float x, float y, bool pressed) { slint_map_libre->handle_mouse_move(x,y,pressed); });
    main_window->global<MapAdapter>().on_double_click_with_shift([=](float x,float y,bool shift){ slint_map_libre->handle_double_click(x,y,shift); });
    main_window->global<MapAdapter>().on_wheel_zoom([=](float x,float y,float dy){ slint_map_libre->handle_wheel_zoom(x,y,dy); });
    main_window->global<MapAdapter>().on_fly_to([=](const slint::SharedString& loc){ slint_map_libre->fly_to(std::string(loc.data(),loc.size())); });

    // Adjusted to new snake_case property naming in .slint (map_size)
    main_window->on_map_size_changed([=]() {
        auto s = main_window->get_map_size();
        int w = static_cast<int>(s.width);
        int h = static_cast<int>(s.height);
        
        // Get the actual DPR from the NSWindow
        float dpr = 1.0f;
        NSWindow* window = findNSWindowForSlint(*main_window);
        if (window) {
            dpr = static_cast<float>(window.backingScaleFactor);
        }
        
        if (w>0 && h>0) {
            if (!*initialized) {
                slint_map_libre->initialize(w,h,dpr);
                
                // Attach the CAMetalLayer to the NSWindow
                std::cout << "[main] Window found: " << (window ? "yes" : "no") << std::endl;
                std::cout << "[main] MapLibre valid: " << (slint_map_libre->valid() ? "yes" : "no") << std::endl;
                if (window && slint_map_libre->valid()) {
                    @autoreleasepool {
                        CAMetalLayer* metalLayer = static_cast<CAMetalLayer*>(slint_map_libre->metalLayer());
                        std::cout << "[main] Metal layer: " << (metalLayer ? "yes" : "no") << std::endl;
                        if (metalLayer) {
                            std::cout << "[main] Creating Metal view..." << std::endl;
                            std::cout << "[main] Window content bounds: " << window.contentView.bounds.size.width 
                                      << "x" << window.contentView.bounds.size.height << std::endl;
                            std::cout << "[main] Map size from Slint: " << w << "x" << h << std::endl;
                            std::cout << "[main] ContentView isFlipped: " << (window.contentView.isFlipped ? "YES" : "NO") << std::endl;
                            
                            // Calculate the frame for the map area only (not the entire window)
                            // Slint lays out: controls at top (~82px), map below (518px of 600px total)
                            // The black area at bottom in the screenshot is where Slint's map Rectangle is
                            // We need to position our Metal view to match that location
                            
                            CGFloat windowHeight = window.contentView.bounds.size.height;
                            CGFloat controlsHeight = windowHeight - h;  // Controls take up the difference
                            
                            NSRect mapFrame;
                            mapFrame.origin.x = 0;
                            
                            // Check if the coordinate system is flipped
                            if (window.contentView.isFlipped) {
                                // If flipped, y=0 is at top, so map should start after controls
                                mapFrame.origin.y = controlsHeight;  // Start after controls
                            } else {
                                // If not flipped, y=0 is at bottom (standard macOS)
                                mapFrame.origin.y = 0;  // Bottom of window
                            }
                            
                            mapFrame.size.width = window.contentView.bounds.size.width;
                            mapFrame.size.height = h;  // Map height (518px)
                            
                            std::cout << "[main] Positioning map: y=" << mapFrame.origin.y 
                                      << " height=" << mapFrame.size.height 
                                      << " (controls height=" << controlsHeight << ")" << std::endl;
                            
                            // Create a dedicated NSView for the Metal layer
                            NSView* metalView = [[NSView alloc] initWithFrame:mapFrame];
                            metalView.wantsLayer = YES;
                            metalView.layerContentsRedrawPolicy = NSViewLayerContentsRedrawOnSetNeedsDisplay;
                            metalView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
                            
                            // Check existing subviews
                            NSArray* subviews = window.contentView.subviews;
                            std::cout << "[main] Existing subviews count: " << subviews.count << std::endl;
                            
                            // Find if there's a Slint view we should position relative to
                            NSView* slintView = nil;
                            if (subviews.count > 0) {
                                slintView = subviews[0];  // Slint's view should be the first one
                                std::cout << "[main] Found existing view frame: " 
                                          << slintView.frame.origin.x << "," << slintView.frame.origin.y 
                                          << " size: " << slintView.frame.size.width << "x" << slintView.frame.size.height << std::endl;
                            }
                            
                            // Set the Metal layer on the view
                            metalView.layer = metalLayer;
                            
                            // Slint uses winit which renders to a CAMetalLayer
                            // We need to insert our Metal view at the correct position
                            // The black area at bottom is Slint's transparent Rectangle where map should be
                            
                            // Insert the Metal view behind Slint's content
                            [window.contentView addSubview:metalView positioned:NSWindowBelow relativeTo:nil];
                            
                            std::cout << "[main] Setting frame..." << std::endl;
                            // Position the Metal layer within its view
                            metalLayer.frame = metalView.bounds;
                            metalLayer.autoresizingMask = kCALayerWidthSizable | kCALayerHeightSizable;
                            
                            std::cout << "[main] Configuring layer properties..." << std::endl;
                            // Set contents scale for HiDPI support
                            metalLayer.contentsScale = dpr;
                            // Set drawable size to match the physical pixels
                            metalLayer.drawableSize = CGSizeMake(w * dpr, h * dpr);
                            
                            // Make the layer visible
                            metalLayer.hidden = NO;
                            metalLayer.opaque = YES;
                            
                            std::cout << "[main] Attached CAMetalLayer size=" << w << "x" << h 
                                      << " dpr=" << dpr << std::endl;
                        }
                    }
                } else {
                    std::cout << "[main] Failed to attach CAMetalLayer - window or layer not available" << std::endl;
                }
                
                std::cout << "[main] Initialized maplibre size=" << w << "x" << h << std::endl;
                std::cout << "[main] Expect warmup frames while tiles load" << std::endl;
                *initialized = true;
            } else {
                slint_map_libre->resize(w,h,dpr);
                // Update Metal view frame on resize
                if (window && window.contentView.subviews.count > 0) {
                    @autoreleasepool {
                        // Find the Metal view (should be the last subview we added)
                        NSView* metalView = nil;
                        for (NSView* view in window.contentView.subviews) {
                            if (view.layer && [view.layer isKindOfClass:[CAMetalLayer class]]) {
                                metalView = view;
                                break;
                            }
                        }
                        
                        if (metalView) {
                            // Update the Metal view frame to match the map area
                            CGFloat windowHeight = window.contentView.bounds.size.height;
                            CGFloat controlsHeight = windowHeight - h;  // Controls take up the difference
                            
                            NSRect mapFrame;
                            mapFrame.origin.x = 0;
                            
                            // Check if the coordinate system is flipped (it should be)
                            if (window.contentView.isFlipped) {
                                // If flipped, y=0 is at top, so map should start after controls
                                mapFrame.origin.y = controlsHeight;  // Start after controls
                            } else {
                                // If not flipped, y=0 is at bottom (standard macOS)
                                mapFrame.origin.y = 0;  // Bottom of window
                            }
                            
                            mapFrame.size.width = window.contentView.bounds.size.width;
                            mapFrame.size.height = h;  // Use the actual map height from Slint
                            metalView.frame = mapFrame;
                            
                            // Update the Metal layer properties
                            CAMetalLayer* metalLayer = (CAMetalLayer*)metalView.layer;
                            metalLayer.contentsScale = dpr;
                            metalLayer.drawableSize = CGSizeMake(w * dpr, h * dpr);
                        }
                    }
                }
                std::cout << "[main] Resized maplibre surface to " << w << "x" << h << std::endl;
                
                // Request a frame update to repaint after resize
                MetalFrameSchedulerCFRunLoop::request(slint_map_libre);
            }
        }
    });

    std::cout << "[main] Entering UI event loop" << std::endl;
    main_window->run();
    MetalFrameSchedulerCFRunLoop::teardown();
    // Teardown CFRunLoop observer if present
    // (Not strictly required since process exiting, but keeps tools quiet if reused)
    // NOTE: Accessing internal scheduler static state replicated here would require exposing it; skipped.
    return 0;
}
