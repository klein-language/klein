use std::ffi::CString;

use thiserror::Error;

#[derive(Error, strum_macros::EnumIter, Debug)]
pub enum RuntimeError {
    #[error("Attempted to reference an undefined variable \"{0}\"")]
    UndefinedVariableReference(String),
}

pub fn run(code: &str) -> Result<(), RuntimeError> {
    unsafe {
        let c_code = CString::new(code).unwrap();
        let result = crate::internal::runKlein(c_code.as_ptr());
        if !result.errorMessage.is_null() {
            Err(RuntimeError::UndefinedVariableReference("null".to_string()))
        } else {
            Ok(())
        }
    }
}
