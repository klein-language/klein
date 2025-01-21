use std::path::PathBuf;

fn main() {
    println!("cargo:rustc-link-search=./lib");
    println!("cargo:rustc-link-lib=klein");

    let bindings = bindgen::Builder::default()
        .header("./lib/klein.h")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .generate()
        .unwrap();

    bindings.write_to_file(PathBuf::from("./src/internal.rs")).unwrap();
}
