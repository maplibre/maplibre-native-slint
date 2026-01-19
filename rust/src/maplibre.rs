use std::cell::RefCell;
use std::sync::Arc;

use slint::ComponentHandle;

use crate::MapWindow;
use crate::MapAdapter;

mod headless;
pub use headless::create_map;
use headless::MapLibre;

/// Initialize UI callbacks and map interactions
pub fn init(ui: &MapWindow, map: &Arc<RefCell<MapLibre>>) {
    ui.on_window_size_changed({
        let map = Arc::downgrade(map);
        move || {
            if let Some(map) = map.upgrade() {
                // Window size changed - could be used for responsive layout
            }
        }
    });

    ui.on_map_size_changed({
        let map = Arc::downgrade(map);
        move || {
            // Map size changed - renderer is already resized by the UI
        }
    });

    ui.global::<MapAdapter>().on_tick_map_loop({
        let map = Arc::downgrade(map);
        let ui_handle = ui.as_weak();
        move || {
            if let Some(map_arc) = map.upgrade() {
                let mut map = map_arc.borrow_mut();
                map.render_once();
                if map.updated() {
                    let image = map.read_still_image();
                    let img_buffer = image.as_image();
                    let img = slint::SharedPixelBuffer::<slint::Rgba8Pixel>::clone_from_slice(
                        img_buffer.as_raw(),
                        img_buffer.width(),
                        img_buffer.height(),
                    );
                    if let Some(ui) = ui_handle.upgrade() {
                        ui.global::<MapAdapter>()
                            .set_map_texture(slint::Image::from_rgba8(img));
                    }
                }
            }
        }
    });

    ui.global::<MapAdapter>().on_mouse_press({
        let map = Arc::downgrade(map);
        move |x: f32, y: f32| {
            if let Some(map_arc) = map.upgrade() {
                // TODO: Handle mouse press on map
            }
        }
    });

    ui.global::<MapAdapter>().on_mouse_release({
        let map = Arc::downgrade(map);
        move |x: f32, y: f32| {
            if let Some(map_arc) = map.upgrade() {
                // TODO: Handle mouse release on map
            }
        }
    });

    ui.global::<MapAdapter>().on_mouse_move({
        let map = Arc::downgrade(map);
        move |x: f32, y: f32, _shift: bool| {
            if let Some(map_arc) = map.upgrade() {
                // TODO: Handle mouse move on map
            }
        }
    });

    ui.global::<MapAdapter>().on_double_click_with_shift({
        let map = Arc::downgrade(map);
        move |_x: f32, _y: f32, _shift: bool| {
            if let Some(map_arc) = map.upgrade() {
                // TODO: Handle double click with shift on map
            }
        }
    });

    ui.global::<MapAdapter>().on_wheel_zoom({
        let map = Arc::downgrade(map);
        move |_x: f32, _y: f32, _delta: f32| {
            if let Some(map_arc) = map.upgrade() {
                // TODO: Handle wheel zoom on map
            }
        }
    });

    ui.global::<MapAdapter>().on_style_changed({
        let map = Arc::downgrade(map);
        move |style_url: slint::SharedString| {
            if let Some(map_arc) = map.upgrade() {
                let mut map = map_arc.borrow_mut();
                map.load_style(&style_url);
            }
        }
    });

    ui.global::<MapAdapter>().on_fly_to({
        let map = Arc::downgrade(map);
        move |_location: slint::SharedString| {
            if let Some(map_arc) = map.upgrade() {
                // TODO: Implement fly_to for different locations
                // - "paris" -> lat: 48.8566, lon: 2.3522
                // - "new_york" -> lat: 40.7128, lon: -74.0060
                // - "tokyo" -> lat: 35.6762, lon: 139.6503
            }
        }
    });

    ui.global::<MapAdapter>().on_pitch_changed({
        let map = Arc::downgrade(map);
        move |_pitch: i32| {
            if let Some(map_arc) = map.upgrade() {
                // TODO: Handle pitch change
            }
        }
    });

    ui.global::<MapAdapter>().on_bearing_changed({
        let map = Arc::downgrade(map);
        move |_bearing: f32| {
            if let Some(map_arc) = map.upgrade() {
                // TODO: Handle bearing change
            }
        }
    });
}
