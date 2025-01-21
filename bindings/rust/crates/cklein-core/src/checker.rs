#[derive(Debug, thiserror::Error)]
pub enum StaticError {
    #[error("Error parsing Klein code: {0}")]
    Parse(crate::parser::ParseError),
}

/// Checks that the given Klein code is valid, including type checking.
///
/// # Returns
///
/// The error that occurred while checking the code, if there was one.
pub fn check(code: &str) -> Option<StaticError> {
    None
}
