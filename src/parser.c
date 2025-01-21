#include "../include/parser.h"
#include "../include//klein.h"
#include "../include/context.h"
#include "../include/list.h"
#include "../include/result.h"
#include "../include/util.h"

#include <stdlib.h>
#include <string.h>

PRIVATE KleinResult parseLiteral(TokenList* tokens, Expression* output);
PRIVATE KleinResult parseStatement(TokenList* tokens, Statement* output);
PRIVATE KleinResult parseType(TokenList* tokens, Type* output);
PRIVATE KleinResult parseExpression(TokenList* tokens, Expression* output);
PRIVATE KleinResult parseBinaryOperation(TokenList* tokens, BinaryOperator operator, Expression * output);
PRIVATE KleinResult parsePrefixExpression(TokenList* tokens, Expression* output);

bool hasInternal(Value value, InternalKey key) {
	void* output;
	return isOk(getValueInternal(value, key, &output));
}

KleinResult getValueInternal(Value value, InternalKey key, void** output) {
	FOR_EACH(Internal internal, value.internals) {
		if (internal.key == key) {
			RETURN_OK(output, internal.value);
		}
	}
	END;

	return (KleinResult) {
		.type = KLEIN_ERROR_INTERNAL,
	};
}

KleinResult getValueField(Value value, String name, Value** output) {
	FOR_EACH_REFP(ValueField * field, value.fields) {
		if (strcmp(field->name, name) == 0) {
			RETURN_OK(output, &field->value);
		}
	}
	END;

	Value* heapValue = malloc(sizeof(Value));
	*heapValue = value;
	return (KleinResult) {
		.type = KLEIN_ERROR_MISSING_FIELD,
		.data = (KleinResultData) {
			.missingField = {
				.value = heapValue,
				.name = name,
			},
		},
	};
}

PRIVATE KleinResult popToken(TokenList* tokens, TokenType type, String* output) {

	// Empty token stream - error
	if (tokens->size == 0) {
		return (KleinResult) {
			.type = KLEIN_ERROR_UNEXPECTED_TOKEN,
			.data = (KleinResultData) {
				.unexpectedToken = (KleinUnexpectedTokenError) {
					.actual = TOKEN_TYPE_EOF,
					.expected = type,
				},
			},
		};
	}

	// Check token
	Token token = tokens->data[0];
	if (token.type != type) {
		return (KleinResult) {
			.type = KLEIN_ERROR_UNEXPECTED_TOKEN,
			.data = (KleinResultData) {
				.unexpectedToken = (KleinUnexpectedTokenError) {
					.actual = token.type,
					.expected = type,
				},
			},
		};
	}

	// Update list
	tokens->data++;
	tokens->size--;
	tokens->capacity--;

	// Return token
	RETURN_OK(output, token.value);
}

PRIVATE KleinResult popAnyToken(TokenList* tokens, String* output) {
	// Get token
	Token token = tokens->data[0];

	// Update list
	tokens->data++;
	tokens->size--;
	tokens->capacity--;

	// Return token
	RETURN_OK(output, token.value);
}

PRIVATE KleinResult peekTokenType(TokenList* tokens, TokenType* output) {
	if (tokens->size == 0) {
		return (KleinResult) {
			.type = KLEIN_ERROR_PEEK_EMPTY_TOKEN_STREAM,
		};
	}

	RETURN_OK(output, tokens->data[0].type);
}

PRIVATE bool nextTokenIs(TokenList* tokens, TokenType type) {
	if (tokens->size == 0) {
		return false;
	}

	return tokens->data[0].type == type;
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

PRIVATE KleinResult parseTypeLiteral(TokenList* tokens, TypeLiteral* output) {
	TokenType nextTokenType;
	TRY(peekTokenType(tokens, &nextTokenType));
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
			ParameterList parameterTypes = emptyParameterList();
			TRY(popToken(tokens, TOKEN_TYPE_LEFT_PARENTHESIS, &next));
			while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS)) {
				Type type;
				TRY(parseType(tokens, &type));
				appendToParameterList(&parameterTypes, (Parameter) {.name = "", .type = type});
			}
			TRY(popToken(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS, &next));

			// Return type
			TRY(popToken(tokens, TOKEN_TYPE_COLON, &next));
			Type returnType;
			TRY(parseType(tokens, &returnType));

			// Create function
			Function* function = malloc(sizeof(Function));
			*function = (Function) {
				.parameters = parameterTypes,
				.returnType = returnType,
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
			return (KleinResult) {
				.type = KLEIN_ERROR_UNEXPECTED_TOKEN,
				.data = (KleinResultData) {
					.unexpectedToken = (KleinUnexpectedTokenError) {
						.actual = nextTokenType,
						.expected = TOKEN_TYPE_IDENTIFIER,
					},
				},
			};
		}
	}
}

PRIVATE KleinResult parseType(TokenList* tokens, Type* output) {
	TypeLiteral literal;
	TRY(parseTypeLiteral(tokens, &literal));
	RETURN_OK(output, ((Type) {.type = TYPE_LITERAL, .data = (TypeData) {.literal = literal}}));
}

/**
 * Parses a `block expression`.
 *
 * Syntax: `"{" <statement>* "}"`
 *
 * # Parameters
 *
 * - `tokens` - The token list to parse from
 * - `output` - Where to place the parsed output
 *
 * # Returns
 *
 * A result, indicating success or failure. If failed, contains the error message.
 *
 * # Errors
 *
 * If an unexpected token was encountered (including the token stream running out of tokens
 * unexpectedly), an error is returned. If memory fails to allocate, an error is returned.
 */
PRIVATE KleinResult parseBlock(TokenList* tokens, Block* output) {
	TRY(enterNewScope());
	TRY_LET(String next, popToken(tokens, TOKEN_TYPE_LEFT_BRACE, &next));

	// Parse statements
	StatementList* statements = emptyHeapStatementList();
	while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_BRACE)) {
		Statement statement;
		TRY(parseStatement(tokens, &statement));
		appendToStatementList(statements, statement);
	}

	UNWRAP(popToken(tokens, TOKEN_TYPE_RIGHT_BRACE, &next));

	Block block = (Block) {
		.statements = statements,
		.innerScope = CONTEXT->scope,
	};

	TRY(exitScope());

	RETURN_OK(output, block);
}

/**
 * Parses an `object literal expression`.
 *
 * Syntax:
 *
 * ```
 * <field> ::= <identifier> "=" <value>
 * <object> ::= "{" ( <field> ("," <field>)* ","? )? "}"`
 * ```
 *
 * # Parameters
 *
 * - `tokens` - The token list to parse from
 * - `output` - Where to place the parsed output
 *
 * # Returns
 *
 * A result, indicating success or failure. If failed, contains the error message.
 *
 * # Errors
 *
 * If an unexpected token was encountered (including the token stream running out of tokens
 * unexpectedly), an error is returned. If memory fails to allocate, an error is returned.
 */
PRIVATE KleinResult parseObjectLiteral(TokenList* tokens, Expression* output) {
	TRY_LET(String next, popToken(tokens, TOKEN_TYPE_LEFT_BRACE, &next));

	// Fields
	FieldList fields = emptyFieldList();
	while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_BRACE)) {
		TRY_LET(String name, popToken(tokens, TOKEN_TYPE_IDENTIFIER, &name));
		TRY(popToken(tokens, TOKEN_TYPE_EQUALS, &next));
		TRY_LET(Expression value, parseExpression(tokens, &value));
		appendToFieldList(&fields, (Field) {.name = name, .value = value});
		if (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_BRACE)) {
			TRY(popToken(tokens, TOKEN_TYPE_COMMA, &next));
		}
	}

	TRY(popToken(tokens, TOKEN_TYPE_RIGHT_BRACE, &next));

	Object* object = malloc(sizeof(Object));
	*object = (Object) {
		.fields = fields,
	};
	Expression expression = (Expression) {
		.type = EXPRESSION_OBJECT,
		.data = (ExpressionData) {
			.object = object,
		},
	};
	RETURN_OK(output, expression);
}

/**
 * Parses a `string literal expression`.
 *
 * Syntax: `<string>`
 *
 * # Parameters
 *
 * - `tokens` - The token list to parse from
 * - `output` - Where to place the parsed output
 *
 * # Returns
 *
 * A result, indicating success or failure. If failed, contains the error message.
 *
 * # Errors
 *
 * If an unexpected token was encountered (including the token stream running out of tokens
 * unexpectedly), an error is returned. If memory fails to allocate, an error is returned.
 */
PRIVATE KleinResult parseStringLiteral(TokenList* tokens, Expression* output) {
	UNWRAP_LET(String value, popToken(tokens, TOKEN_TYPE_STRING, &value));
	Expression expression = (Expression) {
		.type = EXPRESSION_STRING,
		.data = (ExpressionData) {
			.string = value,
		},
	};
	RETURN_OK(output, expression);
}

/**
 * Parses an `identifier literal expression`.
 *
 * Syntax: `<identifier>`
 *
 * # Parameters
 *
 * - `tokens` - The token list to parse from
 * - `output` - Where to place the parsed output
 *
 * # Returns
 *
 * A result, indicating success or failure. If failed, contains the error message.
 *
 * # Errors
 *
 * If an unexpected token was encountered (including the token stream running out of tokens
 * unexpectedly), an error is returned. If memory fails to allocate, an error is returned.
 */
PRIVATE KleinResult parseIdentifierLiteral(TokenList* tokens, Expression* output) {
	UNWRAP_LET(String identifier, popToken(tokens, TOKEN_TYPE_IDENTIFIER, &identifier));
	Expression expression = (Expression) {
		.type = EXPRESSION_IDENTIFIER,
		.data = (ExpressionData) {
			.identifier = identifier,
		},
	};
	RETURN_OK(output, expression);
}

/**
 * Parses a `list literal expression`.
 *
 * Syntax: `"[" ( <expression> ("," <expression>)* ","? )? "]"`
 *
 * # Parameters
 *
 * - `tokens` - The token list to parse from
 * - `output` - Where to place the parsed output
 *
 * # Returns
 *
 * A result, indicating success or failure. If failed, contains the error message.
 *
 * # Errors
 *
 * If an unexpected token was encountered (including the token stream running out of tokens
 * unexpectedly), an error is returned. If memory fails to allocate, an error is returned.
 */
PRIVATE KleinResult parseListLiteral(TokenList* tokens, Expression* output) {
	UNWRAP_LET(String next, popToken(tokens, TOKEN_TYPE_LEFT_BRACKET, &next));

	ExpressionList* elements = emptyHeapExpressionList();
	while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_BRACKET)) {
		TRY_LET(Expression element, parseExpression(tokens, &element));
		appendToExpressionList(elements, element);
		if (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_BRACKET)) {
			TRY(popToken(tokens, TOKEN_TYPE_COMMA, &next));
		}
	}
	UNWRAP(popToken(tokens, TOKEN_TYPE_RIGHT_BRACKET, &next));

	Expression list = (Expression) {
		.type = EXPRESSION_LIST,
		.data = (ExpressionData) {
			.list = elements,
		},
	};

	RETURN_OK(output, list);
}

/**
 * Parses a `for loop expression`.
 *
 * Syntax: `"for" <identifier> "in" <expression> <block>`
 *
 * # Parameters
 *
 * - `tokens` - The token list to parse from
 * - `output` - Where to place the parsed output
 *
 * # Returns
 *
 * A result, indicating success or failure. If failed, contains the error message.
 *
 * # Errors
 *
 * If an unexpected token was encountered (including the token stream running out of tokens
 * unexpectedly), an error is returned. If memory fails to allocate, an error is returned.
 */
PRIVATE KleinResult parseForLoop(TokenList* tokens, Expression* output) {
	UNWRAP_LET(String next, popToken(tokens, TOKEN_TYPE_KEYWORD_FOR, &next));
	TRY_LET(String binding, popToken(tokens, TOKEN_TYPE_IDENTIFIER, &binding));
	TRY(popToken(tokens, TOKEN_TYPE_KEYWORD_IN, &next));
	TRY_LET(Expression list, parseExpression(tokens, &list));
	TRY_LET(Block body, parseBlock(tokens, &body));

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
	RETURN_OK(output, loop);
}

/**
 * Parses a `while loop expression`.
 *
 * Syntax: `"while" <expression> <block>`
 *
 * # Parameters
 *
 * - `tokens` - The token list to parse from
 * - `output` - Where to place the parsed output
 *
 * # Returns
 *
 * A result, indicating success or failure. If failed, contains the error message.
 *
 * # Errors
 *
 * If an unexpected token was encountered (including the token stream running out of tokens
 * unexpectedly), an error is returned. If memory fails to allocate, an error is returned.
 */
PRIVATE KleinResult parseWhileLoop(TokenList* tokens, Expression* output) {
	UNWRAP_LET(String next, popToken(tokens, TOKEN_TYPE_KEYWORD_WHILE, &next));
	TRY_LET(Expression condition, parseExpression(tokens, &condition));
	TRY_LET(Block body, parseBlock(tokens, &body));

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
	RETURN_OK(output, loop);
}

/**
 * Parses an `if-expression`.
 *
 * Syntax: `"if" <expression> <block> ("else" "if" <expression> <block>)* ("else" <block>)?`
 *
 * # Parameters
 *
 * - `tokens` - The token list to parse from
 * - `output` - Where to place the parsed output
 *
 * # Returns
 *
 * A result, indicating success or failure. If failed, contains the error message.
 *
 * # Errors
 *
 * If an unexpected token was encountered (including the token stream running out of tokens
 * unexpectedly), an error is returned. If memory fails to allocate, an error is returned.
 */
PRIVATE KleinResult parseIfExpression(TokenList* tokens, Expression* output) {
	UNWRAP_LET(String next, popToken(tokens, TOKEN_TYPE_KEYWORD_IF, &next));
	TRY_LET(Expression condition, parseExpression(tokens, &condition));
	TRY_LET(Block body, parseBlock(tokens, &body));

	IfExpressionList* elseIfs = emptyHeapIfExpressionList();

	IfExpression ifExpression = (IfExpression) {
		.condition = condition,
		.body = body,
	};
	appendToIfExpressionList(elseIfs, ifExpression);

	while (nextTokenIs(tokens, TOKEN_TYPE_KEYWORD_ELSE)) {
		UNWRAP(popToken(tokens, TOKEN_TYPE_KEYWORD_ELSE, &next));

		// Else-if block
		if (nextTokenIs(tokens, TOKEN_TYPE_KEYWORD_IF)) {
			UNWRAP(popToken(tokens, TOKEN_TYPE_KEYWORD_IF, &next));
			TRY_LET(Expression elseIfCondition, parseExpression(tokens, &elseIfCondition));
			TRY_LET(Block elseIfBody, parseBlock(tokens, &elseIfBody));
			IfExpression elseIfExpression = (IfExpression) {
				.condition = elseIfCondition,
				.body = elseIfBody,
			};
			appendToIfExpressionList(elseIfs, elseIfExpression);
		}

		// Else block
		else {
			TRY_LET(Block elseIfBody, parseBlock(tokens, &elseIfBody));
			IfExpression elseIfExpression = (IfExpression) {
				.condition = (Expression) {
					.type = EXPRESSION_BOOLEAN,
					.data = (ExpressionData) {
						.boolean = true,
					},
				},
				.body = elseIfBody,
			};
			appendToIfExpressionList(elseIfs, elseIfExpression);
		}
	}

	Expression loop = (Expression) {
		.type = EXPRESSION_IF,
		.data = (ExpressionData) {
			.ifExpression = elseIfs,
		},
	};
	RETURN_OK(output, loop);
}

/**
 * Parses a `number literal expression`.
 *
 * Syntax: `<number>`
 *
 * # Parameters
 *
 * - `tokens` - The token list to parse from
 * - `output` - Where to place the parsed output
 *
 * # Returns
 *
 * A result, indicating success or failure. If failed, contains the error message.
 *
 * # Errors
 *
 * If an unexpected token was encountered (including the token stream running out of tokens
 * unexpectedly), an error is returned. If memory fails to allocate, an error is returned.
 */
PRIVATE KleinResult parseNumberLiteral(TokenList* tokens, Expression* output) {
	UNWRAP_LET(String value, popToken(tokens, TOKEN_TYPE_NUMBER, &value));
	Expression expression = (Expression) {
		.type = EXPRESSION_NUMBER,
		.data = (ExpressionData) {
			.number = atof(value),
		},
	};
	RETURN_OK(output, expression);
}

/**
 * Parses a `do block expression`.
 *
 * Syntax: `"do" <block>`
 *
 * # Parameters
 *
 * - `tokens` - The token list to parse from
 * - `output` - Where to place the parsed output
 *
 * # Returns
 *
 * A result, indicating success or failure. If failed, contains the error message.
 *
 * # Errors
 *
 * If an unexpected token was encountered (including the token stream running out of tokens
 * unexpectedly), an error is returned. If memory fails to allocate, an error is returned.
 */
PRIVATE KleinResult parseDoBlock(TokenList* tokens, Expression* output) {
	UNWRAP_LET(String next, popToken(tokens, TOKEN_TYPE_KEYWORD_DO, &next));

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

	RETURN_OK(output, expression);
}

/**
 * Parses a `function literal expression`.
 *
 * Syntax:
 *
 * ```
 * <parameter> ::= <identifier> ":" <type>
 * <function> ::= "function" "(" ( <parameter> ("," <parameter>)* ","? )? ")" ":" <type> <block>`
 * ```
 *
 * # Parameters
 *
 * - `tokens` - The token list to parse from
 * - `output` - Where to place the parsed output
 *
 * # Returns
 *
 * A result, indicating success or failure. If failed, contains the error message.
 *
 * # Errors
 *
 * If an unexpected token was encountered (including the token stream running out of tokens
 * unexpectedly), an error is returned. If memory fails to allocate, an error is returned.
 */
PRIVATE KleinResult parseFunctionLiteral(TokenList* tokens, Expression* output) {
	UNWRAP_LET(String next, popToken(tokens, TOKEN_TYPE_KEYWORD_FUNCTION, &next));

	// Parameters
	ParameterList parameters = emptyParameterList();

	TRY(popToken(tokens, TOKEN_TYPE_LEFT_PARENTHESIS, &next));

	while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS)) {
		TRY_LET(String name, popToken(tokens, TOKEN_TYPE_IDENTIFIER, &name));
		TRY(popToken(tokens, TOKEN_TYPE_COLON, &next));
		TRY_LET(Type type, parseType(tokens, &type));

		Parameter parameter = (Parameter) {
			.type = type,
			.name = name,
		};

		appendToParameterList(&parameters, parameter);

		if (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS)) {
			TRY(popToken(tokens, TOKEN_TYPE_COMMA, &next));
		}
	}
	UNWRAP(popToken(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS, &next));

	// Return type
	TRY(popToken(tokens, TOKEN_TYPE_COLON, &next));
	TRY_LET(Type returnType, parseType(tokens, &returnType));

	// Body
	TRY_LET(Block body, parseBlock(tokens, &body));

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

/**
 * Parses a `parenthesized expression`.
 *
 * Syntax: `"(" <expression> ")"`
 *
 * # Parameters
 *
 * - `tokens` - The token list to parse from
 * - `output` - Where to place the parsed output
 *
 * # Returns
 *
 * A result, indicating success or failure. If failed, contains the error message.
 *
 * # Errors
 *
 * If an unexpected token was encountered (including the token stream running out of tokens
 * unexpectedly), an error is returned. If memory fails to allocate, an error is returned.
 */
PRIVATE KleinResult parseParenthesizedExpression(TokenList* tokens, Expression* output) {
	TRY_LET(String next, popToken(tokens, TOKEN_TYPE_LEFT_PARENTHESIS, &next));
	TRY(parseExpression(tokens, output));
	TRY(popToken(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS, &next));
	return OK;
}

PRIVATE KleinResult parseLiteral(TokenList* tokens, Expression* output) {
	TRY_LET(TokenType nextTokenType, peekTokenType(tokens, &nextTokenType));

	switch (nextTokenType) {
		case TOKEN_TYPE_STRING: {
			return parseStringLiteral(tokens, output);
		}
		case TOKEN_TYPE_IDENTIFIER: {
			return parseIdentifierLiteral(tokens, output);
		}
		case TOKEN_TYPE_LEFT_BRACKET: {
			return parseListLiteral(tokens, output);
		}
		case TOKEN_TYPE_KEYWORD_FOR: {
			return parseForLoop(tokens, output);
		}
		case TOKEN_TYPE_KEYWORD_WHILE: {
			return parseWhileLoop(tokens, output);
		}
		case TOKEN_TYPE_KEYWORD_IF: {
			return parseIfExpression(tokens, output);
		}
		case TOKEN_TYPE_LEFT_BRACE: {
			return parseObjectLiteral(tokens, output);
		}
		case TOKEN_TYPE_NUMBER: {
			return parseNumberLiteral(tokens, output);
		}
		case TOKEN_TYPE_KEYWORD_DO: {
			return parseDoBlock(tokens, output);
		}
		case TOKEN_TYPE_KEYWORD_FUNCTION: {
			return parseFunctionLiteral(tokens, output);
		}
		case TOKEN_TYPE_LEFT_PARENTHESIS: {
			return parseParenthesizedExpression(tokens, output);
		}
		default: {
			return (KleinResult) {
				.type = KLEIN_ERROR_UNEXPECTED_TOKEN,
				.data = (KleinResultData) {
					.unexpectedToken = (KleinUnexpectedTokenError) {
						.actual = nextTokenType,
						.expected = TOKEN_TYPE_STRING,
					},
				},
			};
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
static BinaryOperator COMBINATOR = (BinaryOperator) {
	.precedent = &COMPARISON,
	.tokenTypes = (TokenType[]) {
		TOKEN_TYPE_KEYWORD_AND,
		TOKEN_TYPE_KEYWORD_OR,
	},
	.tokenTypeCount = 2,
};

static BinaryOperator ASSIGNMENT = (BinaryOperator) {
	.precedent = &COMBINATOR,
	.tokenTypes = (TokenType[]) {
		TOKEN_TYPE_EQUALS,
	},
	.tokenTypeCount = 1,
};

PRIVATE KleinResult parsePrecedentBinaryOperation(TokenList* tokens, BinaryOperator operator, Expression * output) {
	if (operator.precedent == NULL) {
		return parsePrefixExpression(tokens, output);
	}

	return parseBinaryOperation(tokens, *operator.precedent, output);
}

PRIVATE KleinResult binaryOperationOf(String tokenValue, BinaryOperation* output) {
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
	if (strcmp(tokenValue, "and") == 0) {
		RETURN_OK(output, BINARY_OPERATION_AND);
	}
	if (strcmp(tokenValue, "or") == 0) {
		RETURN_OK(output, BINARY_OPERATION_OR);
	}
	if (strcmp(tokenValue, "==") == 0) {
		RETURN_OK(output, BINARY_OPERATION_EQUAL);
	}
	if (strcmp(tokenValue, "=") == 0) {
		RETURN_OK(output, BINARY_OPERATION_ASSIGN);
	}
	return (KleinResult) {
		.type = KLEIN_ERROR_INTERNAL,
	};
}

PRIVATE KleinResult parseFieldAccess(TokenList* tokens, Expression* output) {
	TRY_LET(Expression left, parseLiteral(tokens, &left));

	while (nextTokenIs(tokens, TOKEN_TYPE_DOT)) {
		UNWRAP_LET(String next, popToken(tokens, TOKEN_TYPE_DOT, &next));
		Expression right;
		TRY(parseIdentifierLiteral(tokens, &right));

		BinaryExpression* binary = malloc(sizeof(BinaryExpression));
		*binary = (BinaryExpression) {
			.left = left,
			.right = right,
			.operation = BINARY_OPERATION_DOT,
		};
		left = (Expression) {
			.type = EXPRESSION_BINARY,
			.data = (ExpressionData) {
				.binary = binary,
			},
		};
	}

	RETURN_OK(output, left);
}

PRIVATE KleinResult parseBinaryOperation(TokenList* tokens, BinaryOperator operator, Expression * output) {
	TRY_LET(Expression left, parsePrecedentBinaryOperation(tokens, operator, & left));

	while (nextTokenIsOneOf(tokens, operator.tokenTypes, operator.tokenTypeCount)) {
		UNWRAP_LET(String next, popAnyToken(tokens, &next));

		TRY_LET(BinaryOperation operation, binaryOperationOf(next, &operation));

		Expression right;
		TRY(parsePrecedentBinaryOperation(tokens, operator, & right));

		BinaryExpression* binary = malloc(sizeof(BinaryExpression));
		*binary = (BinaryExpression) {
			.left = left,
			.right = right,
			.operation = operation,
		};
		left = (Expression) {
			.type = EXPRESSION_BINARY,
			.data = (ExpressionData) {
				.binary = binary,
			},
		};
	}

	RETURN_OK(output, left);
}

PRIVATE KleinResult parseExpression(TokenList* tokens, Expression* output) {
	return parseBinaryOperation(tokens, ASSIGNMENT, output);
}

PRIVATE KleinResult parsePostfixExpression(TokenList* tokens, Expression* output) {
	TRY_LET(Expression expression, parseFieldAccess(tokens, &expression));

	while (nextTokenIsOneOf(tokens, (TokenType[]) {TOKEN_TYPE_LEFT_PARENTHESIS, TOKEN_TYPE_LEFT_BRACKET}, 2)) {

		// Index
		if (nextTokenIs(tokens, TOKEN_TYPE_LEFT_BRACKET)) {
			UNWRAP_LET(String next, popToken(tokens, TOKEN_TYPE_LEFT_BRACKET, &next));
			TRY_LET(Expression index, parseExpression(tokens, &index));
			TRY(popToken(tokens, TOKEN_TYPE_RIGHT_BRACKET, &next));

			UnaryExpression* unary = malloc(sizeof(UnaryExpression));
			*unary = (UnaryExpression) {
				.expression = expression,
				.operation = (UnaryOperation) {
					.type = UNARY_OPERATION_INDEX,
					.data = (UnaryOperationData) {
						.index = index,
					},
				},
			};
			expression = (Expression) {
				.type = EXPRESSION_UNARY,
				.data = (ExpressionData) {
					.unary = unary,
				},
			};
			continue;
		}

		// Function call
		if (nextTokenIs(tokens, TOKEN_TYPE_LEFT_PARENTHESIS)) {
			UNWRAP_LET(String next, popToken(tokens, TOKEN_TYPE_LEFT_PARENTHESIS, &next));

			// Parse arguments
			ExpressionList arguments = emptyExpressionList();
			while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS)) {
				TRY_LET(Expression argument, parseExpression(tokens, &argument));
				appendToExpressionList(&arguments, argument);

				// Comma
				if (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS)) {
					TRY(popToken(tokens, TOKEN_TYPE_COMMA, &next));
				}
			}

			TRY(popToken(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS, &next));

			UnaryExpression* unary = malloc(sizeof(UnaryExpression));
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
			continue;
		}
	}

	RETURN_OK(output, expression);
}

/**
 * Parses a `prefix expression`.
 *
 * Syntax: `"not" <expression>`
 *
 * # Parameters
 *
 * - `tokens` - The token list to parse from
 * - `output` - Where to place the parsed output
 *
 * # Returns
 *
 * A result, indicating success or failure. If failed, contains the error message.
 *
 * # Errors
 *
 * If an unexpected token was encountered (including the token stream running out of tokens
 * unexpectedly), an error is returned. If memory fails to allocate, an error is returned.
 */
PRIVATE KleinResult parsePrefixExpression(TokenList* tokens, Expression* output) {
	if (nextTokenIs(tokens, TOKEN_TYPE_KEYWORD_NOT)) {
		UNWRAP_LET(String next, popToken(tokens, TOKEN_TYPE_KEYWORD_NOT, &next));
		TRY_LET(Expression inner, parsePrefixExpression(tokens, &inner));

		UnaryExpression* unary = malloc(sizeof(UnaryExpression));
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

	return parsePostfixExpression(tokens, output);
}

/**
 * Parses a `declaration statement`.
 *
 * Syntax: `"let" <identifier> (":" <type>)? "=" <expression> ";"`
 *
 * # Parameters
 *
 * - `tokens` - The token list to parse from
 * - `output` - Where to place the parsed output
 *
 * # Returns
 *
 * A result, indicating success or failure. If failed, contains the error message.
 *
 * # Errors
 *
 * If an unexpected token was encountered (including the token stream running out of tokens
 * unexpectedly), an error is returned. If memory fails to allocate, an error is returned.
 */
PRIVATE KleinResult parseDeclaration(TokenList* tokens, Statement* output) {
	// let
	TRY_LET(String next, popToken(tokens, TOKEN_TYPE_KEYWORD_LET, &next));

	// Name
	TRY_LET(String name, popToken(tokens, TOKEN_TYPE_IDENTIFIER, &name));

	// :type
	Type* type = NULL;
	TRY_LET(TokenType nextTokenType, peekTokenType(tokens, &nextTokenType));
	if (nextTokenType == TOKEN_TYPE_COLON) {
		UNWRAP(popToken(tokens, TOKEN_TYPE_COLON, &next));
		type = malloc(sizeof(Type));
		TRY(parseType(tokens, type));
	}

	// = value
	TRY(popToken(tokens, TOKEN_TYPE_EQUALS, &next));
	TRY_LET(Expression value, parseExpression(tokens, &value));

	// Semicolon
	TRY(popToken(tokens, TOKEN_TYPE_SEMICOLON, &next));

	// Allocate & return
	Statement statement = (Statement) {
		.type = STATEMENT_DECLARATION,
		.data = (StatementData) {
			.declaration = (Declaration) {
				.name = name,
				.type = type,
				.value = value,
			},
		},
	};
	RETURN_OK(output, statement);
}

/**
 * Parses a `return statement`.
 *
 * Syntax: `"return" <expression>? ";"`
 *
 * # Parameters
 *
 * - `tokens` - The token list to parse from
 * - `output` - Where to place the parsed output
 *
 * # Returns
 *
 * A result, indicating success or failure. If failed, contains the error message.
 *
 * # Errors
 *
 * If an unexpected token was encountered (including the token stream running out of tokens
 * unexpectedly), an error is returned. If memory fails to allocate, an error is returned.
 */
PRIVATE KleinResult parseReturnStatement(TokenList* tokens, Statement* output) {
	TRY_LET(String next, popToken(tokens, TOKEN_TYPE_KEYWORD_RETURN, &next));
	Expression expression;
	TRY(parseExpression(tokens, &expression));
	Statement statement = (Statement) {
		.type = STATEMENT_RETURN,
		.data = (StatementData) {
			.returnExpression = expression,
		},
	};
	TRY(popToken(tokens, TOKEN_TYPE_SEMICOLON, &next));
	RETURN_OK(output, statement);
}

/**
 * Parses an `expression statement`.
 *
 * Syntax: `<expression> ";"`
 *
 * # Parameters
 *
 * - `tokens` - The token list to parse from
 * - `output` - Where to place the parsed output
 *
 * # Returns
 *
 * A result, indicating success or failure. If failed, contains the error message.
 *
 * # Errors
 *
 * If an unexpected token was encountered (including the token stream running out of tokens
 * unexpectedly), an error is returned. If memory fails to allocate, an error is returned.
 */
PRIVATE KleinResult parseExpressionStatement(TokenList* tokens, Statement* output) {
	TRY_LET(Expression expression, parseExpression(tokens, &expression));
	Statement statement = (Statement) {
		.type = STATEMENT_EXPRESSION,
		.data = (StatementData) {
			.expression = expression,
		},
	};
	TRY_LET(String next, popToken(tokens, TOKEN_TYPE_SEMICOLON, &next));
	RETURN_OK(output, statement);
}

/**
 * Parses a `statement`.
 *
 * Syntax:
 *
 * ```
 * <statement> ::= <declaration> |
 *                 <return-statement> |
 *                 (<expression> ";")
 * ```
 *
 * # Parameters
 *
 * - `tokens` - The token list to parse from
 * - `output` - Where to place the parsed output
 *
 * # Returns
 *
 * A result, indicating success or failure. If failed, contains the error message.
 *
 * # Errors
 *
 * If an unexpected token was encountered (including the token stream running out of tokens
 * unexpectedly), an error is returned. If memory fails to allocate, an error is returned.
 */
PRIVATE KleinResult parseStatement(TokenList* tokens, Statement* output) {
	TRY_LET(TokenType nextTokenType, peekTokenType(tokens, &nextTokenType));

	switch (nextTokenType) {
		case TOKEN_TYPE_KEYWORD_LET: {
			return parseDeclaration(tokens, output);
		}
		case TOKEN_TYPE_KEYWORD_RETURN: {
			return parseReturnStatement(tokens, output);
		}
		default: {
			return parseExpressionStatement(tokens, output);
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
PRIVATE KleinResult parseTokens(TokenList* tokens, Program* output) {
	StatementList statements = emptyStatementList();
	while (!isTokenListEmpty(*tokens)) {
		Statement statement;
		TRY(parseStatement(tokens, &statement));
		appendToStatementList(&statements, statement);
	}

	RETURN_OK(output, (Program) {.statements = statements});
}

KleinResult parseKlein(String code, Program* output) {
	TokenList tokens;
	TRY(tokenizeKlein(code, &tokens));
	TRY_LET(Program program, parseTokens(&tokens, &program));
	RETURN_OK(output, program);
}

KleinResult parseKleinExpression(String code, Expression* output) {
	TokenList tokens;
	TRY(tokenizeKlein(code, &tokens));
	TRY_LET(Expression expression, parseExpression(&tokens, &expression));
	RETURN_OK(output, expression);
}

IMPLEMENT_KLEIN_LIST(Declaration)
IMPLEMENT_KLEIN_LIST(Statement)
IMPLEMENT_KLEIN_LIST(Expression)
IMPLEMENT_KLEIN_LIST(Parameter)
IMPLEMENT_KLEIN_LIST(Field)
IMPLEMENT_KLEIN_LIST(Internal)
IMPLEMENT_KLEIN_LIST(ValueField)
IMPLEMENT_KLEIN_LIST(Value)
IMPLEMENT_KLEIN_LIST(IfExpression)
