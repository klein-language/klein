use std::ffi::CString;

use strum::IntoEnumIterator as _;

use crate::lexer::TokenType;

pub struct Program {}

#[derive(thiserror::Error, Debug)]
pub enum ParseError {
    #[error("Unexpected token: Expected {expected:?} but found {actual:?}")]
    UnexpectedToken { expected: TokenType, actual: TokenType },
}

pub fn parse(code: &str) -> Result<Program, ParseError> {
    unsafe {
        let c_code = CString::new(code).unwrap();
        let mut program = crate::internal::Program {
            statements: crate::internal::StatementList {
                capacity: 0,
                size: 0,
                data: std::ptr::null_mut(),
            },
        };
        let result = crate::internal::parseKlein(c_code.into_raw(), &mut program);
        match result.type_ {
            crate::internal::KleinResultType_KLEIN_ERROR_UNEXPECTED_TOKEN => {
                let expected = TokenType::iter().nth(result.data.unexpectedToken.expected as usize).unwrap();
                let actual = TokenType::iter().nth(result.data.unexpectedToken.actual as usize).unwrap();
                Err(ParseError::UnexpectedToken { expected, actual })
            },
            crate::internal::KleinResultType_KLEIN_OK => Ok(Program {}),
            _ => unreachable!(),
        }
    }
}
