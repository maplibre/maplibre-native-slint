use crate::Size;
use maplibre_native::ImageRenderer;
use maplibre_native::ImageRendererBuilder;
use maplibre_native::tile_server_options::TileServerOptions;
use maplibre_native::{CameraUpdate, LatLng, ResourceOptions};
use std::cell::RefCell;
use std::num::NonZeroU32;
use std::path::{Path, PathBuf};
use std::rc::Rc;

const DEFAULT_STYLE_URL: &str = "https://demotiles.maplibre.org/style.json";
const MIN_ZOOM: f64 = 0.0;
const MAX_ZOOM: f64 = 22.0;
const MIN_PITCH: f64 = 0.0;
const MAX_PITCH: f64 = 60.0;
const MAX_ABS_LAT: f64 = 85.0;
const WHEEL_STEP: f64 = 0.5;
const DOUBLE_CLICK_STEP: f64 = 1.0;

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct MapCamera {
    pub lat: f64,
    pub lon: f64,
    pub zoom: f64,
    pub bearing: f64,
    pub pitch: f64,
}

impl Default for MapCamera {
    fn default() -> Self {
        Self {
            lat: 0.0,
            lon: 0.0,
            zoom: 1.0,
            bearing: 0.0,
            pitch: 0.0,
        }
    }
}

#[derive(Debug)]
struct DragState {
    x: f32,
    y: f32,
}

pub struct MapLibre {
    renderer: Option<ImageRenderer<maplibre_native::Static>>,
    last_image: Option<maplibre_native::Image>,
    size: (u32, u32),
    style_url: String,
    camera: MapCamera,
    drag_state: Option<DragState>,
    style_loaded: bool,
    map_idle: bool,
    frame_updated: bool,
}

impl MapLibre {
    fn new_lazy(size: (u32, u32)) -> Self {
        Self {
            renderer: None,
            last_image: None,
            size,
            style_url: DEFAULT_STYLE_URL.to_owned(),
            camera: MapCamera::default(),
            drag_state: None,
            style_loaded: false,
            map_idle: false,
            frame_updated: false,
        }
    }

    pub fn camera(&self) -> MapCamera {
        self.camera
    }

    pub fn style_loaded(&self) -> bool {
        self.style_loaded
    }

    pub fn map_idle(&self) -> bool {
        self.map_idle
    }

    pub fn take_frame_updated(&mut self) -> bool {
        let updated = self.frame_updated;
        self.frame_updated = false;
        updated
    }

    pub fn mark_dirty(&mut self) {
        self.frame_updated = true;
        self.map_idle = false;
    }

    pub fn read_still_image(&mut self) -> &maplibre_native::Image {
        self.last_image.as_ref().unwrap()
    }

    pub fn render_once(&mut self) {
        let camera = camera_update(self.camera);
        let Some(renderer) = self.renderer_mut() else {
            return;
        };
        if let Ok(image) = renderer.render_static(&camera) {
            self.last_image = Some(image);
            self.frame_updated = true;
            self.style_loaded = true;
            self.map_idle = true;
        }
    }

    pub fn load_style(&mut self, style_url: &str) {
        if let Ok(url) = style_url.parse() {
            self.style_url = style_url.to_owned();
            if let Some(renderer) = self.renderer_mut() {
                renderer.load_style_from_url(&url);
            }
            self.style_loaded = false;
            self.map_idle = false;
            self.frame_updated = true;
        } else {
            eprintln!("Invalid style URL: {}", style_url);
        }
    }

    pub fn resize(&mut self, size: Size) {
        let new_size = safe_size(size);
        if self.size == new_size {
            return;
        }

        self.size = new_size;
        self.renderer = None;
        self.map_idle = false;
        self.frame_updated = true;
    }

    pub fn fly_to(&mut self, lat: f64, lon: f64, zoom: f64) {
        self.camera.lat = clamp_lat(lat);
        self.camera.lon = normalize_lon(lon);
        self.camera.zoom = clamp_zoom(zoom);
        self.drag_state = None;
        self.mark_dirty();
    }

    pub fn set_pitch(&mut self, pitch: f64) {
        self.camera.pitch = clamp_pitch(pitch);
        self.mark_dirty();
    }

    pub fn set_bearing(&mut self, bearing: f64) {
        self.camera.bearing = normalize_bearing(bearing);
        self.mark_dirty();
    }

    pub fn mouse_pressed(&mut self, x: f32, y: f32) {
        self.drag_state = Some(DragState { x, y });
    }

    pub fn mouse_released(&mut self) {
        self.drag_state = None;
    }

    pub fn mouse_moved(&mut self, x: f32, y: f32) {
        let Some(last) = self.drag_state.as_mut() else {
            return;
        };
        let dx = x - last.x;
        let dy = y - last.y;
        last.x = x;
        last.y = y;

        let (lon_per_px, lat_per_px) = degrees_per_pixel(self.camera.zoom, self.camera.lat);
        self.camera.lon = normalize_lon(self.camera.lon - f64::from(dx) * lon_per_px);
        self.camera.lat = clamp_lat(self.camera.lat + f64::from(dy) * lat_per_px);
        self.mark_dirty();
    }

    pub fn wheel_zoomed(&mut self, delta: f32) {
        if delta == 0.0 {
            return;
        }
        let direction = if delta > 0.0 { -1.0 } else { 1.0 };
        self.camera.zoom = clamp_zoom(self.camera.zoom + direction * WHEEL_STEP);
        self.mark_dirty();
    }

    pub fn double_clicked(&mut self, shift: bool) {
        let delta = if shift {
            -DOUBLE_CLICK_STEP
        } else {
            DOUBLE_CLICK_STEP
        };
        self.camera.zoom = clamp_zoom(self.camera.zoom + delta);
        self.mark_dirty();
    }

    fn renderer_mut(&mut self) -> Option<&mut ImageRenderer<maplibre_native::Static>> {
        if self.renderer.is_none() {
            self.renderer = Some(build_renderer(self.size, &self.style_url));
        }
        self.renderer.as_mut()
    }
}

fn cache_path() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("maplibre_database.sqlite")
}

fn safe_size(size: Size) -> (u32, u32) {
    ((size.width as u32).max(1), (size.height as u32).max(1))
}

fn resource_options() -> ResourceOptions {
    ResourceOptions::default()
        .with_tile_server_options(&TileServerOptions::default())
        .with_cache_path(cache_path())
}

fn camera_update(camera: MapCamera) -> CameraUpdate {
    CameraUpdate::new()
        .center(LatLng {
            lat: camera.lat,
            lng: camera.lon,
        })
        .zoom(camera.zoom)
        .bearing(camera.bearing)
        .pitch(camera.pitch)
}

fn build_renderer(size: (u32, u32), style_url: &str) -> ImageRenderer<maplibre_native::Static> {
    let mut renderer = ImageRendererBuilder::new()
        .with_size(
            NonZeroU32::new(size.0).unwrap(),
            NonZeroU32::new(size.1).unwrap(),
        )
        .with_pixel_ratio(1.0)
        .with_resource_options(resource_options())
        .build_static_renderer();
    renderer.load_style_from_url(&style_url.parse().unwrap());
    renderer
}

fn clamp_zoom(zoom: f64) -> f64 {
    zoom.clamp(MIN_ZOOM, MAX_ZOOM)
}

fn clamp_pitch(pitch: f64) -> f64 {
    pitch.clamp(MIN_PITCH, MAX_PITCH)
}

fn clamp_lat(lat: f64) -> f64 {
    lat.clamp(-MAX_ABS_LAT, MAX_ABS_LAT)
}

fn normalize_lon(lon: f64) -> f64 {
    let wrapped = (lon + 180.0).rem_euclid(360.0) - 180.0;
    if wrapped == -180.0 { 180.0 } else { wrapped }
}

fn normalize_bearing(bearing: f64) -> f64 {
    bearing.rem_euclid(360.0)
}

fn degrees_per_pixel(zoom: f64, lat: f64) -> (f64, f64) {
    let scale = 256.0 * 2.0_f64.powf(zoom);
    let lon_per_px = 360.0 / scale;
    let lat_per_px = lon_per_px * lat.to_radians().cos().abs().max(0.1);
    (lon_per_px, lat_per_px)
}

pub fn create_map(size: Size) -> Rc<RefCell<MapLibre>> {
    let size = safe_size(size);
    Rc::new(RefCell::new(MapLibre::new_lazy(size)))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn normalize_longitude_wraps_to_expected_range() {
        assert_eq!(normalize_lon(190.0), -170.0);
        assert_eq!(normalize_lon(-190.0), 170.0);
    }

    #[test]
    fn normalize_bearing_wraps_positively() {
        assert_eq!(normalize_bearing(450.0), 90.0);
        assert_eq!(normalize_bearing(-90.0), 270.0);
    }

    #[test]
    fn drag_changes_camera_state_in_expected_direction() {
        let mut map = MapLibre::new_lazy((256, 256));
        map.mouse_pressed(100.0, 100.0);
        map.mouse_moved(110.0, 90.0);
        let camera = map.camera();
        assert!(camera.lon < 0.0);
        assert!(camera.lat < 0.0);
    }

    #[test]
    fn wheel_zoom_is_clamped() {
        let mut map = MapLibre::new_lazy((256, 256));
        map.fly_to(0.0, 0.0, MAX_ZOOM);
        map.wheel_zoomed(-120.0);
        assert_eq!(map.camera().zoom, MAX_ZOOM);
        map.fly_to(0.0, 0.0, MIN_ZOOM);
        map.wheel_zoomed(120.0);
        assert_eq!(map.camera().zoom, MIN_ZOOM);
    }
}
