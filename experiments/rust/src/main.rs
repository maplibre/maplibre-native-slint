mod maplibre;

slint::include_modules!();

/// Build the style dropdown list. When MAPLIBRE_STYLE_URL is set, prepend it so
/// it becomes the default-selected entry and the dropdown can still switch
/// between all styles.
fn setup_styles(ui: &MapWindow) {
    use slint::{ModelRc, SharedString, VecModel};
    let mut urls: Vec<SharedString> = vec![
        "https://demotiles.maplibre.org/style.json".into(),
        "https://tile.openstreetmap.jp/styles/osm-bright/style.json".into(),
    ];
    if let Ok(env) = std::env::var("MAPLIBRE_STYLE_URL") {
        let env = env.trim().to_string();
        if !env.is_empty() && !urls.iter().any(|u| u.as_str() == env) {
            urls.insert(0, env.into());
        }
    }
    ui.set_style_urls(ModelRc::new(VecModel::from(urls)));
}

fn main() {
    let ui = MapWindow::new().unwrap();
    let map = maplibre::create_map(ui.get_map_size());

    maplibre::init(&ui, &map);
    setup_styles(&ui);

    ui.run().unwrap();
}
