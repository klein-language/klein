#include "../include/lexer.h"
#include "../include/list.h"
#include "../include/result.h"
#include "../include/string.h"
#include <stdlib.h>
#include <string.h>

/**
 * Creates a token on the heap with the given type and value. The value
 * is also allocated on the heap, so `freeToken()` should be used to free
 * the token, which will also free the value, not just `free()`.
 *
 * # Parameters
 *
 * - `type` - The token type
 * - `value` - The value of the token as it appears in the source code.
 *
 * # Returns
 *
 * The token allocated on the heap wrapped in a `Result`, or an error `Result`
 * if heap allocation failed.
 */
Result heapToken(TokenType type, char* value) {
    Token* token = NONNULL(malloc(sizeof(Token)));
    token->type = type;
    token->value = NONNULL(malloc(sizeof(char) * strlen(value) + 1));
    strcpy(token->value, value);
    return ok(token);
}

void freeToken(Token* token) {
    free(token->value);
    free(token);
}

/**
 * Returns the next token in the given source code, leaving the source code unmodified.
 *
 * # Parameters
 *
 * - `sourceCode` - The source code to get the first token of.
 *
 * # Returns
 *
 * Returns the next `Token` wrapped in a `Result`.
 *
 * # Errors
 *
 * Returns an error if the given source code starts with an unknown token, or if memory
 * fails to allocate.
 */
Result getNextToken(char* sourceCode) {
    char* current = sourceCode;

    // Symbols
    switch (*current) {
        case '=':
            return ok(TRY(heapToken(TOKEN_TYPE_EQUALS, "=")));
        case '(':
            return ok(TRY(heapToken(TOKEN_TYPE_LEFT_PARENTHESIS, "(")));
        case ')':
            return ok(TRY(heapToken(TOKEN_TYPE_RIGHT_PARENTHESIS, ")")));
        case ';':
            return ok(TRY(heapToken(TOKEN_TYPE_SEMICOLON, ";")));
        case ' ':
            return ok(TRY(heapToken(TOKEN_TYPE_WHITESPACE, " ")));
        case '\n':
            return ok(TRY(heapToken(TOKEN_TYPE_WHITESPACE, "\n")));
        case '\r':
            return ok(TRY(heapToken(TOKEN_TYPE_WHITESPACE, "\r")));
        case '\t':
            return ok(TRY(heapToken(TOKEN_TYPE_WHITESPACE, "\t")));
    }

    // Identifier
    if ((*current >= 'A' && *current <= 'Z') ||
        (*current >= 'a' && *current <= 'z') || *current == '_') {
        List identifierCharacters = EMPTY_LIST();
        while ((*current >= 'A' && *current <= 'Z') ||
               (*current >= 'a' && *current <= 'z') ||
               (*current >= '0' && *current <= '9') ||
               *current == '_') {
            TRY(appendToList(&identifierCharacters, current));
            current++;
        }

        char* identifier = NONNULL(malloc(sizeof(char) * identifierCharacters.size + 1));
        for (int index = 0; index < identifierCharacters.size; index++) {
            identifier[index] = *(char*) identifierCharacters.data[index];
        };
        free(identifierCharacters.data);
        identifier[identifierCharacters.size] = '\0';

        // Keywords
        TokenType type = TOKEN_TYPE_IDENTIFIER;
        if (strcmp(identifier, "let") == 0) {
            type = TOKEN_TYPE_KEYWORD_LET;
        } else if (strcmp(identifier, "function") == 0) {
            type = TOKEN_TYPE_KEYWORD_FUNCTION;
        } else if (strcmp(identifier, "true") == 0) {
            type = TOKEN_TYPE_KEYWORD_TRUE;
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
        } else if (strcmp(identifier, "else") == 0) {
            type = TOKEN_TYPE_KEYWORD_ELSE;
        }

        Token* token = TRY(heapToken(type, identifier));
        free(identifier);
        return ok(token);
    }

    // String
    if (*current == '"') {
        current++;

        char quote = '"';
        List stringCharacters = EMPTY_LIST();
        TRY(appendToList(&stringCharacters, &quote));

        while (*current != '"') {
            TRY(appendToList(&stringCharacters, current));
            current++;
        }

        TRY(appendToList(&stringCharacters, &quote));

        char* string = NONNULL(malloc(sizeof(char) * (stringCharacters.size - 1)));

        for (int index = 0; index < stringCharacters.size; index++) {
            string[index] = *(char*) stringCharacters.data[index];
        };
        free(stringCharacters.data);
        string[stringCharacters.size] = '\0';

        Token* token = TRY(heapToken(TOKEN_TYPE_STRING, string));
        free(string);
        return ok(token);
    }

    // Unrecognized
    return ERROR(ERROR_UNRECOGNIZED_TOKEN, "Unrecognized token: ", sourceCode);
}

/**
 * Tokenizes the given source code into a list of tokens.
 *
 * # Parameters
 *
 * - `sourceCode` - The source code to tokenize. The given string will be unmodified.
 *
 * # Returns
 *
 * Returns a `Result` that contains a `List` of `Tokens` if the tokenization
 * was successful, or an error `Result` if an unknown token was encountered.
 */
Result tokenize(char* sourceCode) {
    List* tokens = TRY(emptyHeapList());

    int cursor = 0;
    int sourceLength = strlen(sourceCode);

    while (cursor != sourceLength) {
        Token* token = TRY(getNextToken(sourceCode + cursor));
        int length = strlen(token->value);
        if (token->type != TOKEN_TYPE_WHITESPACE) {
            if (token->type == TOKEN_TYPE_STRING) {
                token->value[length - 1] = '\0';
                token->value++;
            }
            TRY(appendToList(tokens, token));
        } else {
            freeToken(token);
        }
        cursor += length;
    }

    return ok(tokens);
}
