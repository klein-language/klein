#include "../include/parser.h"
#include "../include/builtin.h"
#include "../include/context.h"
#include "../include/lexer.h"
#include "../include/list.h"
#include "../include/result.h"
#include "../include/util.h"

#include <stdlib.h>
#include <string.h>

static const char* TOKEN_TYPE_NAMES[] = {
    "keyword and",
    "keyword do",
    "keyword else",
    "keyword false",
    "keyword for",
    "keyword function",
    "keyword if",
    "keyword in",
    "keyword let",
    "keyword not",
    "keyword or",
    "keyword true",
    "keyword type",

    "left brace",
    "left bracket",
    "left parenthesis",
    "right brace",
    "right bracket",
    "right parenthesis",

    "asterisk",
    "forward slash",
    "less than",
    "greater than",
    "less than or equal to",
    "greater than or equal to",
    "double equals",
    "not equal",
    "caret",
    "comma",
    "colon",
    "dot",
    "equals",
    "minus",
    "plus",
    "semicolon",

    "string",
    "number",
    "identifier",

    "whitespace",
    "comment",
};

Result getObjectInternal(Object object, char* name) {
    for (int index = 0; index < object.internals.size; index++) {
        InternalField* internal = object.internals.data[index];
        if (strcmp(internal->name, name) == 0) {
            return ok(internal->value);
        }
    }

    return ERROR(ERROR_INTERNAL, "No internal field called \"", name, "\" was found on an object");
};

Result getObjectField(Object object, char* name) {
    for (int index = 0; index < object.fields.size; index++) {
        Field* field = object.fields.data[index];
        if (strcmp(field->name, name) == 0) {
            return ok(&field->value);
        }
    }

    return ERROR(ERROR_INTERNAL, "No field called \"", name, "\" was found on an object");
};

/**
 * Removes the next token in a `Token` `List`, and returns it's `value`.
 *
 * # Parameters
 * - `tokens` - The list of tokens to remove from. This must be of type `List` that stores pointers
 *   to `Token` values, otherwise the behavior of this function is undefined.
 * - `type` - The type of token to expect. If the next token is not of this type, an error is returned.
 *
 * # Returns
 *
 * The string value of the token as it appears in the original source code.
 *
 * # Errors
 *
 * If the given token stream is empty, an error is returned. If the next token is not of the given
 * token type, an error is returned. If the given list is not holding `Token` pointers, an error
 * is *not* necessarily returned; The behavior is undefined.
 *
 * # Memory
 *
 * The `Token` struct removed is freed from memory, and the caller is responsible for freeing
 * the returned string value.
 */
Result popToken(List* tokens, TokenType type) {

    // Empty token stream - error
    if (tokens->size == 0) {
        return ERROR(ERROR_UNEXPECTED_TOKEN, "Expected \"", TOKEN_TYPE_NAMES[type], "\", but found end of file.");
    }

    Token* token = tokens->data[0];
    if (token->type != type) {
        return ERROR(ERROR_UNEXPECTED_TOKEN, "Expected \"", TOKEN_TYPE_NAMES[type], "\", but found \"", TOKEN_TYPE_NAMES[token->type], "\"");
    }

    // Update list
    tokens->data++;
    tokens->size--;
    tokens->capacity--;

    // Return token
    return ok(FREE(token, Token).value);
}

Result popAnyToken(List* tokens) {
    // Empty token stream - error
    if (tokens->size == 0) {
        return ERROR(ERROR_UNEXPECTED_TOKEN, "Expected token but found end of file.");
    }

    Token* token = tokens->data[0];

    // Update list
    tokens->data++;
    tokens->size--;
    tokens->capacity--;

    // Return token
    return ok(FREE(token, Token).value);
}

TokenType peekTokenType(List* tokens) {
    if (tokens->size == 0) {
        return TOKEN_TYPE_EOF;
    }

    Token* token = (Token*) tokens->data[0];
    return token->type;
}

bool nextTokenIs(List* tokens, TokenType type) {
    if (tokens->size == 0) {
        return false;
    }

    return ((Token*) tokens->data[0])->type == type;
}

Result peekTokenValue(List* tokens) {
    if (tokens->size == 0) {
        return ERROR(ERROR_UNEXPECTED_TOKEN, "Expected token but found end of file");
    }

    return ok(&((Token*) tokens->data[0])->value);
}

Result parseLiteral(Context* context, List* tokens);
Result parseStatement(Context* context, List* tokens);
Result parseType(Context* context, List* tokens);

Result parseTypeLiteral(Context* context, List* tokens) {
    switch (peekTokenType(tokens)) {

        // Identifier
        case TOKEN_TYPE_IDENTIFIER: {
            char* identifier = unwrapUnsafe(popToken(tokens, TOKEN_TYPE_IDENTIFIER));
            TypeData data = (TypeData) {.identifier = identifier};
            return ok(HEAP(((TypeLiteral) {.data = data, .type = TYPE_LITERAL_IDENTIFIER}), TypeLiteral));
        }

        // Function
        case TOKEN_TYPE_KEYWORD_FUNCTION: {
            free(unwrapUnsafe(popToken(tokens, TOKEN_TYPE_KEYWORD_FUNCTION)));

            // Parameters
            List parameterTypes = EMPTY_LIST();
            free(TRY(popToken(tokens, TOKEN_TYPE_LEFT_PARENTHESIS)));
            while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS)) {
                Type type = FREE(TRY(parseType(context, tokens)), Type);
                Parameter* parameter = HEAP(((Parameter) {.type = type, .name = ""}), Parameter);
                free(TRY(appendToList(&parameterTypes, parameter)));
            }
            free(TRY(popToken(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS)));

            // Return type
            free(TRY(popToken(tokens, TOKEN_TYPE_COLON)));
            Type returnType = FREE(TRY(parseType(context, tokens)), Type);

            // Create function
            Function function = (Function) {
                .parameters = parameterTypes,
                .returnType = returnType,
                .thisObject = NULL,
            };

            // Create expression
            TypeData data = (TypeData) {.function = function};
            return ok(HEAP(((TypeLiteral) {.data = data, .type = TYPE_LITERAL_FUNCTION}), TypeLiteral));
        }

        // Not a type literal
        default: {
            return ERROR(ERROR_UNEXPECTED_TOKEN, "Expected type literal but found \"", TOKEN_TYPE_NAMES[peekTokenType(tokens)], "\"");
        }
    }
}

Result parseType(Context* context, List* tokens) {
    return parseTypeLiteral(context, tokens);
}

Result parseBlock(Context* context, List* tokens) {
    free(TRY(popToken(tokens, TOKEN_TYPE_LEFT_BRACE)));
    TRY(enterNewScope(context));
    List statements = EMPTY_LIST();
    while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_BRACE)) {
        Statement* statement = TRY(parseStatement(context, tokens));
        TRY(appendToList(&statements, statement));
    }
    free(TRY(popToken(tokens, TOKEN_TYPE_RIGHT_BRACE)));
    Block* block = HEAP(((Block) {.statements = statements, .innerScope = context->scope}), Block);
    TRY(exitScope(context));
    return ok(block);
}

Result parseLiteral(Context* context, List* tokens) {
    switch (peekTokenType(tokens)) {

        // String
        case TOKEN_TYPE_STRING: {
            char* string = TRY(popToken(tokens, TOKEN_TYPE_STRING));

            List internals = EMPTY_LIST();
            appendToList(&internals, HEAP(((InternalField) {.name = "string_value", .value = string}), InternalField));

            List fields = EMPTY_LIST();
            Expression length = (Expression) {
                .type = EXPRESSION_BUILTIN_FUNCTION,
                .data = (ExpressionData) {
                    .builtinFunction = (BuiltinFunction) {
                        .function = TRY(getBuiltin("String.length")),
                        .thisObject = NULL,
                    },
                },
            };
            appendToList(&fields, HEAP(((Field) {.name = "length", .value = length}), Field));

            ExpressionData data = (ExpressionData) {
                .object = (Object) {
                    .internals = internals,
                    .fields = fields,
                },
            };
            Expression* expression = HEAP(((Expression) {.data = data, .type = EXPRESSION_OBJECT}), Expression);
            return ok(expression);
        }

        // Identifier
        case TOKEN_TYPE_IDENTIFIER: {
            char* identifier = unwrapUnsafe(popToken(tokens, TOKEN_TYPE_IDENTIFIER));
            ExpressionData data = (ExpressionData) {.identifier = identifier};
            Expression* expression = HEAP(((Expression) {.data = data, .type = EXPRESSION_IDENTIFIER}), Expression);
            return ok(expression);
        }

        // Number
        case TOKEN_TYPE_NUMBER: {
            char* numberString = unwrapUnsafe(popToken(tokens, TOKEN_TYPE_NUMBER));
            float number = atof(numberString);
            free(numberString);
            ExpressionData data = (ExpressionData) {.number = number};
            Expression* expression = HEAP(((Expression) {.data = data, .type = EXPRESSION_NUMBER}), Expression);
            return ok(expression);
        }

        // Block
        case TOKEN_TYPE_KEYWORD_DO: {
            free(unwrapUnsafe(popToken(tokens, TOKEN_TYPE_KEYWORD_DO)));
            Block block = FREE(TRY(parseBlock(context, tokens)), Block);
            ExpressionData data = (ExpressionData) {.block = block};
            Expression* expression = HEAP(((Expression) {.data = data, .type = EXPRESSION_BLOCK}), Expression);
            return ok(expression);
        }

        // False
        case TOKEN_TYPE_KEYWORD_FALSE: {
            free(unwrapUnsafe(popToken(tokens, TOKEN_TYPE_KEYWORD_FALSE)));
            ExpressionData data = (ExpressionData) {.boolean = false};
            Expression* expression = HEAP(((Expression) {.data = data, .type = EXPRESSION_BOOLEAN}), Expression);
            return ok(expression);
        }

        // True
        case TOKEN_TYPE_KEYWORD_TRUE: {
            free(unwrapUnsafe(popToken(tokens, TOKEN_TYPE_KEYWORD_TRUE)));
            ExpressionData data = (ExpressionData) {.boolean = true};
            Expression* expression = HEAP(((Expression) {.data = data, .type = EXPRESSION_BOOLEAN}), Expression);
            return ok(expression);
        }

        // Function
        case TOKEN_TYPE_KEYWORD_FUNCTION: {
            free(unwrapUnsafe(popToken(tokens, TOKEN_TYPE_KEYWORD_FUNCTION)));

            // Parameters
            List parameters = EMPTY_LIST();
            free(TRY(popToken(tokens, TOKEN_TYPE_LEFT_PARENTHESIS)));
            while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS)) {
                char* name = TRY(popToken(tokens, TOKEN_TYPE_STRING));
                free(TRY(popToken(tokens, TOKEN_TYPE_COLON)));
                Type type = FREE(TRY(parseType(context, tokens)), Type);
                Parameter* parameter = HEAP(((Parameter) {.type = type, .name = name}), Parameter);
                free(TRY(appendToList(&parameters, parameter)));
            }
            free(TRY(popToken(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS)));

            // Return type
            free(TRY(popToken(tokens, TOKEN_TYPE_COLON)));
            Type returnType = FREE(TRY(parseType(context, tokens)), Type);

            // Body
            Block body = FREE(TRY(parseBlock(context, tokens)), Block);

            // Create function
            Function function = (Function) {
                .parameters = parameters,
                .returnType = returnType,
                .body = body,
            };

            // Create expression
            ExpressionData data = (ExpressionData) {.function = function};
            return ok(HEAP(((Expression) {.data = data, .type = EXPRESSION_FUNCTION}), Expression));
        }

        case TOKEN_TYPE_KEYWORD_TYPE: {
            free(unwrapUnsafe(popToken(tokens, TOKEN_TYPE_KEYWORD_TYPE)));

            // Fields
            List fields = EMPTY_LIST();
            free(TRY(popToken(tokens, TOKEN_TYPE_LEFT_BRACE)));
            while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_BRACE)) {
                char* fieldName = TRY(popToken(tokens, TOKEN_TYPE_IDENTIFIER));
                free(TRY(popToken(tokens, TOKEN_TYPE_COLON)));
                Type fieldType = FREE(TRY(parseType(context, tokens)), Type);
                Parameter* parameter = HEAP(((Parameter) {.name = fieldName, .type = fieldType}), Parameter);
                appendToList(&fields, parameter);

                if (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_BRACE)) {
                    free(TRY(popToken(tokens, TOKEN_TYPE_COMMA)));
                }
            }
            free(TRY(popToken(tokens, TOKEN_TYPE_RIGHT_BRACE)));

            ExpressionData data = (ExpressionData) {
                .typeDeclaration = (TypeDeclaration) {
                    .fields = fields,
                },
            };

            Expression* expression = HEAP(((Expression) {.data = data, .type = EXPRESSION_TYPE}), Expression);
            return ok(expression);
        }

        // Not a literal
        default: {
            return ERROR(ERROR_UNEXPECTED_TOKEN, "Expected literal but found \"", TOKEN_TYPE_NAMES[peekTokenType(tokens)], "\"");
        }
    }
}

static BinaryOperator MULTIPLICATIVE = (BinaryOperator) {
    .precedent = NULL,
    .tokenTypes = (TokenType[]) {TOKEN_TYPE_ASTERISK, TOKEN_TYPE_FORWARD_SLASH},
    .tokenTypeCount = 2,
};

static BinaryOperator ADDITIVE = (BinaryOperator) {
    .precedent = &MULTIPLICATIVE,
    .tokenTypes = (TokenType[]) {TOKEN_TYPE_PLUS, TOKEN_TYPE_MINUS},
    .tokenTypeCount = 2,
};

static BinaryOperator COMPARISON = (BinaryOperator) {
    .precedent = &ADDITIVE,
    .tokenTypes = (TokenType[]) {
        TOKEN_TYPE_LESS_THAN,
        TOKEN_TYPE_GREATER_THAN,
        TOKEN_TYPE_LESS_THAN_OR_EQUAL_TO,
        TOKEN_TYPE_GREATER_THAN_OR_EQUAL_TO,
        TOKEN_TYPE_DOUBLE_EQUALS,
        TOKEN_TYPE_NOT_EQUAL,
    },
    .tokenTypeCount = 6,
};

Result parseBinaryOperation(Context* context, List* tokens, BinaryOperator operator);
Result parsePrefixPostfixExpression(Context* context, List* tokens);

Result parsePrecedentBinaryOperation(Context* context, List* tokens, BinaryOperator operator) {
    if (operator.precedent == NULL) {
        return parsePrefixPostfixExpression(context, tokens);
    }

    return parseBinaryOperation(context, tokens, *operator.precedent);
}

bool nextTokenIsOneOf(List* tokens, TokenType* options, int optionCount) {
    if (isListEmpty(tokens)) {
        return false;
    }

    for (int index = 0; index < optionCount; index++) {
        if (nextTokenIs(tokens, options[index])) {
            return true;
        }
    }

    return false;
}

Result binaryOperationOf(char* tokenValue) {
    if (strcmp(tokenValue, ".") == 0) {
        return ok(HEAP(BINARY_OPERATION_DOT, BinaryOperation));
    }
    return ERROR(ERROR_INTERNAL, "Unknown binary operation: ", tokenValue);
}

Result parseFieldAccess(Context* context, List* tokens) {
    Expression* left = TRY(parseLiteral(context, tokens));
    while (nextTokenIs(tokens, TOKEN_TYPE_DOT)) {
        free(unwrapUnsafe(popToken(tokens, TOKEN_TYPE_DOT)));
        Expression* right = TRY(parseLiteral(context, tokens));
        ExpressionData data = (ExpressionData) {
            .binary = (BinaryExpression) {
                .left = left,
                .right = right,
                .operation = BINARY_OPERATION_DOT,
            },
        };
        left = HEAP(((Expression) {.type = EXPRESSION_BINARY, .data = data}), Expression);
    }

    return ok(left);
}

Result parseBinaryOperation(Context* context, List* tokens, BinaryOperator operator) {
    Expression* left = TRY(parsePrecedentBinaryOperation(context, tokens, operator));
    while (nextTokenIsOneOf(tokens, operator.tokenTypes, operator.tokenTypeCount)) {
        BinaryOperation operation = FREE(TRY(binaryOperationOf(TRY(popAnyToken(tokens)))), BinaryOperation);
        Expression* right = TRY(parsePrecedentBinaryOperation(context, tokens, operator));
        ExpressionData data = (ExpressionData) {
            .binary = (BinaryExpression) {
                .left = left,
                .right = right,
                .operation = operation,
            },
        };
        left = HEAP(((Expression) {.type = EXPRESSION_BINARY, .data = data}), Expression);
    }

    return ok(left);
}

Result parseExpression(Context* context, List* tokens) {
    return parseBinaryOperation(context, tokens, COMPARISON);
}

Result parsePrefixPostfixExpression(Context* context, List* tokens) {
    Expression* outer = NONNULL(malloc(sizeof(Expression)));
    Expression* innest = outer;

    // Prefix
    bool isFirst = true;
    while (nextTokenIs(tokens, TOKEN_TYPE_KEYWORD_NOT)) {
        Expression* inner = outer;
        if (isFirst) {
            innest = NONNULL(malloc(sizeof(Expression)));
            inner = innest;
            isFirst = false;
        }

        *outer = (Expression) {
            .type = EXPRESSION_UNARY,
            .data = (ExpressionData) {
                .unary = (UnaryExpression) {
                    .operation = (UnaryOperation) {
                        .type = UNARY_OPERATION_NOT,
                    },
                    .expression = inner,
                },
            },
        };
    }

    // Primary
    *innest = FREE(TRY(parseFieldAccess(context, tokens)), Expression);

    // Postfix
    isFirst = true;
    while (nextTokenIs(tokens, TOKEN_TYPE_LEFT_PARENTHESIS)) {
        Expression* inner = outer;
        if (isFirst) {
            outer = NONNULL(malloc(sizeof(Expression)));
            inner = innest;
            isFirst = false;
        }

        // Parse arguments
        List arguments = EMPTY_LIST();
        free(unwrapUnsafe(popToken(tokens, TOKEN_TYPE_LEFT_PARENTHESIS)));
        while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS)) {
            Expression* argument = TRY(parseExpression(context, tokens));
            TRY(appendToList(&arguments, argument));
            if (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS)) {
                free(TRY(popToken(tokens, TOKEN_TYPE_COMMA)));
            }
        }

        free(unwrapUnsafe(popToken(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS)));

        *outer = (Expression) {
            .type = EXPRESSION_UNARY,
            .data = (ExpressionData) {
                .unary = (UnaryExpression) {
                    .operation = (UnaryOperation) {
                        .type = UNARY_OPERATION_FUNCTION_CALL,
                        .data = (UnaryOperationData) {
                            .functionCall = arguments,
                        },
                    },
                    .expression = inner,
                },
            },
        };
    }

    return ok(outer);
}

Result parseDeclaration(Context* context, List* tokens) {
    free(TRY(popToken(tokens, TOKEN_TYPE_KEYWORD_LET)));

    // Name
    char* name = TRY(popToken(tokens, TOKEN_TYPE_IDENTIFIER));

    // Type
    Type* type = NULL;
    if (peekTokenType(tokens) == TOKEN_TYPE_COLON) {
        free(unwrapUnsafe(popToken(tokens, TOKEN_TYPE_COLON)));
        type = TRY(parseType(context, tokens));
    }

    // Value
    free(TRY(popToken(tokens, TOKEN_TYPE_EQUALS)));
    Expression value = FREE(TRY(parseExpression(context, tokens)), Expression);

    // Allocate & return
    Declaration* declaration = HEAP(((Declaration) {.name = name, .type = NULL, .value = value}), Declaration);
    return ok(declaration);
}

Result parseStatement(Context* context, List* tokens) {
    switch (peekTokenType(tokens)) {
        case TOKEN_TYPE_KEYWORD_LET: {
            Declaration declaration = FREE(TRY(parseDeclaration(context, tokens)), Declaration);
            Statement* statement = HEAP(((Statement) {.type = STATEMENT_DECLARATION, .data = (StatementData) {.declaration = declaration}}), Statement);
            free(TRY(popToken(tokens, TOKEN_TYPE_SEMICOLON)));
            return ok(statement);
        }
        default: {
            Expression expression = *(Expression*) TRY(parseExpression(context, tokens));
            Statement* statement = HEAP(((Statement) {.type = STATEMENT_EXPRESSION, .data = (StatementData) {.expression = expression}}), Statement);
            free(TRY(popToken(tokens, TOKEN_TYPE_SEMICOLON)));
            return ok(statement);
        }
    }
}

/**
 * Parses a program from a stream of tokens into an abstract syntax tree.
 *
 * # Parameters
 *
 * - `tokens` - The program's tokens as a `List` of `Tokens`, generally from the
 *   output of a call to `tokenize()` from `lexer.h`.
 *
 * # Returns
 *
 * The parsed abstract syntax tree.
 *
 * # Errors
 *
 * If the system failed to allocate memory for the abstract syntax tree, an error is
 * returned. If an unexpected token is encountered while parsing (i.e. the user entered
 * malformatted syntax), an error is returned.
 */
Result parse(void* context, List* tokens) {
    List statements = EMPTY_LIST();
    while (!isListEmpty(tokens)) {
        TRY(appendToList(&statements, TRY(parseStatement(context, tokens))));
    }
    Program* program = HEAP((Program) {.statements = statements}, Program);
    return ok(program);
}

void freeExpression(Expression* expression);

void freeStatement(Statement* statement) {
    switch (statement->type) {
        case STATEMENT_EXPRESSION: {
            freeExpression(&statement->data.expression);
            break;
        }
        case STATEMENT_DECLARATION: {
            break;
        }
    }

    free(statement);
}

void freeExpression(Expression* expression) {
    switch (expression->type) {
        case EXPRESSION_BLOCK: {
            for (int statementNumber = 0; statementNumber < expression->data.block.statements.size; statementNumber++) {
                freeStatement(expression->data.block.statements.data[statementNumber]);
            }
            break;
        }
        case EXPRESSION_STRING: {
            free(expression->data.string);
            break;
        }
        case EXPRESSION_IDENTIFIER: {
            free(expression->data.identifier);
            break;
        }
        case EXPRESSION_BOOLEAN: {
            // Nothing to free
            break;
        }
        case EXPRESSION_NUMBER: {
            // Nothing to free
            break;
        }
        case EXPRESSION_BINARY: {
            freeExpression(expression->data.binary.left);
            free(expression->data.binary.left);
            freeExpression(expression->data.binary.right);
            free(expression->data.binary.right);
            break;
        }
        case EXPRESSION_UNARY: {
            freeExpression(expression->data.unary.expression);
            break;
        }
        case EXPRESSION_BUILTIN_FUNCTION: {
            // Nothing to free
            break;
        }
        case EXPRESSION_FUNCTION: {
            for (int statementNumber = 0; statementNumber < expression->data.function.body.statements.size; statementNumber++) {
                freeStatement(expression->data.function.body.statements.data[statementNumber]);
            }
            break;
        }
    }
}
