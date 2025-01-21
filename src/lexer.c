#include "../include/lexer.h"
#include "../include//klein.h"
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
PRIVATE Token createToken(TokenType type, String value) {
	return (Token) {
		.type = type,
		.value = value,
	};
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
	char* current = sourceCode;

	// Symbols
	switch (*current) {
		case '=':
			if (strncmp("==", sourceCode, MIN(2, strlen(sourceCode))) == 0) {
				RETURN_OK(output, createToken(TOKEN_TYPE_DOUBLE_EQUALS, "=="));
			}
			RETURN_OK(output, createToken(TOKEN_TYPE_EQUALS, "="));
		case '+':
			RETURN_OK(output, createToken(TOKEN_TYPE_PLUS, "+"));
		case '.':
			RETURN_OK(output, createToken(TOKEN_TYPE_DOT, "."));
		case ',':
			RETURN_OK(output, createToken(TOKEN_TYPE_COMMA, ","));
		case '<':
			if (strncmp("<=", sourceCode, MIN(2, strlen(sourceCode))) == 0) {
				RETURN_OK(output, createToken(TOKEN_TYPE_LESS_THAN_OR_EQUAL_TO, "<="));
			}
			RETURN_OK(output, createToken(TOKEN_TYPE_LESS_THAN, "<"));
		case '(':
			RETURN_OK(output, createToken(TOKEN_TYPE_LEFT_PARENTHESIS, "("));
		case '{':
			RETURN_OK(output, createToken(TOKEN_TYPE_LEFT_BRACE, "{"));
		case '[':
			RETURN_OK(output, createToken(TOKEN_TYPE_LEFT_BRACKET, "["));
		case ']':
			RETURN_OK(output, createToken(TOKEN_TYPE_RIGHT_BRACKET, "]"));
		case '}':
			RETURN_OK(output, createToken(TOKEN_TYPE_RIGHT_BRACE, "}"));
		case ')':
			RETURN_OK(output, createToken(TOKEN_TYPE_RIGHT_PARENTHESIS, ")"));
		case ';':
			RETURN_OK(output, createToken(TOKEN_TYPE_SEMICOLON, ";"));
		case ':':
			RETURN_OK(output, createToken(TOKEN_TYPE_COLON, ":"));
		case ' ':
			RETURN_OK(output, createToken(TOKEN_TYPE_WHITESPACE, " "));
		case '\n':
			RETURN_OK(output, createToken(TOKEN_TYPE_WHITESPACE, "\n"));
		case '\r':
			RETURN_OK(output, createToken(TOKEN_TYPE_WHITESPACE, "\r"));
		case '\t':
			RETURN_OK(output, createToken(TOKEN_TYPE_WHITESPACE, "\t"));
	}

	// Number
	if (*current >= '0' && *current <= '9') {
		CharList characters = emptyCharList();
		while ((*current >= '0' && *current <= '9') && current < sourceCode + strlen(sourceCode)) {
			appendToCharList(&characters, *current);
			current++;
		}
		appendToCharList(&characters, '\0');

		RETURN_OK(output, createToken(TOKEN_TYPE_NUMBER, characters.data));
	}

	// Identifier
	if ((*current >= 'A' && *current <= 'Z') ||
		(*current >= 'a' && *current <= 'z') || *current == '_') {
		CharList identifierCharacters = emptyCharList();
		while (((*current >= 'A' && *current <= 'Z') ||
				(*current >= 'a' && *current <= 'z') ||
				(*current >= '0' && *current <= '9') ||
				*current == '_') &&
			   current < sourceCode + strlen(sourceCode)) {
			appendToCharList(&identifierCharacters, *current);
			current++;
		}
		appendToCharList(&identifierCharacters, '\0');
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

		RETURN_OK(output, createToken(type, identifier));
	}

	// String
	if (*current == '"') {
		current++;

		char quote = '"';
		CharList stringCharacters = emptyCharList();

		appendToCharList(&stringCharacters, quote);
		while (*current != '"') {
			appendToCharList(&stringCharacters, *current);
			current++;
		}
		appendToCharList(&stringCharacters, quote);
		appendToCharList(&stringCharacters, '\0');

		String string = stringCharacters.data;

		RETURN_OK(output, createToken(TOKEN_TYPE_STRING, string));
	}

	// Unrecognized
	return (KleinResult) {
		.type = KLEIN_ERROR_UNRECOGNIZED_TOKEN,
		.data = (KleinResultData) {
			.unrecognizedToken = strdup(current),
		},
	};
}

KleinResult tokenizeKlein(String sourceCode, TokenList* output) {
	*output = emptyTokenList();
	size_t cursor = 0;
	size_t sourceLength = strlen(sourceCode);

	while (cursor != sourceLength) {
		Token token;
		TRY(getNextToken(sourceCode + cursor, &token));
		size_t length = strlen(token.value);
		if (token.type != TOKEN_TYPE_WHITESPACE) {
			if (token.type == TOKEN_TYPE_STRING) {
				token.value[length - 1] = '\0';
				token.value++;
			}
			appendToTokenList(output, token);
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
		case TOKEN_TYPE_EOF:
			return "end of file";
	}
}

IMPLEMENT_KLEIN_LIST(Token)
