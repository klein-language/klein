#include "../include/parser.h"
#include "../include/context.h"
#include "../include/lexer.h"
#include "../include/list.h"
#include "../include/result.h"
#include "../include/sugar.h"
#include "../include/util.h"

#include <stdlib.h>
#include <string.h>

PRIVATE Result parseExpression(Context* context, TokenList* tokens, Expression* output);

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
	DEBUG("Getting field \"%s\" on an object\n", name);
	FOR_EACH_REF(Field * field, object.fields) {
		if (strcmp(field->name, name) == 0) {
			RETURN_OK(output, &field->value);
		}
	}
	END;

	DEBUG("No field \"%s\" found on object\n", name);
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

PRIVATE Result parseLiteral(Context* context, TokenList* tokens, Expression* output);
PRIVATE Result parseStatement(Context* context, TokenList* tokens, Statement* output);
PRIVATE Result parseType(Context* context, TokenList* tokens, Type* output);

PRIVATE Result parseTypeLiteral(Context* context, TokenList* tokens, TypeLiteral* output) {
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
				TRY(parseType(context, tokens, &type));
				TRY(appendToParameterList(&parameterTypes, (Parameter) {.name = "", .type = type}));
			}
			TRY(popToken(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS, &next));

			// Return type
			TRY(popToken(tokens, TOKEN_TYPE_COLON, &next));
			Type returnType;
			TRY(parseType(context, tokens, &returnType));

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

PRIVATE Result parseType(Context* context, TokenList* tokens, Type* output) {
	TypeLiteral literal;
	TRY(parseTypeLiteral(context, tokens, &literal));
	RETURN_OK(output, ((Type) {.type = TYPE_LITERAL, .data = (TypeData) {.literal = literal}}));
}

PRIVATE Result parseBlock(Context* context, TokenList* tokens, Block* output) {
	TRY(enterNewScope(context));
	TRY_LET(String, next, popToken, tokens, TOKEN_TYPE_LEFT_BRACE);

	// Parse statements
	StatementList* statements;
	TRY(emptyHeapStatementList(&statements));
	while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_BRACE)) {
		Statement statement;
		TRY(parseStatement(context, tokens, &statement));
		TRY(appendToStatementList(statements, statement));
	}

	UNWRAP(popToken(tokens, TOKEN_TYPE_RIGHT_BRACE, &next));

	Block block = (Block) {
		.statements = statements,
		.innerScope = context->scope,
	};

	TRY(exitScope(context));

	RETURN_OK(output, block);
}

Result parseObjectLiteral(Context* context, TokenList* tokens, Expression* output) {
	TRY_LET(String, next, popToken, tokens, TOKEN_TYPE_LEFT_BRACE);

	// Fields
	FieldList fields;
	TRY(emptyFieldList(&fields));
	while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_BRACE)) {
		TRY_LET(String, name, popToken, tokens, TOKEN_TYPE_IDENTIFIER);
		TRY(popToken(tokens, TOKEN_TYPE_EQUALS, &next));
		TRY_LET(Expression, value, parseExpression, context, tokens);
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

PRIVATE Result parseLiteral(Context* context, TokenList* tokens, Expression* output) {
	DEBUG("Parsing literal\n");
	TRY_LET(TokenType, nextTokenType, peekTokenType, tokens);
	switch (nextTokenType) {

		// String
		case TOKEN_TYPE_STRING: {
			UNWRAP_LET(String, value, popToken, tokens, TOKEN_TYPE_STRING);
			TRY_LET(Expression, expression, stringExpression, value);
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
			RETURN_OK(output, expression);
		}

		case TOKEN_TYPE_LEFT_BRACE: {
			return parseObjectLiteral(context, tokens, output);
		}

		// Number
		case TOKEN_TYPE_NUMBER: {
			UNWRAP_LET(String, numberString, popToken, tokens, TOKEN_TYPE_NUMBER);
			TRY_LET(Expression, expression, numberExpression, atof(numberString));
			RETURN_OK(output, expression);
		}

		// Block
		case TOKEN_TYPE_KEYWORD_DO: {
			String next;
			TRY(popToken(tokens, TOKEN_TYPE_KEYWORD_DO, &next));

			Block block;
			TRY(parseBlock(context, tokens, &block));
			Block* heapBlock = malloc(sizeof(Block));
			*heapBlock = block;

			Expression expression = (Expression) {
				.type = EXPRESSION_BLOCK,
				.data = (ExpressionData) {
					.block = heapBlock,
				},
			};

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
			RETURN_OK(output, expression);
		}

		// Function
		case TOKEN_TYPE_KEYWORD_FUNCTION: {
			String next;
			TRY(popToken(tokens, TOKEN_TYPE_KEYWORD_FUNCTION, &next));

			// Parameters
			ParameterList parameters;
			TRY(emptyParameterList(&parameters));

			TRY(popToken(tokens, TOKEN_TYPE_LEFT_PARENTHESIS, &next));
			while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS)) {
				String name;
				TRY(popToken(tokens, TOKEN_TYPE_STRING, &name));

				TRY(popToken(tokens, TOKEN_TYPE_COLON, &next));

				Type type;
				TRY(parseType(context, tokens, &type));

				Parameter parameter = (Parameter) {
					.type = type,
					.name = name,
				};

				TRY(appendToParameterList(&parameters, parameter));
			}
			TRY(popToken(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS, &next));

			// Return type
			TRY(popToken(tokens, TOKEN_TYPE_COLON, &next));
			Type returnType;
			TRY(parseType(context, tokens, &returnType));

			// Body
			Block body;
			TRY(parseBlock(context, tokens, &body));

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
				TRY(parseType(context, tokens, &fieldType));

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

PRIVATE Result parseBinaryOperation(Context* context, TokenList* tokens, BinaryOperator operator, Expression * output);
PRIVATE Result parsePrefixExpression(Context* context, TokenList* tokens, Expression* output);

PRIVATE Result parsePrecedentBinaryOperation(Context* context, TokenList* tokens, BinaryOperator operator, Expression * output) {
	if (operator.precedent == NULL) {
		return parsePrefixExpression(context, tokens, output);
	}

	return parseBinaryOperation(context, tokens, *operator.precedent, output);
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
	return ERROR_INTERNAL;
}

PRIVATE Result parseFieldAccess(Context* context, TokenList* tokens, Expression* output) {
	TRY_LET(Expression, left, parseLiteral, context, tokens);

	String next;

	while (nextTokenIs(tokens, TOKEN_TYPE_DOT)) {
		DEBUG("Parsing field access\n");
		UNWRAP(popToken(tokens, TOKEN_TYPE_DOT, &next));
		Expression* right = malloc(sizeof(Expression));
		TRY(parseLiteral(context, tokens, right));

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
	}

	RETURN_OK(output, left);
}

PRIVATE Result parseBinaryOperation(Context* context, TokenList* tokens, BinaryOperator operator, Expression * output) {

	Expression left;
	TRY(parsePrecedentBinaryOperation(context, tokens, operator, & left));

	String next;
	while (nextTokenIsOneOf(tokens, operator.tokenTypes, operator.tokenTypeCount)) {
		UNWRAP(popAnyToken(tokens, &next));

		DEBUG("Parsing binary expression \"%s\"\n", next);
		BinaryOperation operation;
		TRY(binaryOperationOf(next, &operation));

		Expression* right = malloc(sizeof(Expression));
		TRY(parsePrecedentBinaryOperation(context, tokens, operator, right));

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
	}

	RETURN_OK(output, left);
}

PRIVATE Result parseExpression(Context* context, TokenList* tokens, Expression* output) {
	return parseBinaryOperation(context, tokens, COMPARISON, output);
}

PRIVATE Result parsePostfixExpression(Context* context, TokenList* tokens, Expression* output) {
	TRY_LET(Expression, expression, parseFieldAccess, context, tokens);

	while (nextTokenIs(tokens, TOKEN_TYPE_LEFT_PARENTHESIS)) {
		DEBUG("Parsing postfix expression\n");

		TRY_LET(String, next, popToken, tokens, TOKEN_TYPE_LEFT_PARENTHESIS);

		// Parse arguments
		ExpressionList arguments;
		TRY(emptyExpressionList(&arguments));
		while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS)) {
			TRY_LET(Expression, argument, parseExpression, context, tokens);
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
	}

	RETURN_OK(output, expression);
}

PRIVATE Result parsePrefixExpression(Context* context, TokenList* tokens, Expression* output) {
	if (nextTokenIs(tokens, TOKEN_TYPE_KEYWORD_NOT)) {
		DEBUG("Parsing prefix expression\n");

		UNWRAP_LET(String, next, popToken, tokens, TOKEN_TYPE_KEYWORD_NOT);
		TRY_LET(Expression, inner, parsePrefixExpression, context, tokens);

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

		RETURN_OK(output, expression);
	}

	return parsePostfixExpression(context, tokens, output);
}

PRIVATE Result parseDeclaration(Context* context, TokenList* tokens, Declaration* output) {
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
		TRY(parseType(context, tokens, type));
	}

	// Value
	TRY(popToken(tokens, TOKEN_TYPE_EQUALS, &next));
	Expression value;
	TRY(parseExpression(context, tokens, &value));

	// Allocate & return
	Declaration declaration = (Declaration) {
		.name = name,
		.type = type,
		.value = value,
	};
	RETURN_OK(output, declaration);
}

PRIVATE Result parseStatement(Context* context, TokenList* tokens, Statement* output) {
	TokenType nextTokenType;
	TRY(peekTokenType(tokens, &nextTokenType));

	switch (nextTokenType) {
		case TOKEN_TYPE_KEYWORD_LET: {
			Declaration declaration;
			TRY(parseDeclaration(context, tokens, &declaration));
			Statement statement = (Statement) {
				.type = STATEMENT_DECLARATION,
				.data = (StatementData) {
					.declaration = declaration,
				},
			};
			String next;
			TRY(popToken(tokens, TOKEN_TYPE_SEMICOLON, &next));
			RETURN_OK(output, statement);
		}
		default: {
			Expression expression;
			TRY(parseExpression(context, tokens, &expression));
			Statement statement = (Statement) {
				.type = STATEMENT_EXPRESSION,
				.data = (StatementData) {
					.expression = expression,
				},
			};
			String next;
			TRY(popToken(tokens, TOKEN_TYPE_SEMICOLON, &next));
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
Result parse(void* context, TokenList* tokens, Program* output) {
	StatementList statements;
	TRY(emptyStatementList(&statements));
	while (!isTokenListEmpty(*tokens)) {
		Statement statement;
		TRY(parseStatement(context, tokens, &statement));
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
