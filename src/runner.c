#include "../include/runner.h"
#include "../include/builtin.h"
#include "../include/context.h"
#include "../include/parser.h"
#include "../include/sugar.h"
#include <stdlib.h>
#include <string.h>

PRIVATE Result evaluateStatement(Context* context, Statement* statement);

PRIVATE Result evaluateBlock(Context* context, Block* block, Expression* output) {
	FOR_EACHP(Statement statement, block->statements) {
		TRY(evaluateStatement(context, &statement));
	}
	END;

	Expression expression = (Expression) {
		.type = EXPRESSION_BLOCK,
		.data = (ExpressionData) {
			.block = block,
		},
	};

	RETURN_OK(Output, expression);
}

PRIVATE Result evaluateExpression(Context* context, Expression* expression, Expression* output) {
	DEBUG("Evaluating expression\n");

	switch (expression->type) {

		// Literals
		case EXPRESSION_NUMBER:
		case EXPRESSION_BOOLEAN:
		case EXPRESSION_IDENTIFIER:
		case EXPRESSION_TYPE:
		case EXPRESSION_OBJECT:
		case EXPRESSION_FUNCTION: {
			RETURN_OK(output, *expression);
		}

		// Block
		case EXPRESSION_BLOCK: {
			return evaluateBlock(context, expression->data.block, output);
		}

		case EXPRESSION_BUILTIN_FUNCTION: {
			return ERROR_INTERNAL;
		}

		// Unary expressions
		case EXPRESSION_UNARY: {
			UnaryExpression* unary = expression->data.unary;
			TRY(evaluateExpression(context, &unary->expression, &unary->expression));
			switch (unary->operation.type) {

				// Function call
				case UNARY_OPERATION_FUNCTION_CALL: {
					DEBUG("Evaluating function call\n");
					switch (unary->expression.type) {

						// Calling a regular function
						case EXPRESSION_FUNCTION: {
							return evaluateBlock(context, &unary->expression.data.function.body, output);
						}

						// Calling an identifier such as `name()`
						case EXPRESSION_IDENTIFIER: {
							DEBUG("Evaluating identifier function call\n");
							String identifier = unary->expression.data.identifier;

							// Calling `builtin()`
							if (strcmp(identifier, "builtin") == 0) {
								DEBUG("Evaluating call to builtin()\n");

								String* builtinName;
								TRY(getString(unary->operation.data.functionCall.data[0], &builtinName));

								TRY_LET(BuiltinFunction, function, getBuiltin, *builtinName);

								Expression functionExpression = (Expression) {
									.type = EXPRESSION_BUILTIN_FUNCTION,
									.data = (ExpressionData) {
										.builtinFunction = {
											.function = function,
											.thisObject = NULL,
										},
									},
								};

								RETURN_OK(output, functionExpression);
							}

							// Not `builtin()`
							TRY_LET(Expression*, value, getVariable, *context->scope, identifier);

							// Calling a  function
							if (value->type == EXPRESSION_FUNCTION) {
								return evaluateBlock(context, &value->data.function.body, output);
							}

							// Calling a builtin function like `print()`
							if (value->type == EXPRESSION_BUILTIN_FUNCTION) {
								if (value->data.builtinFunction.thisObject != NULL) {
									TRY(prependToExpressionList(&unary->operation.data.functionCall, *unary->expression.data.builtinFunction.thisObject));
								}
								return value->data.builtinFunction.function(context, &unary->operation.data.functionCall, output);
							}

							// Calling something else (like an identifier that resolves to a number)
							return ERROR_CALL_NON_FUNCTION;
						}

						// Calling something else (like a number literal like `3()`)
						default: {
							return ERROR_CALL_NON_FUNCTION;
						}
					}
				}

				// Negation
				case UNARY_OPERATION_NOT: {
					if (unary->expression.type != EXPRESSION_BOOLEAN) {
						return ERROR_INVALID_OPERAND;
					}

					bool result = !expression->data.boolean;
					Expression booleanExpression = (Expression) {
						.type = EXPRESSION_BOOLEAN,
						.data = (ExpressionData) {
							.boolean = result,
						},
					};
					RETURN_OK(output, booleanExpression);
				}
			}
		}

		// Binary Expression
		case EXPRESSION_BINARY: {
			Expression left;
			TRY(evaluateExpression(context, expression->data.binary.left, &left));
			Expression right;
			TRY(evaluateExpression(context, expression->data.binary.right, &right));

			switch (expression->data.binary.operation) {
				// Field access
				case BINARY_OPERATION_DOT: {
					if (right.type != EXPRESSION_IDENTIFIER) {
						return ERROR_INVALID_OPERAND;
					}

					if (left.type != EXPRESSION_OBJECT) {
						return ERROR_INVALID_OPERAND;
					}

					Expression* fieldValue;
					TRY(getObjectField(*left.data.object, right.data.identifier, &fieldValue));

					if (fieldValue->type == EXPRESSION_FUNCTION) {
						fieldValue->data.function.thisObject = &left;
					} else if (fieldValue->type == EXPRESSION_BUILTIN_FUNCTION) {
						fieldValue->data.builtinFunction.thisObject = &left;
					}

					RETURN_OK(output, *fieldValue);
				}

				default: {
					return ERROR_INTERNAL;
				}
			}
		}
	}

	return ERROR_INTERNAL;
}

PRIVATE Result evaluateStatement(Context* context, Statement* statement) {
	DEBUG("Evaluating statement\n");
	switch (statement->type) {
		case STATEMENT_EXPRESSION: {
			Expression output;
			TRY(evaluateExpression(context, &statement->data.expression, &output));
			return OK;
		}

		case STATEMENT_DECLARATION: {
			TRY(evaluateExpression(context, &statement->data.declaration.value, &statement->data.declaration.value));
			TRY(declareNewVariable(context->scope, statement->data.declaration));
			return OK;
		}
	}
}

Result run(Context* context, Program program) {
	FOR_EACH(Statement statement, program.statements) {
		TRY(evaluateStatement(context, &statement));
	}
	END;

	return OK;
}
