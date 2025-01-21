fn main() {
    let dir = std::env::var("CARGO_MANIFEST_DIR").unwrap();
    println!("cargo:rustc-link-search={}", std::path::Path::new(&dir).join("lib").display());
    println!("cargo:rustc-link-lib=klein");

    let bindings = bindgen::Builder::default()
        .header("./lib/klein.h")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .generate()
        .unwrap();

    bindings.write_to_file(std::path::PathBuf::from("./src/internal.rs")).unwrap();
}
