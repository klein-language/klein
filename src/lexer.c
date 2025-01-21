#include "../include/lexer.h"
#include "../bindings/c/klein.h"
#include "../include/list.h"
#include "../include/result.h"
#include "../include/util.h"
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
PRIVATE KleinResult createToken(TokenType type, String value, Token* output) {
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
PRIVATE KleinResult getNextToken(String sourceCode, Token* output) {
	ASSERT_NONNULL(sourceCode);

	char* current = sourceCode;

	// Symbols
	switch (*current) {
		case '=':
			if (strncmp("==", sourceCode, MIN(2, strlen(sourceCode))) == 0) {
				return createToken(TOKEN_TYPE_DOUBLE_EQUALS, "==", output);
			}
			return createToken(TOKEN_TYPE_EQUALS, "=", output);
		case '+':
			return createToken(TOKEN_TYPE_PLUS, "+", output);
		case '.':
			return createToken(TOKEN_TYPE_DOT, ".", output);
		case ',':
			return createToken(TOKEN_TYPE_COMMA, ",", output);
		case '<':
			if (strncmp("<=", sourceCode, MIN(2, strlen(sourceCode))) == 0) {
				return createToken(TOKEN_TYPE_LESS_THAN_OR_EQUAL_TO, "<=", output);
			}
			return createToken(TOKEN_TYPE_LESS_THAN, "<", output);
		case '(':
			return createToken(TOKEN_TYPE_LEFT_PARENTHESIS, "(", output);
		case '{':
			return createToken(TOKEN_TYPE_LEFT_BRACE, "{", output);
		case '[':
			return createToken(TOKEN_TYPE_LEFT_BRACKET, "[", output);
		case ']':
			return createToken(TOKEN_TYPE_RIGHT_BRACKET, "]", output);
		case '}':
			return createToken(TOKEN_TYPE_RIGHT_BRACE, "}", output);
		case ')':
			return createToken(TOKEN_TYPE_RIGHT_PARENTHESIS, ")", output);
		case ';':
			return createToken(TOKEN_TYPE_SEMICOLON, ";", output);
		case ':':
			return createToken(TOKEN_TYPE_COLON, ":", output);
		case ' ':
			return createToken(TOKEN_TYPE_WHITESPACE, " ", output);
		case '\n':
			return createToken(TOKEN_TYPE_WHITESPACE, "\n", output);
		case '\r':
			return createToken(TOKEN_TYPE_WHITESPACE, "\r", output);
		case '\t':
			return createToken(TOKEN_TYPE_WHITESPACE, "\t", output);
	}

	// Number
	if (*current >= '0' && *current <= '9') {
		CharList characters;
		TRY(emptyCharList(&characters), "creating the character list for a number token");
		while ((*current >= '0' && *current <= '9') && current < sourceCode + strlen(sourceCode)) {
			TRY(appendToCharList(&characters, *current), "appending to the number token's character list");
			current++;
		}
		TRY(appendToCharList(&characters, '\0'), "appending the null terminator on the number token's character list");

		return createToken(TOKEN_TYPE_NUMBER, characters.data, output);
	}

	// Identifier
	if ((*current >= 'A' && *current <= 'Z') ||
		(*current >= 'a' && *current <= 'z') || *current == '_') {
		CharList identifierCharacters;
		TRY(emptyCharList(&identifierCharacters), "creating the character list for an identifier token");
		while (((*current >= 'A' && *current <= 'Z') ||
				(*current >= 'a' && *current <= 'z') ||
				(*current >= '0' && *current <= '9') ||
				*current == '_') &&
			   current < sourceCode + strlen(sourceCode)) {
			TRY(appendToCharList(&identifierCharacters, *current), "appending a character to the identifier token's character list");
			current++;
		}
		TRY(appendToCharList(&identifierCharacters, '\0'), "appending the null terminator to the identifier token's character list");
		String identifier = identifierCharacters.data;

		// Keywords
		TokenType type = TOKEN_TYPE_IDENTIFIER;
		if (strcmp(identifier, "let") == 0) {
			type = TOKEN_TYPE_KEYWORD_LET;
		} else if (strcmp(identifier, "function") == 0) {
			type = TOKEN_TYPE_KEYWORD_FUNCTION;
		} else if (strcmp(identifier, "and") == 0) {
			type = TOKEN_TYPE_KEYWORD_AND;
		} else if (strcmp(identifier, "or") == 0) {
			type = TOKEN_TYPE_KEYWORD_OR;
		} else if (strcmp(identifier, "not") == 0) {
			type = TOKEN_TYPE_KEYWORD_NOT;
		} else if (strcmp(identifier, "do") == 0) {
			type = TOKEN_TYPE_KEYWORD_DO;
		} else if (strcmp(identifier, "type") == 0) {
			type = TOKEN_TYPE_KEYWORD_TYPE;
		} else if (strcmp(identifier, "for") == 0) {
			type = TOKEN_TYPE_KEYWORD_FOR;
		} else if (strcmp(identifier, "in") == 0) {
			type = TOKEN_TYPE_KEYWORD_IN;
		} else if (strcmp(identifier, "return") == 0) {
			type = TOKEN_TYPE_KEYWORD_RETURN;
		} else if (strcmp(identifier, "if") == 0) {
			type = TOKEN_TYPE_KEYWORD_IF;
		} else if (strcmp(identifier, "type") == 0) {
			type = TOKEN_TYPE_KEYWORD_TYPE;
		} else if (strcmp(identifier, "while") == 0) {
			type = TOKEN_TYPE_KEYWORD_WHILE;
		} else if (strcmp(identifier, "else") == 0) {
			type = TOKEN_TYPE_KEYWORD_ELSE;
		}

		return createToken(type, identifier, output);
	}

	// String
	if (*current == '"') {
		current++;

		char quote = '"';
		CharList stringCharacters;
		TRY(emptyCharList(&stringCharacters), "creating the character list for a string token");

		TRY(appendToCharList(&stringCharacters, quote), "appending the leading quote character to a string token's character list");
		while (*current != '"') {
			TRY(appendToCharList(&stringCharacters, *current), "appending a character to a string tokne's character list");
			current++;
		}
		TRY(appendToCharList(&stringCharacters, quote), "appending the trailing quote to a string token's character list");
		TRY(appendToCharList(&stringCharacters, '\0'), "appending the null terminator to a string token's character list");

		String string = stringCharacters.data;

		return createToken(TOKEN_TYPE_STRING, string, output);
	}

	// Unrecognized
	return error("Unrecognized token");
}
KleinResult tokenizeKlein(String sourceCode, TokenList* output) {
	ASSERT_NONNULL(sourceCode);
	ASSERT_NONNULL(output);

	TRY(emptyTokenList(output), "creating the program's token list");
	size_t cursor = 0;
	size_t sourceLength = strlen(sourceCode);

	while (cursor != sourceLength) {
		Token token;
		TRY(getNextToken(sourceCode + cursor, &token), "getting the next token from the source code");
		size_t length = strlen(token.value);
		if (token.type != TOKEN_TYPE_WHITESPACE) {
			if (token.type == TOKEN_TYPE_STRING) {
				token.value[length - 1] = '\0';
				token.value++;
			}
			TRY(appendToTokenList(output, token), "appending to the program's token list");
		}
		cursor += length;
	}

	return OK;
}

String tokenTypeName(TokenType type) {
	switch (type) {
		case TOKEN_TYPE_KEYWORD_AND:
			return "keyword and";
		case TOKEN_TYPE_KEYWORD_OR:
			return "keyword or";
		case TOKEN_TYPE_KEYWORD_NOT:
			return "keyword not";
		case TOKEN_TYPE_KEYWORD_FUNCTION:
			return "keyword function";
		case TOKEN_TYPE_KEYWORD_IF:
			return "keyword if";
		case TOKEN_TYPE_KEYWORD_ELSE:
			return "keyword else";
		case TOKEN_TYPE_KEYWORD_IN:
			return "keyword in";
		case TOKEN_TYPE_KEYWORD_FOR:
			return "keyword in";
		case TOKEN_TYPE_KEYWORD_DO:
			return "keyword do";
		case TOKEN_TYPE_KEYWORD_WHILE:
			return "keyword while";
		case TOKEN_TYPE_KEYWORD_TYPE:
			return "keyword type";
		case TOKEN_TYPE_KEYWORD_LET:
			return "keyword let";
		case TOKEN_TYPE_KEYWORD_RETURN:
			return "keyword return";
		case TOKEN_TYPE_LEFT_BRACE:
			return "left brace";
		case TOKEN_TYPE_LEFT_BRACKET:
			return "left bracket";
		case TOKEN_TYPE_LEFT_PARENTHESIS:
			return "left parenthesis";
		case TOKEN_TYPE_RIGHT_BRACE:
			return "right brace";
		case TOKEN_TYPE_RIGHT_BRACKET:
			return "right bracket";
		case TOKEN_TYPE_RIGHT_PARENTHESIS:
			return "right parenthesis";
		case TOKEN_TYPE_ASTERISK:
			return "asterisk";
		case TOKEN_TYPE_FORWARD_SLASH:
			return "forward slash";
		case TOKEN_TYPE_PLUS:
			return "plus";
		case TOKEN_TYPE_MINUS:
			return "minus";
		case TOKEN_TYPE_LESS_THAN:
			return "less than";
		case TOKEN_TYPE_GREATER_THAN:
			return "greater than";
		case TOKEN_TYPE_LESS_THAN_OR_EQUAL_TO:
			return "less than or equal to";
		case TOKEN_TYPE_GREATER_THAN_OR_EQUAL_TO:
			return "greater than or equal to";
		case TOKEN_TYPE_COMMA:
			return "comma";
		case TOKEN_TYPE_DOUBLE_EQUALS:
			return "double equals";
		case TOKEN_TYPE_NOT_EQUAL:
			return "not equals";
		case TOKEN_TYPE_CARET:
			return "caret";
		case TOKEN_TYPE_COLON:
			return "colon";
		case TOKEN_TYPE_DOT:
			return "dot";
		case TOKEN_TYPE_EQUALS:
			return "equals";
		case TOKEN_TYPE_SEMICOLON:
			return "semicolon";
		case TOKEN_TYPE_STRING:
			return "string";
		case TOKEN_TYPE_NUMBER:
			return "number";
		case TOKEN_TYPE_IDENTIFIER:
			return "variable name";
		case TOKEN_TYPE_COMMENT:
			return "comment";
		case TOKEN_TYPE_WHITESPACE:
			return "whitespace";
	}
}

IMPLEMENT_KLEIN_LIST(Token)
