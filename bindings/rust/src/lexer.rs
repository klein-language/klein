use std::ffi::{CStr, CString};

use strum::IntoEnumIterator as _;
use thiserror::Error;

#[derive(strum_macros::EnumIter)]
pub enum TokenType {
    String,
    Identifier,
}

pub struct Token {
    pub value: String,
    pub token_type: TokenType,
}

#[derive(Error, Debug)]
pub enum TokenizationError {
    #[error("an unrecognized token was encountered")]
    UnrecognizedToken,
}

pub fn tokenize(code: &str) -> Result<Vec<Token>, TokenizationError> {
    unsafe {
        let ccode = CString::new(code).unwrap();
        let mut tokens = crate::internal::KleinTokenList {
            length: 0,
            data: std::ptr::null_mut(),
        };
        let result = crate::internal::tokenizeKlein(ccode.as_ptr(), &mut tokens);
        if !result.errorMessage.is_null() {
            return Err(TokenizationError::UnrecognizedToken);
        }

        let mut output = Vec::new();
        for index in 0 .. tokens.length {
            let internalToken = *tokens.data.add(index as usize);
            let value_str = CStr::from_ptr(internalToken.value);
            output.push(Token {
                token_type: TokenType::iter().nth(internalToken.type_ as usize).unwrap(),
                value: value_str.to_str().unwrap().to_string(),
            });
        }
        Ok(output)
    }
}
