use maplibre_native::ImageRenderer;
use maplibre_native::ImageRendererBuilder;
use std::cell::RefCell;
use std::num::NonZeroU32;
use std::path::Path;
use std::sync::Arc;
use crate::Size;

pub struct MapLibre {
    renderer: ImageRenderer<maplibre_native::Static>,
    last_image: Option<maplibre_native::Image>,
}

impl MapLibre {
    pub fn new(renderer: ImageRenderer<maplibre_native::Static>) -> Self {
        Self {
            renderer,
            last_image: None,
        }
    }

    pub fn updated(&self) -> bool {
        self.last_image.is_some()
    }

    pub fn renderer(&mut self) -> &mut ImageRenderer<maplibre_native::Static> {
        &mut self.renderer
    }

    pub fn read_still_image(&mut self) -> &maplibre_native::Image {
        self.last_image.as_ref().unwrap()
    }

    pub fn render_once(&mut self) {
        // For static rendering, we render each frame
        if let Ok(image) = self.renderer.render_static(0.0, 0.0, 0.0, 0.0, 0.0) {
            self.last_image = Some(image);
        }
    }

    /// Load a map style from URL
    pub fn load_style(&mut self, style_url: &str) {
        if let Ok(url) = style_url.parse() {
            self.renderer.load_style_from_url(&url);
        } else {
            eprintln!("Invalid style URL: {}", style_url);
        }
    }
}

pub fn create_map(size: Size) -> Arc<RefCell<MapLibre>> {
    // Convert length values to u32 pixels
    let width = (size.width as u32).max(1);
    let height = (size.height as u32).max(1);
    
    let mut renderer = ImageRendererBuilder::new()
        .with_size(NonZeroU32::new(width).unwrap(), NonZeroU32::new(height).unwrap())
        .with_pixel_ratio(1.0)
        .with_cache_path(Path::new(env!("CARGO_MANIFEST_DIR")).join("maplibre_database.sqlite"))
        .build_static_renderer();
    
    // Load default style
    renderer.load_style_from_url(&"https://demotiles.maplibre.org/style.json".parse().unwrap());
    
    let map = Arc::new(RefCell::new(MapLibre::new(renderer)));
    
    map
}
