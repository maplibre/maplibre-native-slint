#pragma once

// Experimental Metal zero-copy scaffold for MapLibre + Slint (Option B)
// This is an initial skeleton that will be incrementally filled in.
// Currently it creates / owns a CAMetalLayer and exposes it so the example
// app can embed the layer alongside Slint UI. Rendering integration with
// mbgl::Renderer (Metal backend) will be added in subsequent steps.

#include <cstdint>
#include <memory>
#include <string>
#include <functional>

namespace slint_metal {

class SlintMetalMapLibre {
public:
    SlintMetalMapLibre();
    ~SlintMetalMapLibre();

    // Initialize (creates CAMetalLayer if not already). logical sizes are in logical (DPI independent) units.
    void initialize(int logicalWidth, int logicalHeight, float devicePixelRatio = 1.0f);

    // Resize layer (logical size * dpr -> drawableSize)
    void resize(int logicalWidth, int logicalHeight, float devicePixelRatio = 1.0f);

    // Set remote style (no-op until renderer wired)
    void setStyleUrl(const std::string &url);

    // Interaction hooks (mirroring CPU path for future parity)
    void handle_mouse_press(float x, float y);
    void handle_mouse_release(float x, float y);
    void handle_mouse_move(float x, float y, bool pressed);
    void handle_double_click(float x, float y, bool shift);
    void handle_wheel_zoom(float x, float y, float dy);
        void fly_to(const std::string& location);
    
        // Provide a callback that will be invoked when the map backend requests another frame.
        void set_frame_request_callback(std::function<void()> cb) { m_request_frame = std::move(cb); }

    // Trigger one render (stub for now). Returns true if a frame was issued.
    bool render_once();


    // Expose underlying CAMetalLayer pointer (as void*) for embedding in native view hierarchy.
    void* layerPtr() const { return m_layerPtr; }

    // Raw pointer to underlying CAMetalLayer (opaque outside Objective-C++ translation unit).
    void *metalLayer() const { return m_layerPtr; }

    // Whether Metal layer scaffold is valid.
    bool valid() const { return m_layerPtr != nullptr; }

private:
    void ensureLayer();

    void *m_layerPtr = nullptr; // CA::MetalLayer* (retained) - created/owned here
    int m_logicalWidth = 0;
    int m_logicalHeight = 0;
    float m_dpr = 1.0f;

    // Interaction state
    double m_min_zoom = 0.0;
    double m_max_zoom = 22.0;
    float m_last_x = 0.f;
    float m_last_y = 0.f;
    bool m_pressed = false;
    int m_move_count = 0;

    // Style management
    std::string m_pendingStyleUrl;
    bool m_styleApplied = false;

    // Future placeholders for MapLibre objects (to be added in next increment):
    struct Impl;
    std::unique_ptr<Impl> m_impl; // will hold mbgl::Map, backend, renderer, etc.
        std::function<void()> m_request_frame;
};

} // namespace slint_metal
