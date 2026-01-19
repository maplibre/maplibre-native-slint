use std::cell::RefCell;
use std::num::NonZeroU32;
use std::sync::Arc;

use maplibre_native::{Image, ImageRenderer, ImageRendererBuilder, Static};
use slint::ComponentHandle;

use crate::MainWindow;
use crate::MapAdapter;
use crate::Size;

/// Wrapper around a static image renderer for headless map rendering
pub struct MapLibre {
    renderer: ImageRenderer<Static>,
    last_image: Option<Image>,
}

impl MapLibre {
    /// Create a new MapLibre instance with the given size
    pub fn new(width: u32, height: u32) -> Self {
        let mut renderer = ImageRendererBuilder::new()
            .with_size(
                NonZeroU32::new(width.max(1)).unwrap(),
                NonZeroU32::new(height.max(1)).unwrap(),
            )
            .build_static_renderer();

        // Load a default map style
        if let Ok(url) = "https://demotiles.maplibre.org/style.json".parse() {
            renderer.load_style_from_url(&url);
        }

        Self {
            renderer,
            last_image: None,
        }
    }

    /// Get a mutable reference to the renderer
    #[allow(dead_code)]
    pub fn renderer(&mut self) -> &mut ImageRenderer<Static> {
        &mut self.renderer
    }

    /// Get the last rendered image
    pub fn image(&self) -> Option<&Image> {
        self.last_image.as_ref()
    }

    /// Render a frame and store the result
    pub fn render(&mut self) -> Result<(), String> {
        // Render a static image at the default position
        // (0.0, 0.0) = center, 0.0 = zoom level, bearing and pitch are 0.0
        let image = self
            .renderer
            .render_static(0.0, 0.0, 0.0, 0.0, 0.0)
            .map_err(|e| format!("Render error: {:?}", e))?;

        self.last_image = Some(image);
        Ok(())
    }
}

/// Create a new MapLibre instance
pub fn create_map(size: Size) -> Arc<RefCell<MapLibre>> {
    let width = (size.width as u32).max(1);
    let height = (size.height as u32).max(1);
    Arc::new(RefCell::new(MapLibre::new(width, height)))
}

/// Initialize UI callbacks and map interactions
pub fn init(ui: &MainWindow, map: &Arc<RefCell<MapLibre>>) {
    // Set initial map size callback
    ui.on_get_map_size({
        move || {
            Size {
                width: 800.0,
                height: 600.0,
            }
        }
    });

    ui.on_map_size_changed({
        let map = Arc::downgrade(map);
        move |size| {
            if let Some(map) = map.upgrade() {
                let width = (size.width as u32).max(1);
                let height = (size.height as u32).max(1);
                // Recreate the renderer with the new size
                let mut map = map.borrow_mut();
                *map = MapLibre::new(width, height);
            }
        }
    });

    ui.global::<MapAdapter>().on_tick_map_loop({
        let map = Arc::downgrade(map);
        let ui_handle = ui.as_weak();
        move || {
            if let Some(map_arc) = map.upgrade() {
                let mut map = map_arc.borrow_mut();
                if let Ok(_) = map.render() {
                    if let Some(image) = map.image() {
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
        }
    });

    ui.global::<MapAdapter>().on_mouse_press({
        let map = Arc::downgrade(map);
        move |_x: f32, _y: f32| {
            if let Some(_map_arc) = map.upgrade() {
                // TODO: Handle mouse press on map
            }
        }
    });

    ui.global::<MapAdapter>().on_mouse_release({
        let map = Arc::downgrade(map);
        move |_x: f32, _y: f32| {
            if let Some(_map_arc) = map.upgrade() {
                // TODO: Handle mouse release on map
            }
        }
    });

    ui.global::<MapAdapter>().on_mouse_move({
        let map = Arc::downgrade(map);
        move |_x: f32, _y: f32, _shift: bool| {
            if let Some(_map_arc) = map.upgrade() {
                // TODO: Handle mouse move on map
            }
        }
    });

    ui.global::<MapAdapter>().on_wheel_zoom({
        let map = Arc::downgrade(map);
        move |_x: f32, _y: f32, _delta: f32| {
            if let Some(_map_arc) = map.upgrade() {
                // TODO: Handle wheel zoom on map
            }
        }
    });
}
