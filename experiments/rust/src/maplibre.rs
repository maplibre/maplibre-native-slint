use std::cell::RefCell;
use std::rc::Rc;

use slint::ComponentHandle;

use crate::MMapAdapter;
use crate::MapWindow;

mod headless;
use headless::MapCamera;
use headless::MapLibre;
pub use headless::create_map;

fn push_camera_state(ui: &MapWindow, camera: MapCamera) {
    let adapter = ui.global::<MMapAdapter>();
    adapter.set_current_lat(camera.lat as f32);
    adapter.set_current_lon(camera.lon as f32);
    adapter.set_current_zoom(camera.zoom as f32);
    adapter.set_current_bearing(camera.bearing as f32);
    adapter.set_current_pitch(camera.pitch as f32);
}

fn push_frame(ui: &MapWindow, map: &mut MapLibre) {
    let image = map.read_still_image();
    let img_buffer = image.as_image();
    let img = slint::SharedPixelBuffer::<slint::Rgba8Pixel>::clone_from_slice(
        img_buffer.as_raw(),
        img_buffer.width(),
        img_buffer.height(),
    );
    ui.global::<MMapAdapter>()
        .set_frame(slint::Image::from_rgba8(img));
}

/// Initialize UI callbacks and map interactions
pub fn init(ui: &MapWindow, map: &Rc<RefCell<MapLibre>>) {
    let ui_handle = ui.as_weak();

    ui.on_map_size_changed({
        let map = Rc::downgrade(map);
        let ui_handle = ui_handle.clone();
        move || {
            if let (Some(map), Some(ui)) = (map.upgrade(), ui_handle.upgrade()) {
                let mut map = map.borrow_mut();
                map.resize(ui.get_map_size());
                map.mark_dirty();
            }
        }
    });

    ui.global::<MMapAdapter>().on_tick({
        let map = Rc::downgrade(map);
        let ui_handle = ui_handle.clone();
        move || {
            if let Some(map) = map.upgrade() {
                let mut map = map.borrow_mut();
                map.render_once();
                if map.take_frame_updated() {
                    if let Some(ui) = ui_handle.upgrade() {
                        push_frame(&ui, &mut map);
                        push_camera_state(&ui, map.camera());
                        ui.global::<MMapAdapter>()
                            .set_style_loaded(map.style_loaded());
                        ui.global::<MMapAdapter>().set_map_idle(map.map_idle());
                    }
                }
            }
        }
    });

    ui.global::<MMapAdapter>().on_mouse_pressed({
        let map = Rc::downgrade(map);
        move |x: f32, y: f32| {
            if let Some(map) = map.upgrade() {
                map.borrow_mut().mouse_pressed(x, y);
            }
        }
    });

    ui.global::<MMapAdapter>().on_mouse_released({
        let map = Rc::downgrade(map);
        move |_x: f32, _y: f32| {
            if let Some(map) = map.upgrade() {
                map.borrow_mut().mouse_released();
            }
        }
    });

    ui.global::<MMapAdapter>().on_mouse_moved({
        let map = Rc::downgrade(map);
        move |x: f32, y: f32| {
            if let Some(map) = map.upgrade() {
                map.borrow_mut().mouse_moved(x, y);
            }
        }
    });

    ui.global::<MMapAdapter>().on_double_clicked({
        let map = Rc::downgrade(map);
        move |_x: f32, _y: f32, shift: bool| {
            if let Some(map) = map.upgrade() {
                map.borrow_mut().double_clicked(shift);
            }
        }
    });

    ui.global::<MMapAdapter>().on_wheel_zoomed({
        let map = Rc::downgrade(map);
        move |_x: f32, _y: f32, delta: f32| {
            if let Some(map) = map.upgrade() {
                map.borrow_mut().wheel_zoomed(delta);
            }
        }
    });

    ui.global::<MMapAdapter>().on_request_style_change({
        let map = Rc::downgrade(map);
        move |style_url: slint::SharedString| {
            if let Some(map) = map.upgrade() {
                let mut map = map.borrow_mut();
                map.load_style(&style_url);
            }
        }
    });

    ui.global::<MMapAdapter>().on_request_fly_to({
        let map = Rc::downgrade(map);
        move |lat: f32, lon: f32, zoom: f32| {
            if let Some(map) = map.upgrade() {
                map.borrow_mut()
                    .fly_to(f64::from(lat), f64::from(lon), f64::from(zoom));
            }
        }
    });

    ui.global::<MMapAdapter>().on_request_pitch_change({
        let map = Rc::downgrade(map);
        move |pitch: f32| {
            if let Some(map) = map.upgrade() {
                map.borrow_mut().set_pitch(f64::from(pitch));
            }
        }
    });

    ui.global::<MMapAdapter>().on_request_bearing_change({
        let map = Rc::downgrade(map);
        move |bearing: f32| {
            if let Some(map) = map.upgrade() {
                map.borrow_mut().set_bearing(f64::from(bearing));
            }
        }
    });
}
