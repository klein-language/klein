# Klein

A small, minimal, embeddable scripting language.

Klein is meant to fill the same niche as Lua&mdash;dead simple and highly embeddable scripting&mdash;with some improvements, notably:

- A simple TypeScript-like type system
- Improved ergonomics for iterators, tables and lists
- More built-in functionality for tables and lists
- Less ways of doing the same thing

At a glance, Klein is a statically, structurally typed, interpreted, multi-paradigm scripting language.

## Installation

### Building From Source

Klein can be built from source on any machine that can compile standard C at at least C99:

```bash
git clone https://github.com/klein-language/klein.git && cd klein && make install
```

#### Installation Customization

All customizable flags can be found at the top of [the makefile](https://github.com/vi013t/klein/tree/main/Makefile).

The default compiler is `clang`, but any C compiler can be used by passing it to the `CC` variable, i.e.:

```bash
make install CC=gcc
```

Klein is written using only standard C without compiler extensions, so it can be compiled with any C compiler.

The default location for the executable is `/usr/bin`, which can be set with `LOCATION`:

```bash
make install LOCATION=/home/me/.klein
```

Note that when changing this option, the location passed will be automatically created if it doesn't exist, but it *won't* be automatically added to your `$PATH`.

## Embedding & Bindings

### C

To use klein in a C project, copy the `klein.a` static library and the `klein.h` definitions in the [`bindings/c`](https://github.com/klein-language/klein/tree/main/bindings/c) directory into your project, and include `klein.a` when linking your object files. An example C project embedding klein can be found in [`tests/c`](https://github.com/klein-language/tree/main/tests/c).

### Rust

Klein is available through the Rust crate [`cklein`](https://crates.io/crates/cklein).
