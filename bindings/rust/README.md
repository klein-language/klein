# `cklein`

Rust bindings to the Klein scripting language.

## Usage

Add `cklein` to your project:

```bash
cargo add cklein
```

It can be used to run, tokenize, and parse Klein code; For example:

```rust
fn main() -> Result<(), cklein::RuntimeError> {
	cklein::run!(r#"
		for number in 1.to(100) {
			if number.mod(3) == 0 and number.mod(5) == 0 {
				print("FizzBuzz");
			} else if number.mod(3) == 0 {
				print("Fizz");
			} else if number.mod(5) == 0 {
				print("Buzz");
			} else {
				print(number);
			}
		}
	"#)
}
```

## Contributing

Bindings are automatically generated via [`bindgen`](https://github.com/rust-lang/rust-bindgen) from [the `klein.h` header file](https://github.com/klein-language/tree/main/bindings/c/klein.h); See [`the build file`](https://github.com/klein-language/tree/main/bindings/rust/build.rs) for more details.
