# Rust Bindings to Klein

Rust bindings to the Klein scripting language.

To just use the bindings, see [the cklein crate](https://github.com/klein-language/tree/main/bindings/rust/crates/cklein).

## Structure

The Rust bindings are split into three crates:

- [cklein](https://github.com/klein-language/tree/main/bindings/rust/crates/cklein) - The main high-level bindings
- [cklein-core](https://github.com/klein-language/tree/main/bindings/rust/crates/cklein-core) - Automatically generated low-level bindings re-exported by `cklein`
- [cklein](https://github.com/klein-language/tree/main/bindings/rust/crates/cklein-macros) - Procedural macros that can handle Klein code at compile-time.

Procedural macros need to be defined in their own crate, hence `cklein-macros`. `cklein` needs to re-export items from `cklein-macros` and `cklein-macros` needs to use types provided by the Klein bindings, so to avoid a circular dependency, the shared code is separated into its own crate, `cklein-core`. Only `cklein` itself is intended for public use.

## Contributing

Bindings are automatically generated via [`bindgen`](https://github.com/rust-lang/rust-bindgen) from [the `klein.h` header file](https://github.com/klein-language/tree/main/bindings/c/klein.h); See [`the build file`](https://github.com/klein-language/tree/main/bindings/rust/crates/cklein-core/build.rs) for more details.
