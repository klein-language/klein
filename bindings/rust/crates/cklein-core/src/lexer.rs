use std::ffi::{CStr, CString};

use strum::IntoEnumIterator as _;
use thiserror::Error;

#[derive(Debug, strum_macros::EnumIter)]
pub enum TokenType {
    String,
    Identifier,
}

pub struct Token {
    pub value: String,
    pub token_type: TokenType,
}

#[derive(Error, Debug)]
pub enum TokenizeError {
    #[error("an unrecognized token was encountered")]
    UnrecognizedToken(String),
}

pub fn tokenize(code: &str) -> Result<Vec<Token>, TokenizeError> {
    unsafe {
        let ccode = CString::new(code).unwrap();
        let mut tokens = crate::internal::TokenList {
            size: 0,
            capacity: 0,
            data: std::ptr::null_mut(),
        };

        let result = crate::internal::tokenizeKlein(ccode.into_raw(), &mut tokens);
        match result.type_ {
            crate::internal::KleinResultType_KLEIN_ERROR_UNRECOGNIZED_TOKEN => Err(TokenizeError::UnrecognizedToken(
                CStr::from_ptr(result.data.unrecognizedToken).to_str().unwrap().to_string(),
            )),
            crate::internal::KleinResultType_KLEIN_OK => {
                let mut output = Vec::new();
                for index in 0 .. tokens.size {
                    let internalToken = *tokens.data.add(index as usize);
                    let value_str = CStr::from_ptr(internalToken.value);
                    output.push(Token {
                        token_type: TokenType::iter().nth(internalToken.type_ as usize).unwrap(),
                        value: value_str.to_str().unwrap().to_string(),
                    });
                }
                Ok(output)
            },
            _ => unreachable!(),
        }
    }
}
