mod maplibre;

slint::include_modules!();

fn main() {
    let ui = MapWindow::new().unwrap();

    let size = ui.get_window_size();
    let map = maplibre::create_map(size);

    maplibre::init(&ui, &map);

    ui.run().unwrap();
}
