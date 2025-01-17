#include "../include/lexer.h"
#include "../include/list.h"
#include "../include/result.h"
#include "../include/string.h"
#include <stdlib.h>
#include <string.h>

/**
 * Creates a token on the stack with the given type and value and stores
 * the result in `output`.
 *
 * # Parameters
 *
 * - `type` - The type of the token
 *
 * - `value` - The string value of the token as it appears in the source code
 *	 as a null-terminated `char*`. It must live at least as long as `output`.
 *	 If this value is `NULL`, an error is returned.
 *
 * - `output` - The location to store the `Token` created. It must point to
 *   some memory (be it stack or heap) that already has enough space to hold
 *   a `Token`; i.e. it could be the address of stack token or the result of
 *   `malloc(sizeof(Token))`, but it can't be `NULL`, or an error will be returned.
 *   The token stored in this `output` is only valid as long as `value` is valid.
 *
 * # Errors
 *
 * If the given string value is null, an error is returned.
 */
PRIVATE Result createToken(TokenType type, String value, Token* output) {
	ASSERT_NONNULL(value);
	ASSERT_NONNULL(output);
	Token token = (Token) {
		.type = type,
		.value = value,
	};

	RETURN_OK(output, token);
}

/**
 * Returns the next token that appears in the given source code, under the
 * assumption that the given source code doesn't begin midway through a token.
 *
 * # Parameters
 *
 * - `sourceCode` - The source code as a null-terminated `char*`. If this value
 *   is `NULL`, an error is returned. If it doesn't begin with a valid Klein token,
 *   an error is returned.
 *
 * - `output` - The location to store the `Token` created. It must point to
 *   some memory (be it stack or heap) that already has enough space to hold
 *   a `Token`; i.e. it could be the address of stack token or the result of
 *   `malloc(sizeof(Token))`, but it can't be `NULL`, or an error will be returned.
 *   This outputted token has copied strings from the original source code (because
 *   they need to be null-terminated and the original source code is contiguous),
 *   so it's not dependent on `sourceCode` being valid; i.e., if the string stored at
 *   `sourceCode` is freed, `output` will still be valid.
 *
 * # Errors
 *
 * If the given source code is `NULL`, an error is returned.
 * If the given source code doesn't start with a valid Klein token, an error is
 * returned.
 */
PRIVATE Result getNextToken(String sourceCode, Token* output) {
	ASSERT_NONNULL(sourceCode);

	char* current = sourceCode;

	// Symbols
	switch (*current) {
		case '=':
			TRY(createToken(TOKEN_TYPE_EQUALS, "=", output));
			return OK;
		case '+':
			return createToken(TOKEN_TYPE_PLUS, "+", output);
		case '.':
			return createToken(TOKEN_TYPE_DOT, ".", output);
		case '(':
			TRY(createToken(TOKEN_TYPE_LEFT_PARENTHESIS, "(", output));
			return OK;
		case '{':
			TRY(createToken(TOKEN_TYPE_LEFT_BRACE, "{", output));
			return OK;
		case '}':
			TRY(createToken(TOKEN_TYPE_RIGHT_BRACE, "}", output));
			return OK;
		case ')':
			TRY(createToken(TOKEN_TYPE_RIGHT_PARENTHESIS, ")", output));
			return OK;
		case ';':
			TRY(createToken(TOKEN_TYPE_SEMICOLON, ";", output));
			return OK;
		case ':':
			TRY(createToken(TOKEN_TYPE_COLON, ":", output));
			return OK;
		case ' ':
			TRY(createToken(TOKEN_TYPE_WHITESPACE, " ", output));
			return OK;
		case '\n':
			TRY(createToken(TOKEN_TYPE_WHITESPACE, "\n", output));
			return OK;
		case '\r':
			TRY(createToken(TOKEN_TYPE_WHITESPACE, "\r", output));
			return OK;
		case '\t':
			TRY(createToken(TOKEN_TYPE_WHITESPACE, "\t", output));
			return OK;
	}

	// Identifier
	if ((*current >= 'A' && *current <= 'Z') ||
		(*current >= 'a' && *current <= 'z') || *current == '_') {
		CharList identifierCharacters;
		TRY(emptyCharList(&identifierCharacters));
		while ((*current >= 'A' && *current <= 'Z') ||
			   (*current >= 'a' && *current <= 'z') ||
			   (*current >= '0' && *current <= '9') ||
			   *current == '_') {
			TRY(appendToCharList(&identifierCharacters, *current));
			current++;
		}
		TRY(appendToCharList(&identifierCharacters, '\0'));
		String identifier = identifierCharacters.data;

		// Keywords
		TokenType type = TOKEN_TYPE_IDENTIFIER;
		if (strcmp(identifier, "let") == 0) {
			type = TOKEN_TYPE_KEYWORD_LET;
		} else if (strcmp(identifier, "function") == 0) {
			type = TOKEN_TYPE_KEYWORD_FUNCTION;
		} else if (strcmp(identifier, "true") == 0) {
			type = TOKEN_TYPE_KEYWORD_TRUE;
		} else if (strcmp(identifier, "do") == 0) {
			type = TOKEN_TYPE_KEYWORD_DO;
		} else if (strcmp(identifier, "false") == 0) {
			type = TOKEN_TYPE_KEYWORD_FALSE;
		} else if (strcmp(identifier, "type") == 0) {
			type = TOKEN_TYPE_KEYWORD_TYPE;
		} else if (strcmp(identifier, "for") == 0) {
			type = TOKEN_TYPE_KEYWORD_FOR;
		} else if (strcmp(identifier, "in") == 0) {
			type = TOKEN_TYPE_KEYWORD_IN;
		} else if (strcmp(identifier, "if") == 0) {
			type = TOKEN_TYPE_KEYWORD_IF;
		} else if (strcmp(identifier, "type") == 0) {
			type = TOKEN_TYPE_KEYWORD_TYPE;
		} else if (strcmp(identifier, "else") == 0) {
			type = TOKEN_TYPE_KEYWORD_ELSE;
		}

		TRY_LET(Token, token, createToken, type, identifier);
		RETURN_OK(output, token);
	}

	// String
	if (*current == '"') {
		current++;

		char quote = '"';
		CharList stringCharacters;
		TRY(emptyCharList(&stringCharacters));

		TRY(appendToCharList(&stringCharacters, quote));
		while (*current != '"') {
			TRY(appendToCharList(&stringCharacters, *current));
			current++;
		}
		TRY(appendToCharList(&stringCharacters, quote));
		TRY(appendToCharList(&stringCharacters, '\0'));

		String string = stringCharacters.data;
		TRY_LET(Token, token, createToken, TOKEN_TYPE_STRING, string);
		RETURN_OK(output, token);
	}

	// Unrecognized
	return ERROR_UNRECOGNIZED_TOKEN;
}

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
Result tokenize(String sourceCode, TokenList* output) {
	ASSERT_NONNULL(sourceCode);
	ASSERT_NONNULL(output);

	TRY(emptyTokenList(output));
	size_t cursor = 0;
	size_t sourceLength = strlen(sourceCode);

	while (cursor != sourceLength) {
		TRY_LET(Token, token, getNextToken, sourceCode + cursor);
		size_t length = strlen(token.value);
		if (token.type != TOKEN_TYPE_WHITESPACE) {
			if (token.type == TOKEN_TYPE_STRING) {
				token.value[length - 1] = '\0';
				token.value++;
			}
			TRY(appendToTokenList(output, token));
		}
		cursor += length;
	}

	return OK;
}

IMPLEMENT_LIST(Token)
