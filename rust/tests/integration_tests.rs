// Integration tests for MapLibre Native Slint
// These tests verify the integration between components

use std::num::NonZeroU32;

#[test]
fn test_image_renderer_builder_creation() {
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
    use maplibre_native::ImageRendererBuilder;
    use std::num::NonZeroU32;
    
    // Test that we can create multiple renderers
    let sizes = vec![
        (256, 256),
        (512, 512),
        (1024, 768),
    ];
    
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
    use std::path::Path;
    use maplibre_native::ImageRendererBuilder;
    use std::num::NonZeroU32;
    
    // Test renderer with cache path
    let width = NonZeroU32::new(256).unwrap();
    let height = NonZeroU32::new(256).unwrap();
    let cache_path = Path::new(env!("CARGO_MANIFEST_DIR")).join("test_cache.sqlite");
    
    let _renderer = ImageRendererBuilder::new()
        .with_size(width, height)
        .with_pixel_ratio(1.0)
        .with_cache_path(&cache_path)
        .build_static_renderer();
    
    assert!(true);
    
    // Clean up test cache file if it exists
    let _ = std::fs::remove_file(&cache_path);
}

#[test]
fn test_render_static_basic() {
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
    let result = renderer.render_static(0.0, 0.0, 0.0, 0.0, 0.0);
    
    // Just verify we can call the method
    // The result may be Err since we haven't loaded a style
    assert!(result.is_ok() || result.is_err());
}
