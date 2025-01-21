#include "../include/parser.h"
#include "../bindings/c/klein.h"
#include "../include/context.h"
#include "../include/lexer.h"
#include "../include/list.h"
#include "../include/result.h"
#include "../include/util.h"

#include <stdlib.h>
#include <string.h>

PRIVATE Result parseLiteral(TokenList* tokens, Expression* output);
PRIVATE Result parseStatement(TokenList* tokens, Statement* output);
PRIVATE Result parseType(TokenList* tokens, Type* output);
PRIVATE Result parseExpression(TokenList* tokens, Expression* output);
PRIVATE Result parseBinaryOperation(TokenList* tokens, BinaryOperator operator, Expression * output);
PRIVATE Result parsePrefixExpression(TokenList* tokens, Expression* output);

bool hasInternal(Value value, InternalKey key) {
	void* output;
	return isOk(getValueInternal(value, key, &output));
}

Result getValueInternal(Value value, InternalKey key, void** output) {
	FOR_EACH(Internal internal, value.internals) {
		if (internal.key == key) {
			RETURN_OK(output, internal.value);
		}
	}
	END;

	RETURN_ERROR("Attempted to get an internal called \"%u\" on a value, but no internal with that name exists.", key);
}

Result getValueField(Value value, String name, Value** output) {
	FOR_EACH_REFP(ValueField * field, value.fields) {
		if (strcmp(field->name, name) == 0) {
			RETURN_OK(output, &field->value);
		}
	}
	END;

	RETURN_ERROR("Attempted to get a field called \"%s\" on an object, but no field with that name exists.", name);
}

PRIVATE Result popToken(TokenList* tokens, TokenType type, String* output) {

	// Empty token stream - error
	if (tokens->size == 0) {
		RETURN_ERROR("Expected %s but found end of input.", tokenTypeName(type));
	}

	// Check token
	Token token = tokens->data[0];
	if (token.type != type) {
		RETURN_ERROR("Expected %s but found %s.", tokenTypeName(type), tokenTypeName(token.type));
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
		RETURN_ERROR("Expected token but found end of input.");
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
		RETURN_ERROR("Expected token but found end of input.");
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

PRIVATE Result parseTypeLiteral(TokenList* tokens, TypeLiteral* output) {
	TokenType nextTokenType;
	TRY(peekTokenType(tokens, &nextTokenType), "peeking the next token type");
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
			TRY(emptyParameterList(&parameterTypes), "creating the parameter list for a function expression");
			TRY(popToken(tokens, TOKEN_TYPE_LEFT_PARENTHESIS, &next), "attempting to consume the opening parenthesis for a function's parameter list");
			while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS)) {
				Type type;
				TRY(parseType(tokens, &type), "parsing a function parameter's type");
				TRY(appendToParameterList(&parameterTypes, (Parameter) {.name = "", .type = type}), "appending to a function's parameter list");
			}
			TRY(popToken(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS, &next), "attempting to consume the closing parenthesis for a function's parameter list");

			// Return type
			TRY(popToken(tokens, TOKEN_TYPE_COLON, &next), "attempting to consume the colon before a function's return type");
			Type returnType;
			TRY(parseType(tokens, &returnType), "parsing a function's return type");

			// Create function
			Function* function = malloc(sizeof(Function));
			ASSERT_NONNULL(function);
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
			RETURN_ERROR("Unexpected token: %s", tokenTypeName(nextTokenType));
		}
	}
}

PRIVATE Result parseType(TokenList* tokens, Type* output) {
	TypeLiteral literal;
	TRY(parseTypeLiteral(tokens, &literal), "parsing a type literal");
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
PRIVATE Result parseBlock(TokenList* tokens, Block* output) {
	TRY(enterNewScope(), "entering a new scope");
	TRY_LET(String next, popToken(tokens, TOKEN_TYPE_LEFT_BRACE, &next), "consuming the opening brace for a block");

	// Parse statements
	StatementList* statements;
	TRY(emptyHeapStatementList(&statements), "creating a block's statement list");
	while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_BRACE)) {
		Statement statement;
		TRY(parseStatement(tokens, &statement), "parsing a statement");
		TRY(appendToStatementList(statements, statement), "appending a statement to a block's statement list");
	}

	UNWRAP(popToken(tokens, TOKEN_TYPE_RIGHT_BRACE, &next));

	Block block = (Block) {
		.statements = statements,
		.innerScope = CONTEXT->scope,
	};

	TRY(exitScope(), "exiting a block's scope");

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
PRIVATE Result parseObjectLiteral(TokenList* tokens, Expression* output) {
	TRY_LET(String next, popToken(tokens, TOKEN_TYPE_LEFT_BRACE, &next), "attempting to consume the opening brace on an object literal");

	// Fields
	FieldList fields;
	TRY(emptyFieldList(&fields), "creating an object's field list");
	while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_BRACE)) {
		TRY_LET(String name, popToken(tokens, TOKEN_TYPE_IDENTIFIER, &name), "parsing the name of an object field");
		TRY(popToken(tokens, TOKEN_TYPE_EQUALS, &next), "consuming the equal sign after an object field name");
		TRY_LET(Expression value, parseExpression(tokens, &value), "parsing the value of an object field");
		TRY(appendToFieldList(&fields, (Field) {.name = name, .value = value}), "appending to an object's field list");
		if (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_BRACE)) {
			TRY(popToken(tokens, TOKEN_TYPE_COMMA, &next), "attempting to consume a comma in between object fields");
		}
	}

	TRY(popToken(tokens, TOKEN_TYPE_RIGHT_BRACE, &next), "attempting to consume the closing brace on an object literal");

	Object* object = malloc(sizeof(Object));
	ASSERT_NONNULL(object);
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
PRIVATE Result parseStringLiteral(TokenList* tokens, Expression* output) {
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
PRIVATE Result parseIdentifierLiteral(TokenList* tokens, Expression* output) {
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
PRIVATE Result parseListLiteral(TokenList* tokens, Expression* output) {
	UNWRAP_LET(String next, popToken(tokens, TOKEN_TYPE_LEFT_BRACKET, &next));

	ExpressionList* elements;
	TRY(emptyHeapExpressionList(&elements), "creating the element list for a list literal");
	while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_BRACKET)) {
		TRY_LET(Expression element, parseExpression(tokens, &element), "parsing an element of a list literal");
		TRY(appendToExpressionList(elements, element), "appending to a list's expression list");
		if (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_BRACKET)) {
			TRY(popToken(tokens, TOKEN_TYPE_COMMA, &next), "attempting to consume a comma between list elements");
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
PRIVATE Result parseForLoop(TokenList* tokens, Expression* output) {
	UNWRAP_LET(String next, popToken(tokens, TOKEN_TYPE_KEYWORD_FOR, &next));
	TRY_LET(String binding, popToken(tokens, TOKEN_TYPE_IDENTIFIER, &binding), "parsing the binding in a for loop");
	TRY(popToken(tokens, TOKEN_TYPE_KEYWORD_IN, &next), "consuming the keyword in in a for loop");
	TRY_LET(Expression list, parseExpression(tokens, &list), "parsing the list to iterate on in a for loop");
	TRY_LET(Block body, parseBlock(tokens, &body), "parsing the body of a for loop");

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
PRIVATE Result parseWhileLoop(TokenList* tokens, Expression* output) {
	UNWRAP_LET(String next, popToken(tokens, TOKEN_TYPE_KEYWORD_WHILE, &next));
	TRY_LET(Expression condition, parseExpression(tokens, &condition), "parsing the condition for a while loop");
	TRY_LET(Block body, parseBlock(tokens, &body), "parsing the body of a while loop");

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
PRIVATE Result parseIfExpression(TokenList* tokens, Expression* output) {
	UNWRAP_LET(String next, popToken(tokens, TOKEN_TYPE_KEYWORD_IF, &next));
	TRY_LET(Expression condition, parseExpression(tokens, &condition), "parsing the condition for an if expression");
	TRY_LET(Block body, parseBlock(tokens, &body), "parsing the body of an if expression");

	IfExpressionList* elseIfs;
	TRY(emptyHeapIfExpressionList(&elseIfs), "creating an if-expression's empty else-if list");

	IfExpression ifExpression = (IfExpression) {
		.condition = condition,
		.body = body,
	};
	TRY(appendToIfExpressionList(elseIfs, ifExpression), "appending to an if-expression list");

	while (nextTokenIs(tokens, TOKEN_TYPE_KEYWORD_ELSE)) {
		UNWRAP(popToken(tokens, TOKEN_TYPE_KEYWORD_ELSE, &next));

		// Else-if block
		if (nextTokenIs(tokens, TOKEN_TYPE_KEYWORD_IF)) {
			UNWRAP(popToken(tokens, TOKEN_TYPE_KEYWORD_IF, &next));
			TRY_LET(Expression elseIfCondition, parseExpression(tokens, &elseIfCondition), "parsing an else-if branch's condition");
			TRY_LET(Block elseIfBody, parseBlock(tokens, &elseIfBody), "parsing the body of an else-if branch");
			IfExpression elseIfExpression = (IfExpression) {
				.condition = elseIfCondition,
				.body = elseIfBody,
			};
			TRY(appendToIfExpressionList(elseIfs, elseIfExpression), "appending to an if-expression list");
		}

		// Else block
		else {
			TRY_LET(Block elseIfBody, parseBlock(tokens, &elseIfBody), "parsing the body of an else-if branch");
			IfExpression elseIfExpression = (IfExpression) {
				.condition = (Expression) {
					.type = EXPRESSION_BOOLEAN,
					.data = (ExpressionData) {
						.boolean = true,
					},
				},
				.body = elseIfBody,
			};
			TRY(appendToIfExpressionList(elseIfs, elseIfExpression), "appending to an if-expression list");
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
PRIVATE Result parseNumberLiteral(TokenList* tokens, Expression* output) {
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
PRIVATE Result parseDoBlock(TokenList* tokens, Expression* output) {
	UNWRAP_LET(String next, popToken(tokens, TOKEN_TYPE_KEYWORD_DO, &next));

	Block block;
	TRY(parseBlock(tokens, &block), "parsing a block expression");
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
PRIVATE Result parseFunctionLiteral(TokenList* tokens, Expression* output) {
	UNWRAP_LET(String next, popToken(tokens, TOKEN_TYPE_KEYWORD_FUNCTION, &next));

	// Parameters
	ParameterList parameters;
	TRY(emptyParameterList(&parameters), "creating a function literal's parameter list");

	TRY(popToken(tokens, TOKEN_TYPE_LEFT_PARENTHESIS, &next), "attempting to consume the opening parenthesis for a function literal's parameter list");

	while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS)) {
		TRY_LET(String name, popToken(tokens, TOKEN_TYPE_IDENTIFIER, &name), "parsing a function literal's parameter name");
		TRY(popToken(tokens, TOKEN_TYPE_COLON, &next), "consuming the colon after a function literal's parameter name");
		TRY_LET(Type type, parseType(tokens, &type), "parsing a function parameter's type");

		Parameter parameter = (Parameter) {
			.type = type,
			.name = name,
		};

		TRY(appendToParameterList(&parameters, parameter), "appending to a function's parameter list");

		if (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS)) {
			TRY(popToken(tokens, TOKEN_TYPE_COMMA, &next), "attempting to consume the comma after a function's parameter");
		}
	}
	UNWRAP(popToken(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS, &next));

	// Return type
	TRY(popToken(tokens, TOKEN_TYPE_COLON, &next), "attempting to consume the colon before a function's return type");
	TRY_LET(Type returnType, parseType(tokens, &returnType), "parsing a function literal's return type");

	// Body
	TRY_LET(Block body, parseBlock(tokens, &body), "parsing the body of a function literal");

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
PRIVATE Result parseParenthesizedExpression(TokenList* tokens, Expression* output) {
	TRY_LET(String next, popToken(tokens, TOKEN_TYPE_LEFT_PARENTHESIS, &next), "parsing the left parenthesis in a parenthesized expression");
	TRY(parseExpression(tokens, output), "parsing the inside of a parenthesized expression");
	TRY(popToken(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS, &next), "parsing the closing parenthesis in a parenthesized expression");
	return OK;
}

PRIVATE Result parseLiteral(TokenList* tokens, Expression* output) {
	TRY_LET(TokenType nextTokenType, peekTokenType(tokens, &nextTokenType), "peeking the next token type");

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
			RETURN_ERROR("Expected literal but found %s", tokenTypeName(nextTokenType));
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

PRIVATE Result parsePrecedentBinaryOperation(TokenList* tokens, BinaryOperator operator, Expression * output) {
	if (operator.precedent == NULL) {
		return parsePrefixExpression(tokens, output);
	}

	return parseBinaryOperation(tokens, *operator.precedent, output);
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
	RETURN_ERROR("Unknown binary operation: %s", tokenValue);
}

PRIVATE Result parseFieldAccess(TokenList* tokens, Expression* output) {
	TRY_LET(Expression left, parseLiteral(tokens, &left), "");

	while (nextTokenIs(tokens, TOKEN_TYPE_DOT)) {
		UNWRAP_LET(String next, popToken(tokens, TOKEN_TYPE_DOT, &next));
		Expression right;
		TRY(parseIdentifierLiteral(tokens, &right), "parsing the right-hand sde of a field access expression");

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

PRIVATE Result parseBinaryOperation(TokenList* tokens, BinaryOperator operator, Expression * output) {
	TRY_LET(Expression left, parsePrecedentBinaryOperation(tokens, operator, & left), "");

	while (nextTokenIsOneOf(tokens, operator.tokenTypes, operator.tokenTypeCount)) {
		UNWRAP_LET(String next, popAnyToken(tokens, &next));

		TRY_LET(BinaryOperation operation, binaryOperationOf(next, &operation), "getting a binary operation");

		Expression right;
		TRY(parsePrecedentBinaryOperation(tokens, operator, & right), "parsing the right-hand side of a binary operation");

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

PRIVATE Result parseExpression(TokenList* tokens, Expression* output) {
	return parseBinaryOperation(tokens, ASSIGNMENT, output);
}

PRIVATE Result parsePostfixExpression(TokenList* tokens, Expression* output) {
	TRY_LET(Expression expression, parseFieldAccess(tokens, &expression), "");

	while (nextTokenIsOneOf(tokens, (TokenType[]) {TOKEN_TYPE_LEFT_PARENTHESIS, TOKEN_TYPE_LEFT_BRACKET}, 2)) {

		// Index
		if (nextTokenIs(tokens, TOKEN_TYPE_LEFT_BRACKET)) {
			UNWRAP_LET(String next, popToken(tokens, TOKEN_TYPE_LEFT_BRACKET, &next));
			TRY_LET(Expression index, parseExpression(tokens, &index), "parsing an index expression");
			TRY(popToken(tokens, TOKEN_TYPE_RIGHT_BRACKET, &next), "parsing a closing bracket on an index expression");

			UnaryExpression* unary = malloc(sizeof(UnaryExpression));
			ASSERT_NONNULL(unary);
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
			ExpressionList arguments;
			TRY(emptyExpressionList(&arguments), "creating a function call's argument list");
			while (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS)) {
				TRY_LET(Expression argument, parseExpression(tokens, &argument), "parsing a function call's argument");
				TRY(appendToExpressionList(&arguments, argument), "appending to a function call's argument list");

				// Comma
				if (!nextTokenIs(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS)) {
					TRY(popToken(tokens, TOKEN_TYPE_COMMA, &next), "attempting to consume a comma after a function call's argument");
				}
			}

			TRY(popToken(tokens, TOKEN_TYPE_RIGHT_PARENTHESIS, &next), "parsing the closing parentheses on a function call");

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
PRIVATE Result parsePrefixExpression(TokenList* tokens, Expression* output) {
	if (nextTokenIs(tokens, TOKEN_TYPE_KEYWORD_NOT)) {
		UNWRAP_LET(String next, popToken(tokens, TOKEN_TYPE_KEYWORD_NOT, &next));
		TRY_LET(Expression inner, parsePrefixExpression(tokens, &inner), "parsing the inside of a unary not expression");

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
PRIVATE Result parseDeclaration(TokenList* tokens, Statement* output) {
	// let
	TRY_LET(String next, popToken(tokens, TOKEN_TYPE_KEYWORD_LET, &next), "parsing the keyword let in a declaration");

	// Name
	TRY_LET(String name, popToken(tokens, TOKEN_TYPE_IDENTIFIER, &name), "parsing the name of a declaration");

	// :type
	Type* type = NULL;
	TRY_LET(TokenType nextTokenType, peekTokenType(tokens, &nextTokenType), "peeking the next token after the name of a variable declaration");
	if (nextTokenType == TOKEN_TYPE_COLON) {
		UNWRAP(popToken(tokens, TOKEN_TYPE_COLON, &next));
		type = malloc(sizeof(Type));
		TRY(parseType(tokens, type), "parsing the type annotation on a variable declaration");
	}

	// = value
	TRY(popToken(tokens, TOKEN_TYPE_EQUALS, &next), "parsing the equal sign in a variable declaration");
	TRY_LET(Expression value, parseExpression(tokens, &value), "parsing the value of a variable declaration");

	// Semicolon
	TRY(popToken(tokens, TOKEN_TYPE_SEMICOLON, &next), "parsing a semicolon after a declaration");

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
PRIVATE Result parseReturnStatement(TokenList* tokens, Statement* output) {
	TRY_LET(String next, popToken(tokens, TOKEN_TYPE_KEYWORD_RETURN, &next), "parsing the keyword return in a return statement");
	Expression expression;
	TRY(parseExpression(tokens, &expression), "parsing the expression in a return statement");
	Statement statement = (Statement) {
		.type = STATEMENT_RETURN,
		.data = (StatementData) {
			.returnExpression = expression,
		},
	};
	TRY(popToken(tokens, TOKEN_TYPE_SEMICOLON, &next), "consuming the semicolon after a return statement");
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
PRIVATE Result parseExpressionStatement(TokenList* tokens, Statement* output) {
	TRY_LET(Expression expression, parseExpression(tokens, &expression), "parsing a statement's expression");
	Statement statement = (Statement) {
		.type = STATEMENT_EXPRESSION,
		.data = (StatementData) {
			.expression = expression,
		},
	};
	TRY_LET(String next, popToken(tokens, TOKEN_TYPE_SEMICOLON, &next), "consuming the semicolon after an expression statement");
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
PRIVATE Result parseStatement(TokenList* tokens, Statement* output) {
	TRY_LET(TokenType nextTokenType, peekTokenType(tokens, &nextTokenType), "peeking the next token type");

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
PRIVATE Result parseTokens(TokenList* tokens, Program* output) {
	StatementList statements;
	TRY(emptyStatementList(&statements), "creating the programs statement list");
	while (!isTokenListEmpty(*tokens)) {
		Statement statement;
		TRY(parseStatement(tokens, &statement), "parsing a top-level statement in the program");
		TRY(appendToStatementList(&statements, statement), "appending to the program's statement list");
	}

	RETURN_OK(output, (Program) {.statements = statements});
}

Result parseKlein(String code, Program* output) {
	TokenList tokens;
	TRY(tokenizeKlein(code, &tokens), "tokenizing the program's source code");
	TRY_LET(Program program, parseTokens(&tokens, &program), "parsing the program");
	RETURN_OK(output, program);
}

Result parseKleinExpression(String code, Expression* output) {
	TokenList tokens;
	TRY(tokenizeKlein(code, &tokens), "tokenizing the program's source code");
	TRY_LET(Expression expression, parseExpression(&tokens, &expression), "parsing an expression");
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
