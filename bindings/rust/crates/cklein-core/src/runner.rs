use std::ffi::{CStr, CString};

use thiserror::Error;

use crate::KleinError;

#[derive(Error, strum_macros::EnumIter, Debug)]
pub enum RuntimeError {
    #[error("Attempted to reference an undefined variable \"{0}\"")]
    UndefinedVariableReference(String),
}

pub fn run(code: &str) -> Result<(), crate::KleinError> {
    unsafe {
        let c_code = CString::new(code).unwrap();
        let result = crate::internal::runKlein(c_code.into_raw());
        match result.type_ {
            crate::internal::KleinResultType_KLEIN_ERROR_PEEK_EMPTY_TOKEN_STREAM => Err(KleinError::Runtime(
                RuntimeError::UndefinedVariableReference(CStr::from_ptr(result.data.referenceUndefinedVariable).to_str().unwrap().to_string()),
            )),
            crate::internal::KleinResultType_KLEIN_OK => Ok(()),
            _ => unreachable!(),
        }
    }
}
