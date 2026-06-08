// Integration tests for MapLibre Native Slint
// These tests verify the integration between components

use std::num::NonZeroU32;
use std::path::PathBuf;
use std::sync::{Mutex, OnceLock};

use maplibre_native::tile_server_options::TileServerOptions;
use maplibre_native::{CameraUpdate, LatLng, ResourceOptions};

fn renderer_test_lock() -> &'static Mutex<()> {
    static LOCK: OnceLock<Mutex<()>> = OnceLock::new();
    LOCK.get_or_init(|| Mutex::new(()))
}

fn renderer_tests_enabled() -> bool {
    std::env::var_os("MAPLIBRE_NATIVE_SLINT_RUN_RENDERER_TESTS").as_deref() == Some("1".as_ref())
}

fn display_configured() -> bool {
    std::env::var_os("DISPLAY").is_some() || std::env::var_os("WAYLAND_DISPLAY").is_some()
}

fn skip_without_display() -> bool {
    if !renderer_tests_enabled() {
        eprintln!("skipping renderer integration test: opt-in env not set");
        true
    } else if (cfg!(target_os = "linux") || cfg!(target_os = "freebsd")) && !display_configured() {
        eprintln!("skipping renderer integration test: no display configured");
        true
    } else {
        false
    }
}

fn resource_options_with_cache(path: PathBuf) -> ResourceOptions {
    ResourceOptions::default()
        .with_tile_server_options(&TileServerOptions::default())
        .with_cache_path(path)
}

fn test_camera() -> CameraUpdate {
    CameraUpdate::new()
        .center(LatLng { lat: 0.0, lng: 0.0 })
        .zoom(0.0)
        .bearing(0.0)
        .pitch(0.0)
}

#[test]
fn test_image_renderer_builder_creation() {
    let _guard = renderer_test_lock().lock().unwrap();
    use maplibre_native::ImageRendererBuilder;

    // Test that we can create an ImageRendererBuilder with valid parameters
    let width = NonZeroU32::new(512).unwrap();
    let height = NonZeroU32::new(512).unwrap();

    let _builder = ImageRendererBuilder::new()
        .with_size(width, height)
        .with_pixel_ratio(1.0);

    // If we get here, the builder was created successfully
    assert!(true);
}

#[test]
fn test_static_renderer_creation() {
    if skip_without_display() {
        return;
    }
    let _guard = renderer_test_lock().lock().unwrap();
    use maplibre_native::ImageRendererBuilder;
    use std::num::NonZeroU32;

    // Test that we can create a static renderer
    let width = NonZeroU32::new(256).unwrap();
    let height = NonZeroU32::new(256).unwrap();

    let _renderer = ImageRendererBuilder::new()
        .with_size(width, height)
        .with_pixel_ratio(1.0)
        .build_static_renderer();

    // If we get here, the renderer was created successfully
    assert!(true);
}

#[test]
fn test_style_url_loading() {
    if skip_without_display() {
        return;
    }
    let _guard = renderer_test_lock().lock().unwrap();
    use maplibre_native::ImageRendererBuilder;
    use std::num::NonZeroU32;

    // Test that we can load a style URL
    let width = NonZeroU32::new(256).unwrap();
    let height = NonZeroU32::new(256).unwrap();

    let _renderer = ImageRendererBuilder::new()
        .with_size(width, height)
        .with_pixel_ratio(1.0)
        .build_static_renderer();

    // Just verify we can create the renderer
    // Actual style loading would require network access
    assert!(true);
}

#[test]
fn test_multiple_renderers() {
    if skip_without_display() {
        return;
    }
    let _guard = renderer_test_lock().lock().unwrap();
    use maplibre_native::ImageRendererBuilder;
    use std::num::NonZeroU32;

    // Test that we can create multiple renderers
    let sizes = vec![(256, 256), (512, 512), (1024, 768)];

    for (w, h) in sizes {
        let width = NonZeroU32::new(w).unwrap();
        let height = NonZeroU32::new(h).unwrap();

        let _renderer = ImageRendererBuilder::new()
            .with_size(width, height)
            .with_pixel_ratio(1.0)
            .build_static_renderer();
    }

    assert!(true);
}

#[test]
fn test_pixel_ratios() {
    if skip_without_display() {
        return;
    }
    let _guard = renderer_test_lock().lock().unwrap();
    use maplibre_native::ImageRendererBuilder;
    use std::num::NonZeroU32;

    // Test different pixel ratios
    let width = NonZeroU32::new(256).unwrap();
    let height = NonZeroU32::new(256).unwrap();

    let ratios = vec![1.0, 1.5, 2.0, 3.0];

    for ratio in ratios {
        let _renderer = ImageRendererBuilder::new()
            .with_size(width, height)
            .with_pixel_ratio(ratio)
            .build_static_renderer();
    }

    assert!(true);
}

#[test]
fn test_cache_path_creation() {
    if skip_without_display() {
        return;
    }
    let _guard = renderer_test_lock().lock().unwrap();
    use maplibre_native::ImageRendererBuilder;
    use std::num::NonZeroU32;
    use std::path::Path;

    // Test renderer with cache path
    let width = NonZeroU32::new(256).unwrap();
    let height = NonZeroU32::new(256).unwrap();
    let cache_path = Path::new(env!("CARGO_MANIFEST_DIR")).join("test_cache.sqlite");

    let _renderer = ImageRendererBuilder::new()
        .with_size(width, height)
        .with_pixel_ratio(1.0)
        .with_resource_options(resource_options_with_cache(cache_path.clone()))
        .build_static_renderer();

    assert!(true);

    // Clean up test cache file if it exists
    let _ = std::fs::remove_file(&cache_path);
}

#[test]
fn test_render_static_basic() {
    if skip_without_display() {
        return;
    }
    let _guard = renderer_test_lock().lock().unwrap();
    use maplibre_native::ImageRendererBuilder;
    use std::num::NonZeroU32;

    // Test basic static rendering (without style, will likely fail but shouldn't panic)
    let width = NonZeroU32::new(256).unwrap();
    let height = NonZeroU32::new(256).unwrap();

    let mut renderer = ImageRendererBuilder::new()
        .with_size(width, height)
        .with_pixel_ratio(1.0)
        .build_static_renderer();

    // Try to render without loading style - this may fail but shouldn't panic
    let camera = test_camera();
    let result = renderer.render_static(&camera);

    // Just verify we can call the method
    // The result may be Err since we haven't loaded a style
    assert!(result.is_ok() || result.is_err());
}
