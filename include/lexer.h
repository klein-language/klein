#ifndef LEXER_H
#define LEXER_H

#include "list.h"
#include "result.h"

typedef enum {

	// Keywords
	TOKEN_TYPE_KEYWORD_AND,
	TOKEN_TYPE_KEYWORD_DO,
	TOKEN_TYPE_KEYWORD_ELSE,
	TOKEN_TYPE_KEYWORD_FALSE,
	TOKEN_TYPE_KEYWORD_FOR,
	TOKEN_TYPE_KEYWORD_FUNCTION,
	TOKEN_TYPE_KEYWORD_IF,
	TOKEN_TYPE_KEYWORD_IN,
	TOKEN_TYPE_KEYWORD_LET,
	TOKEN_TYPE_KEYWORD_NOT,
	TOKEN_TYPE_KEYWORD_OR,
	TOKEN_TYPE_KEYWORD_TRUE,
	TOKEN_TYPE_KEYWORD_TYPE,
	TOKEN_TYPE_KEYWORD_WHILE,
	TOKEN_TYPE_KEYWORD_RETURN,

	// Grouping
	TOKEN_TYPE_LEFT_BRACE,
	TOKEN_TYPE_LEFT_BRACKET,
	TOKEN_TYPE_LEFT_PARENTHESIS,
	TOKEN_TYPE_RIGHT_BRACE,
	TOKEN_TYPE_RIGHT_BRACKET,
	TOKEN_TYPE_RIGHT_PARENTHESIS,

	// Operators
	TOKEN_TYPE_ASTERISK,
	TOKEN_TYPE_FORWARD_SLASH,
	TOKEN_TYPE_LESS_THAN,
	TOKEN_TYPE_GREATER_THAN,
	TOKEN_TYPE_LESS_THAN_OR_EQUAL_TO,
	TOKEN_TYPE_GREATER_THAN_OR_EQUAL_TO,
	TOKEN_TYPE_DOUBLE_EQUALS,
	TOKEN_TYPE_NOT_EQUAL,
	TOKEN_TYPE_CARET,
	TOKEN_TYPE_COLON,
	TOKEN_TYPE_COMMA,
	TOKEN_TYPE_DOT,
	TOKEN_TYPE_EQUALS,
	TOKEN_TYPE_MINUS,
	TOKEN_TYPE_PLUS,
	TOKEN_TYPE_SEMICOLON,

	// Literals
	TOKEN_TYPE_STRING,
	TOKEN_TYPE_NUMBER,
	TOKEN_TYPE_IDENTIFIER,

	// Ignored
	TOKEN_TYPE_WHITESPACE,
	TOKEN_TYPE_COMMENT,
	TOKEN_TYPE_EOF

} TokenType;

typedef struct {
	TokenType type;
	char* value;
} Token;

DEFINE_LIST(Token)

/**
 * Tokenizes the given string of Klein source code into a list of tokens.
 * This is the first step of interpreting Klein code.
 *
 * # Parameters
 *
 * - `sourceCode` - The original, Klein source code. It may contain
 *   syntax and semantic errors and this function will still return `OK`
 *   as long as each individual token in the code is a valid Klein token.
 *   It mustn't be `NULL`.
 *
 * - `output` - Where to store the resulting `TokenList`. It must point to
 *   some memory (be it stack or heap) that already has enough space to hold
 *   a `TokenList`; i.e. it could be the address of stack token or the result of
 *   `malloc(sizeof(TokenList))`, but it can't be `NULL`, or an error will be returned.
 *   The outputted tokens have copied strings from the original source code (because
 *   they need to be null-terminated and the original source code is contiguous),
 *   so it's not dependent on `sourceCode` being valid; i.e., if the string stored at
 *   `sourceCode` is freed, `output` will still be valid.
 *
 * # Errors
 *
 * If memory for the token list fails to allocate, an error is returned.
 * If the source code contains unrecognized tokens, an error is returned.
 * If the given `sourceCode` or `output` is `NULL`, an error is returned.
 */
Result tokenize(char* sourceCode, TokenList* output);

String tokenTypeName(TokenType type);

#endif
