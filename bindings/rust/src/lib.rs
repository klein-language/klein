#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

mod internal;
mod lexer;
mod parser;
mod runner;

pub use crate::{lexer::tokenize, runner::run};
