use std::path::PathBuf;

fn main() {
    println!("cargo:rustc-link-search=../c");
    println!("cargo:rustc-link-lib=klein");

    let bindings = bindgen::Builder::default()
        .header("../c/klein.h")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .generate()
        .unwrap();

    bindings.write_to_file(PathBuf::from("./src/internal.rs")).unwrap();
}
