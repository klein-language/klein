#include "../include/parser.h"
#include "../include/context.h"
#include "../include/lexer.h"
#include "../include/list.h"
#include "../include/result.h"
#include "../include/sugar.h"
#include "../include/util.h"

#include <stdlib.h>
#include <string.h>

typedef union {
	Expression owned;
	Expression* borrowed;
} MaybeBorrowedExpressionData;

PRIVATE Result parseExpression(TokenList* tokens, Expression* output);

Result getObjectInternal(Object object, String name, void** output) {
	FOR_EACH(InternalField internal, object.internals) {
		if (strcmp(internal.name, name) == 0) {
			RETURN_OK(output, internal.value);
		}
	}
	END;

	return ERROR_INTERNAL;
}

Result getObjectField(Object object, String name, Expression** output) {
	FOR_EACH_REF(Field * field, object.fields) {
		if (strcmp(field->name, name) == 0) {
			RETURN_OK(output, &field->value);
		}
	}
	END;

	return ERROR_INTERNAL;
}

PRIVATE Result popToken(TokenList* tokens, TokenType type, String* output) {

	// Empty token stream - error
	if (tokens->size == 0) {
		return ERROR_UNEXPECTED_TOKEN;
	}

	// Check token
	Token token = tokens->data[0];
	if (token.type != type) {
		return ERROR_UNEXPECTED_TOKEN;
	}

	// Update list
	tokens->data++;
	tokens->size--;
	tokens->capacity--;

	// Return token
	RETURN_OK(output, token.value);
}

PRIVATE Result popAnyToken(TokenList* tokens, String* output) {
	// Empty token stream - error
	if (tokens->size == 0) {
		return ERROR_UNEXPECTED_TOKEN;
	}

	// Get token
	Token token = tokens->data[0];

	// Update list
	tokens->data++;
	tokens->size--;
	tokens->capacity--;

	// Return token
	RETURN_OK(output, token.value);
}

PRIVATE Result peekTokenType(TokenList* tokens, TokenType* output) {
	if (tokens->size == 0) {
		return ERROR_UNEXPECTED_TOKEN;
	}

	RETURN_OK(output, tokens->data[0].type);
}

PRIVATE bool nextTokenIs(TokenList* tokens, TokenType type) {
	if (tokens->size == 0) {
		return false;
	}

	return tokens->data[0].type == type;
}

PRIVATE Result parseLiteral(TokenList* tokens, Expression* output);
PRIVATE Result parseStatement(TokenList* tokens, Statement* output);
PRIVATE Result parseType(TokenList* tokens, Type* output);

PRIVATE Result parseTypeLiteral(TokenList* tokens, TypeLiteral* output) {
	TRY_LET(TokenType, nextTokenType, peekTokenType, tokens);
	switch (nextTokenType) {

		// Identifier
		case TOKEN_TYPE_IDENTIFIER: {
			String identifier;
			UNWRAP(popToken(tokens, TOKEN_TYPE_IDENTIFIER, &identifier));
			*output = (TypeLiteral) {
				.data = (TypeLiteralData) {
					.identifier = identifier,
				},
				.type = TYPE_LITERAL_IDENTIFIER,
			};
			return OK;
		}

		// Function
		case TOKEN_TYPE_KEYWORD_FUNCTION: {
			String next;
			UNWRAP(popToken(tokens, TOKEN_TYPE_KEYWORD_FUNCTION, &next));

			// Parameters
			ParameterList parameterTypes;
			TRY(emptyParameterList(&parameterTypes));
			TRY(popToken(tokens, TOKEN_TYPE_LEFT_PARENTHESIS, &next));
			while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS)) {
				Type type;
				TRY(parseType(tokens, &type));
				TRY(appendToParameterList(&parameterTypes, (Parameter) {.name = "", .type = type}));
			}
			TRY(popToken(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS, &next));

			// Return type
			TRY(popToken(tokens, TOKEN_TYPE_COLON, &next));
			Type returnType;
			TRY(parseType(tokens, &returnType));

			// Create function
			Function* function = malloc(sizeof(Function));
			ASSERT_NONNULL(function);
			*function = (Function) {
				.parameters = parameterTypes,
				.returnType = returnType,
				.thisObject = NULL,
			};

			// Create expression
			*output = (TypeLiteral) {
				.data = (TypeLiteralData) {
					.function = function,
				},
				.type = TYPE_LITERAL_FUNCTION,
			};
			return OK;
		}

		// Not a type literal
		default: {
			return ERROR_UNEXPECTED_TOKEN;
		}
	}
}

PRIVATE Result parseType(TokenList* tokens, Type* output) {
	DEBUG_START("Parsing", "type");
	TypeLiteral literal;
	TRY(parseTypeLiteral(tokens, &literal));
	DEBUG_END("parsing type");
	RETURN_OK(output, ((Type) {.type = TYPE_LITERAL, .data = (TypeData) {.literal = literal}}));
}

PRIVATE Result parseBlock(TokenList* tokens, Block* output) {
	DEBUG_START("Parsing", "block expression");

	TRY(enterNewScope());
	TRY_LET(String, next, popToken, tokens, TOKEN_TYPE_LEFT_BRACE);

	// Parse statements
	StatementList* statements;
	TRY(emptyHeapStatementList(&statements));
	while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_BRACE)) {
		Statement statement;
		TRY(parseStatement(tokens, &statement));
		TRY(appendToStatementList(statements, statement));
	}

	UNWRAP(popToken(tokens, TOKEN_TYPE_RIGHT_BRACE, &next));

	Block block = (Block) {
		.statements = statements,
		.innerScope = CONTEXT->scope,
	};

	TRY(exitScope());

	DEBUG_END("Parsing block expression");
	RETURN_OK(output, block);
}

Result parseObjectLiteral(TokenList* tokens, Expression* output) {
	TRY_LET(String, next, popToken, tokens, TOKEN_TYPE_LEFT_BRACE);

	// Fields
	FieldList fields;
	TRY(emptyFieldList(&fields));
	while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_BRACE)) {
		TRY_LET(String, name, popToken, tokens, TOKEN_TYPE_IDENTIFIER);
		TRY(popToken(tokens, TOKEN_TYPE_EQUALS, &next));
		TRY_LET(Expression, value, parseExpression, tokens);
		TRY(appendToFieldList(&fields, (Field) {.name = name, .value = value}));
		if (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_BRACE)) {
			TRY(popToken(tokens, TOKEN_TYPE_COMMA, &next));
		}
	}

	TRY(popToken(tokens, TOKEN_TYPE_RIGHT_BRACE, &next));

	InternalFieldList internals;
	TRY(emptyInternalFieldList(&internals));

	Object* object = malloc(sizeof(Object));
	ASSERT_NONNULL(object);
	*object = (Object) {
		.fields = fields,
		.internals = internals,
	};
	Expression expression = (Expression) {
		.type = EXPRESSION_OBJECT,
		.data = (ExpressionData) {
			.object = object,
		},
	};
	RETURN_OK(output, expression);
}

PRIVATE Result parseLiteral(TokenList* tokens, Expression* output) {
	DEBUG_START("Parsing", "literal");
	TRY_LET(TokenType, nextTokenType, peekTokenType, tokens);
	switch (nextTokenType) {

		// String
		case TOKEN_TYPE_STRING: {
			DEBUG_START("Parsing", "string literal");
			UNWRAP_LET(String, value, popToken, tokens, TOKEN_TYPE_STRING);
			TRY_LET(Expression, expression, stringExpression, value);
			DEBUG_END("parsing string literal");
			DEBUG_END("parsing literal");
			RETURN_OK(output, expression);
		}

		// Identifier
		case TOKEN_TYPE_IDENTIFIER: {
			UNWRAP_LET(String, identifier, popToken, tokens, TOKEN_TYPE_IDENTIFIER);
			Expression expression = (Expression) {
				.type = EXPRESSION_IDENTIFIER,
				.data = (ExpressionData) {
					.identifier = identifier,
				},
			};
			DEBUG_END("parsing literal");
			RETURN_OK(output, expression);
		}

		// List literal
		case TOKEN_TYPE_LEFT_BRACKET: {
			DEBUG_START("parsing", "list literal");
			UNWRAP_LET(String, next, popToken, tokens, TOKEN_TYPE_LEFT_BRACKET);

			ExpressionList elements;
			TRY(emptyExpressionList(&elements));
			while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_BRACKET)) {
				TRY_LET(Expression, element, parseExpression, tokens);
				TRY(appendToExpressionList(&elements, element));
				if (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_BRACKET)) {
					TRY(popToken(tokens, TOKEN_TYPE_COMMA, &next));
				}
			}
			UNWRAP(popToken(tokens, TOKEN_TYPE_RIGHT_BRACKET, &next));
			TRY_LET(Expression, list, listExpression, elements);

			DEBUG_END("parsing list literal");
			DEBUG_END("parsing literal");
			RETURN_OK(output, list);
		}

		case TOKEN_TYPE_KEYWORD_FOR: {
			DEBUG_START("Parsing", "for loop expression");

			UNWRAP_LET(String, next, popToken, tokens, TOKEN_TYPE_KEYWORD_FOR);
			TRY_LET(String, binding, popToken, tokens, TOKEN_TYPE_IDENTIFIER);
			TRY(popToken(tokens, TOKEN_TYPE_KEYWORD_IN, &next));
			TRY_LET(Expression, list, parseExpression, tokens);
			TRY_LET(Block, body, parseBlock, tokens);

			ForLoop* forLoop = malloc(sizeof(ForLoop));
			*forLoop = (ForLoop) {
				.binding = binding,
				.list = list,
				.body = body,
			};

			Expression loop = (Expression) {
				.type = EXPRESSION_FOR_LOOP,
				.data = (ExpressionData) {
					.forLoop = forLoop,
				},
			};
			DEBUG_END("parsing for loop expression");
			DEBUG_END("parsing literal");
			RETURN_OK(output, loop);
		}

		case TOKEN_TYPE_KEYWORD_WHILE: {
			DEBUG_START("Parsing", "while loop expression");

			UNWRAP_LET(String, next, popToken, tokens, TOKEN_TYPE_KEYWORD_WHILE);
			TRY_LET(Expression, condition, parseExpression, tokens);
			TRY_LET(Block, body, parseBlock, tokens);

			WhileLoop* whileLoop = malloc(sizeof(WhileLoop));
			*whileLoop = (WhileLoop) {
				.condition = condition,
				.body = body,
			};

			Expression loop = (Expression) {
				.type = EXPRESSION_WHILE_LOOP,
				.data = (ExpressionData) {
					.whileLoop = whileLoop,
				},
			};
			DEBUG_END("parsing while loop expression");
			DEBUG_END("parsing literal");
			RETURN_OK(output, loop);
		}

		case TOKEN_TYPE_KEYWORD_IF: {
			DEBUG_START("Parsing", "if expression");

			UNWRAP_LET(String, next, popToken, tokens, TOKEN_TYPE_KEYWORD_IF);
			TRY_LET(Expression, condition, parseExpression, tokens);
			TRY_LET(Block, body, parseBlock, tokens);

			IfExpression* ifExpression = malloc(sizeof(IfExpression));
			*ifExpression = (IfExpression) {
				.condition = condition,
				.body = body,
			};

			Expression loop = (Expression) {
				.type = EXPRESSION_IF,
				.data = (ExpressionData) {
					.ifExpression = ifExpression,
				},
			};
			DEBUG_END("parsing if expression");
			DEBUG_END("parsing literal");
			RETURN_OK(output, loop);
		}

		// Object literal
		case TOKEN_TYPE_LEFT_BRACE: {
			TRY_LET(Expression, object, parseObjectLiteral, tokens);
			DEBUG_END("parsing literal");
			RETURN_OK(output, object);
		}

		// Number
		case TOKEN_TYPE_NUMBER: {
			UNWRAP_LET(String, numberString, popToken, tokens, TOKEN_TYPE_NUMBER);
			TRY_LET(Expression, expression, numberExpression, atof(numberString));
			DEBUG_END("parsing literal");
			RETURN_OK(output, expression);
		}

		// Block
		case TOKEN_TYPE_KEYWORD_DO: {
			String next;
			TRY(popToken(tokens, TOKEN_TYPE_KEYWORD_DO, &next));

			Block block;
			TRY(parseBlock(tokens, &block));
			Block* heapBlock = malloc(sizeof(Block));
			*heapBlock = block;

			Expression expression = (Expression) {
				.type = EXPRESSION_BLOCK,
				.data = (ExpressionData) {
					.block = heapBlock,
				},
			};

			DEBUG_END("parsing literal");
			RETURN_OK(output, expression);
		}

		// False
		case TOKEN_TYPE_KEYWORD_FALSE: {
			String next;
			TRY(popToken(tokens, TOKEN_TYPE_KEYWORD_FALSE, &next));

			Expression expression = (Expression) {
				.type = EXPRESSION_BOOLEAN,
				.data = (ExpressionData) {
					.boolean = false,
				},
			};
			DEBUG_END("parsing literal");
			RETURN_OK(output, expression);
		}

		// True
		case TOKEN_TYPE_KEYWORD_TRUE: {
			String next;
			TRY(popToken(tokens, TOKEN_TYPE_KEYWORD_TRUE, &next));

			Expression expression = (Expression) {
				.type = EXPRESSION_BOOLEAN,
				.data = (ExpressionData) {
					.boolean = true,
				},
			};
			DEBUG_END("parsing literal");
			RETURN_OK(output, expression);
		}

		// Function
		case TOKEN_TYPE_KEYWORD_FUNCTION: {
			DEBUG_START("Parsing", "function");
			String next;
			TRY(popToken(tokens, TOKEN_TYPE_KEYWORD_FUNCTION, &next));

			// Parameters
			ParameterList parameters;
			TRY(emptyParameterList(&parameters));

			TRY(popToken(tokens, TOKEN_TYPE_LEFT_PARENTHESIS, &next));

			DEBUG_START("Parsing", "function parameters");
			while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS)) {
				String name;
				TRY(popToken(tokens, TOKEN_TYPE_IDENTIFIER, &name));

				TRY(popToken(tokens, TOKEN_TYPE_COLON, &next));

				Type type;
				TRY(parseType(tokens, &type));

				Parameter parameter = (Parameter) {
					.type = type,
					.name = name,
				};

				TRY(appendToParameterList(&parameters, parameter));

				if (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS)) {
					TRY(popToken(tokens, TOKEN_TYPE_COMMA, &next));
				}
			}
			UNWRAP(popToken(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS, &next));
			DEBUG_END("parsing function parameters");

			// Return type
			TRY(popToken(tokens, TOKEN_TYPE_COLON, &next));
			Type returnType;
			TRY(parseType(tokens, &returnType));

			// Body
			Block body;
			TRY(parseBlock(tokens, &body));

			// Create function
			Function function = (Function) {
				.parameters = parameters,
				.returnType = returnType,
				.body = body,
			};

			// Create expression
			Expression expression = (Expression) {
				.type = EXPRESSION_FUNCTION,
				.data = (ExpressionData) {
					.function = function,
				},
			};

			DEBUG_END("parsing function");
			DEBUG_END("parsing literal");
			RETURN_OK(output, expression);
		}

		case TOKEN_TYPE_KEYWORD_TYPE: {
			String next;
			TRY(popToken(tokens, TOKEN_TYPE_KEYWORD_TYPE, &next));

			// Fields
			ParameterList fields;
			emptyParameterList(&fields);

			TRY(popToken(tokens, TOKEN_TYPE_LEFT_BRACE, &next));
			while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_BRACE)) {

				// Field name
				String fieldName;
				TRY(popToken(tokens, TOKEN_TYPE_IDENTIFIER, &fieldName));

				// Colon
				TRY(popToken(tokens, TOKEN_TYPE_COLON, &next));

				// Field type
				Type fieldType;
				TRY(parseType(tokens, &fieldType));

				// Parameter
				Parameter parameter = (Parameter) {
					.name = fieldName,
					.type = fieldType,
				};
				TRY(appendToParameterList(&fields, parameter));

				// Comma
				if (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_BRACE)) {
					TRY(popToken(tokens, TOKEN_TYPE_COMMA, &next));
				}
			}
			TRY(popToken(tokens, TOKEN_TYPE_RIGHT_BRACE, &next));

			Expression expression = (Expression) {
				.type = EXPRESSION_TYPE,
				.data = (ExpressionData) {
					.typeDeclaration = (TypeDeclaration) {
						.fields = fields,
					},
				},
			};

			DEBUG_END("parsing literal");
			RETURN_OK(output, expression);
		}

		// Not a literal
		default: {
			return ERROR_UNEXPECTED_TOKEN;
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

static BinaryOperator ASSIGNMENT = (BinaryOperator) {
	.precedent = &COMPARISON,
	.tokenTypes = (TokenType[]) {
		TOKEN_TYPE_EQUALS,
	},
	.tokenTypeCount = 1,
};

PRIVATE Result parseBinaryOperation(TokenList* tokens, BinaryOperator operator, Expression * output);
PRIVATE Result parsePrefixExpression(TokenList* tokens, Expression* output);

PRIVATE Result parsePrecedentBinaryOperation(TokenList* tokens, BinaryOperator operator, Expression * output) {
	if (operator.precedent == NULL) {
		return parsePrefixExpression(tokens, output);
	}

	return parseBinaryOperation(tokens, *operator.precedent, output);
}

PRIVATE bool nextTokenIsOneOf(TokenList* tokens, TokenType* options, size_t optionCount) {
	if (isTokenListEmpty(*tokens)) {
		return false;
	}

	for (size_t index = 0; index < optionCount; index++) {
		if (nextTokenIs(tokens, options[index])) {
			return true;
		}
	}

	return false;
}

PRIVATE Result binaryOperationOf(String tokenValue, BinaryOperation* output) {
	if (strcmp(tokenValue, ".") == 0) {
		RETURN_OK(output, BINARY_OPERATION_DOT);
	}
	if (strcmp(tokenValue, "+") == 0) {
		RETURN_OK(output, BINARY_OPERATION_PLUS);
	}
	if (strcmp(tokenValue, "*") == 0) {
		RETURN_OK(output, BINARY_OPERATION_TIMES);
	}
	if (strcmp(tokenValue, "/") == 0) {
		RETURN_OK(output, BINARY_OPERATION_DIVIDE);
	}
	if (strcmp(tokenValue, "<=") == 0) {
		RETURN_OK(output, BINARY_OPERATION_LESS_THAN_OR_EQUAL_TO);
	}
	if (strcmp(tokenValue, "==") == 0) {
		RETURN_OK(output, BINARY_OPERATION_EQUAL);
	}
	if (strcmp(tokenValue, "=") == 0) {
		RETURN_OK(output, BINARY_OPERATION_ASSIGN);
	}
	return ERROR_INTERNAL;
}

PRIVATE Result parseFieldAccess(TokenList* tokens, Expression* output) {
	TRY_LET(Expression, left, parseLiteral, tokens);

	String next;

	while (nextTokenIs(tokens, TOKEN_TYPE_DOT)) {
		DEBUG_START("Parsing", "field access");
		UNWRAP(popToken(tokens, TOKEN_TYPE_DOT, &next));
		Expression* right = malloc(sizeof(Expression));
		TRY(parseLiteral(tokens, right));

		Expression* newLeft = malloc(sizeof(Expression));
		*newLeft = left;
		left = (Expression) {
			.type = EXPRESSION_BINARY,
			.data = (ExpressionData) {
				.binary = (BinaryExpression) {
					.left = newLeft,
					.right = right,
					.operation = BINARY_OPERATION_DOT,
				},
			},
		};
		DEBUG_END("Parsing field access");
	}

	RETURN_OK(output, left);
}

PRIVATE Result parseBinaryOperation(TokenList* tokens, BinaryOperator operator, Expression * output) {

	Expression left;
	TRY(parsePrecedentBinaryOperation(tokens, operator, & left));

	String next;
	while (nextTokenIsOneOf(tokens, operator.tokenTypes, operator.tokenTypeCount)) {
		UNWRAP(popAnyToken(tokens, &next));

		DEBUG_START("Parsing", "binary expression \"%s\"", next);
		BinaryOperation operation;
		TRY(binaryOperationOf(next, &operation));

		Expression* right = malloc(sizeof(Expression));
		TRY(parsePrecedentBinaryOperation(tokens, operator, right));

		Expression* newLeft = malloc(sizeof(Expression));
		*newLeft = left;

		left = (Expression) {
			.type = EXPRESSION_BINARY,
			.data = (ExpressionData) {
				.binary = (BinaryExpression) {
					.left = newLeft,
					.right = right,
					.operation = operation,
				},
			},
		};

		DEBUG_END("Parsing binary expression");
	}

	RETURN_OK(output, left);
}

PRIVATE Result parseExpression(TokenList* tokens, Expression* output) {
	return parseBinaryOperation(tokens, ASSIGNMENT, output);
}

PRIVATE Result parsePostfixExpression(TokenList* tokens, Expression* output) {
	TRY_LET(Expression, expression, parseFieldAccess, tokens);

	while (nextTokenIs(tokens, TOKEN_TYPE_LEFT_PARENTHESIS)) {
		DEBUG_START("Parsing", "postfix expression");

		TRY_LET(String, next, popToken, tokens, TOKEN_TYPE_LEFT_PARENTHESIS);

		// Parse arguments
		ExpressionList arguments;
		TRY(emptyExpressionList(&arguments));
		while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS)) {
			TRY_LET(Expression, argument, parseExpression, tokens);
			TRY(appendToExpressionList(&arguments, argument));

			// Comma
			if (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS)) {
				TRY(popToken(tokens, TOKEN_TYPE_COMMA, &next));
			}
		}

		TRY(popToken(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS, &next));

		UnaryExpression* unary = malloc(sizeof(UnaryExpression));
		ASSERT_NONNULL(unary);
		*unary = (UnaryExpression) {
			.expression = expression,
			.operation = (UnaryOperation) {
				.type = UNARY_OPERATION_FUNCTION_CALL,
				.data = (UnaryOperationData) {
					.functionCall = arguments,
				},
			},
		};

		expression = (Expression) {
			.type = EXPRESSION_UNARY,
			.data = (ExpressionData) {
				.unary = unary,
			},
		};

		DEBUG_END("Done parsing postfix expression");
	}

	RETURN_OK(output, expression);
}

PRIVATE Result parsePrefixExpression(TokenList* tokens, Expression* output) {
	if (nextTokenIs(tokens, TOKEN_TYPE_KEYWORD_NOT)) {
		DEBUG_START("Parsing", "prefix expression");

		UNWRAP_LET(String, next, popToken, tokens, TOKEN_TYPE_KEYWORD_NOT);
		TRY_LET(Expression, inner, parsePrefixExpression, tokens);

		UnaryExpression* unary = malloc(sizeof(UnaryExpression));
		ASSERT_NONNULL(unary);
		*unary = (UnaryExpression) {
			.expression = inner,
			.operation = (UnaryOperation) {
				.type = UNARY_OPERATION_NOT,
			},
		};
		Expression expression = (Expression) {
			.type = EXPRESSION_UNARY,
			.data = (ExpressionData) {
				.unary = unary,
			},
		};

		DEBUG_END("parsing prefix expression");
		RETURN_OK(output, expression);
	}

	return parsePostfixExpression(tokens, output);
}

PRIVATE Result parseDeclaration(TokenList* tokens, Declaration* output) {
	String next;
	TRY(popToken(tokens, TOKEN_TYPE_KEYWORD_LET, &next));

	// Name
	String name;
	TRY(popToken(tokens, TOKEN_TYPE_IDENTIFIER, &name));

	// Type
	Type* type = NULL;
	TokenType nextTokenType;
	TRY(peekTokenType(tokens, &nextTokenType));
	if (nextTokenType == TOKEN_TYPE_COLON) {
		TRY(popToken(tokens, TOKEN_TYPE_COLON, &next));
		type = malloc(sizeof(Type));
		TRY(parseType(tokens, type));
	}

	// Value
	TRY(popToken(tokens, TOKEN_TYPE_EQUALS, &next));
	Expression value;
	TRY(parseExpression(tokens, &value));

	// Allocate & return
	Declaration declaration = (Declaration) {
		.name = name,
		.type = type,
		.value = value,
	};
	RETURN_OK(output, declaration);
}

PRIVATE Result parseStatement(TokenList* tokens, Statement* output) {
	DEBUG_START("Parsing", "statement");

	TokenType nextTokenType;
	TRY(peekTokenType(tokens, &nextTokenType));

	switch (nextTokenType) {
		case TOKEN_TYPE_KEYWORD_LET: {
			DEBUG_START("Parsing", "declaration");
			Declaration declaration;
			TRY(parseDeclaration(tokens, &declaration));
			Statement statement = (Statement) {
				.type = STATEMENT_DECLARATION,
				.data = (StatementData) {
					.declaration = declaration,
				},
			};
			String next;
			TRY(popToken(tokens, TOKEN_TYPE_SEMICOLON, &next));
			DEBUG_END("Parsing declaration");
			DEBUG_END("Parsing statement");
			RETURN_OK(output, statement);
		}
		case TOKEN_TYPE_KEYWORD_RETURN: {
			DEBUG_START("Parsing", "return statement");
			UNWRAP_LET(String, next, popToken, tokens, TOKEN_TYPE_KEYWORD_RETURN);
			Expression expression;
			TRY(parseExpression(tokens, &expression));
			Statement statement = (Statement) {
				.type = STATEMENT_RETURN,
				.data = (StatementData) {
					.returnExpression = expression,
				},
			};
			TRY(popToken(tokens, TOKEN_TYPE_SEMICOLON, &next));
			DEBUG_END("Parsing return statement");
			DEBUG_END("Parsing statement");
			RETURN_OK(output, statement);
		}
		default: {
			DEBUG_START("Parsing", "expression statement");

			Expression expression;
			TRY(parseExpression(tokens, &expression));
			Statement statement = (Statement) {
				.type = STATEMENT_EXPRESSION,
				.data = (StatementData) {
					.expression = expression,
				},
			};
			String next;
			TRY(popToken(tokens, TOKEN_TYPE_SEMICOLON, &next));
			DEBUG_END("Parsing expression statement");
			DEBUG_END("Parsing statement");
			RETURN_OK(output, statement);
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
Result parse(TokenList* tokens, Program* output) {
	StatementList statements;
	TRY(emptyStatementList(&statements));
	while (!isTokenListEmpty(*tokens)) {
		Statement statement;
		TRY(parseStatement(tokens, &statement));
		TRY(appendToStatementList(&statements, statement));
	}

	RETURN_OK(output, (Program) {.statements = statements});
}

PRIVATE void freeExpression(Expression expression) {
	switch (expression.type) {
		case EXPRESSION_OBJECT: {

			// Free internals
			FOR_EACH(InternalField internalField, expression.data.object->internals) {
				free(internalField.value);
			}
			END;
			free(expression.data.object->internals.data);

			free(expression.data.object->fields.data);
			free(expression.data.object);
			break;
		}
		case EXPRESSION_UNARY: {
			free(expression.data.unary);
		}
	}
}

PRIVATE void freeStatement(Statement statement) {
	switch (statement.type) {
		case STATEMENT_DECLARATION: {
			free(statement.data.declaration.name);
			break;
		};
		case STATEMENT_EXPRESSION: {
			freeExpression(statement.data.expression);
			break;
		}
	}
}

String expressionTypeName(ExpressionType type) {
	switch (type) {
		case EXPRESSION_BOOLEAN:
			return "boolean";
		case EXPRESSION_IDENTIFIER:
			return "identifier";
		case EXPRESSION_OBJECT:
			return "object literal";
		case EXPRESSION_BINARY:
			return "binary expression";
		case EXPRESSION_FUNCTION:
			return "function literal";
		case EXPRESSION_BLOCK:
			return "block expression";
		case EXPRESSION_UNARY:
			return "unary expression";
		case EXPRESSION_BUILTIN_FUNCTION:
			return "builtin function";
		case EXPRESSION_TYPE:
			return "type";
		case EXPRESSION_WHILE_LOOP:
			return "while loop expression";
		case EXPRESSION_FOR_LOOP:
			return "for loop expression";
		case EXPRESSION_IF:
			return "if expression";
	}
}

void freeProgram(Program program) {
	FOR_EACH(Statement statement, program.statements) {
		freeStatement(statement);
	}
	END;

	free(program.statements.data);
}

IMPLEMENT_LIST(Declaration)
IMPLEMENT_LIST(Statement)
IMPLEMENT_LIST(Expression)
IMPLEMENT_LIST(Parameter)
IMPLEMENT_LIST(Field)
IMPLEMENT_LIST(InternalField)
