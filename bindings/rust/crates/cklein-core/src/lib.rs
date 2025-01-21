#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

mod checker;
mod internal;
mod lexer;
mod parser;
mod runner;

pub enum KleinError {
    Static(crate::checker::StaticError),
    Runtime(crate::runner::RuntimeError),
}

pub use crate::{checker::check, lexer::tokenize, parser::parse, runner::run};
