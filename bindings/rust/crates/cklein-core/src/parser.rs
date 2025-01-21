pub struct Program {}

#[derive(Debug, thiserror::Error)]
pub enum ParseError {}

pub fn parse(code: &str) -> Result<Program, ParseError> {
    todo!()
}
