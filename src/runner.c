#include "../include/runner.h"
#include "../include/builtin.h"
#include "../include/context.h"
#include "../include/parser.h"
#include "../include/sugar.h"
#include <stdlib.h>
#include <string.h>

static bool isReturning = false;
static Expression returnValue;

PRIVATE Result evaluateStatement(Statement* statement);

PRIVATE Result evaluateBlock(Block* block, Expression* output) {
	DEBUG_START("Evaluating block");

	Scope* previousScope = CONTEXT->scope;
	CONTEXT->scope = block->innerScope;

	FOR_EACHP(Statement statement, block->statements) {
		TRY(evaluateStatement(&statement));
		if (isReturning) {
			isReturning = false;
			DEBUG_END;
			RETURN_OK(output, returnValue);
		}
	}
	END;

	CONTEXT->scope = previousScope;

	Expression expression = (Expression) {
		.type = EXPRESSION_BLOCK,
		.data = (ExpressionData) {
			.block = block,
		},
	};

	DEBUG_END;
	RETURN_OK(output, expression);
}

PRIVATE Result evaluateExpression(Expression* expression, Expression* output) {
	DEBUG_START("Evaluating expression");

	switch (expression->type) {

		// Literals
		case EXPRESSION_IDENTIFIER:
		case EXPRESSION_TYPE:
		case EXPRESSION_OBJECT:
		case EXPRESSION_FUNCTION: {
			DEBUG_START("Evaluating literal");
			DEBUG_END;
			DEBUG_END;
			RETURN_OK(output, *expression);
		}

		// Block
		case EXPRESSION_BLOCK: {
			TRY(evaluateBlock(expression->data.block, output));
			DEBUG_END;
			return OK;
		}

		case EXPRESSION_BUILTIN_FUNCTION: {
			DEBUG_END;
			RETURN_OK(output, *expression);
		}

		// For loop
		case EXPRESSION_FOR_LOOP: {
			DEBUG_START("Evaluating for loop");

			DEBUG_START("Evaluating for loop iterable");
			TRY(evaluateExpression(&expression->data.forLoop->list, &expression->data.forLoop->list));
			if (expression->data.forLoop->list.type == EXPRESSION_IDENTIFIER) {
				Expression* value;
				TRY(getVariable(*CONTEXT->scope, expression->data.forLoop->list.data.identifier, &value));
				expression->data.forLoop->list = *value;
			}
			TRY_LET(ExpressionList*, list, getList, expression->data.forLoop->list);
			DEBUG_END;

			DEBUG_START("Evaluating for loop body");
			FOR_EACHP(Expression value, list) {
				Declaration declaration = (Declaration) {
					.name = expression->data.forLoop->binding,
					.value = value,
				};
				setVariable(expression->data.forLoop->body.innerScope, declaration);
				TRY_LET(Expression, blockValue, evaluateBlock, &expression->data.forLoop->body);
			}
			END;
			DEBUG_END;

			Expression null = (Expression) {
				.type = EXPRESSION_IDENTIFIER,
				.data = (ExpressionData) {
					.identifier = "null",
				},
			};
			DEBUG_END;
			DEBUG_END;
			RETURN_OK(output, null);
		}

		// While loop
		case EXPRESSION_WHILE_LOOP: {
			DEBUG_START("Evaluating while loop");

			while (true) {
				TRY_LET(Expression, condition, evaluateExpression, &expression->data.whileLoop->condition);

				if (!isBoolean(condition)) {
					RETURN_ERROR("While loop condition must be a boolean");
				}

				if (!condition.data.boolean) {
					break;
				}

				DEBUG_START("Evaluating while loop body");
				TRY_LET(Expression, blockValue, evaluateBlock, &expression->data.whileLoop->body);
				DEBUG_END;
			}

			Expression null = (Expression) {
				.type = EXPRESSION_IDENTIFIER,
				.data = (ExpressionData) {
					.identifier = "null",
				},
			};
			DEBUG_END;
			DEBUG_END;
			RETURN_OK(output, null);
		}

		// While loop
		case EXPRESSION_IF: {
			DEBUG_START("Evaluating if expression");

			TRY_LET(Expression, condition, evaluateExpression, &expression->data.whileLoop->condition);

			if (!isBoolean(condition)) {
				RETURN_ERROR("If expression condition isn't a boolean");
			}

			if (condition.data.boolean) {
				DEBUG_START("Evaluating if expression body");
				TRY_LET(Expression, blockValue, evaluateBlock, &expression->data.whileLoop->body);
				DEBUG_END;
			}

			Expression null = (Expression) {
				.type = EXPRESSION_IDENTIFIER,
				.data = (ExpressionData) {
					.identifier = "null",
				},
			};
			DEBUG_END;
			DEBUG_END;
			RETURN_OK(output, null);
		}

		// Unary expressions
		case EXPRESSION_UNARY: {
			DEBUG_START("Evaluating unary expression");
			UnaryExpression* unary = expression->data.unary;
			DEBUG_START("Evaluating operand of unary expression");
			TRY_LET(Expression, inner, evaluateExpression, &unary->expression);
			DEBUG_END;
			switch (unary->operation.type) {

				// Function call
				case UNARY_OPERATION_FUNCTION_CALL: {
					DEBUG_START("Evaluating function call");
					switch (inner.type) {

						// Calling a regular function
						case EXPRESSION_FUNCTION: {
							DEBUG_START("Evaluating a literal function call");
							if (inner.data.function.thisObject != NULL) {
								Expression* thisObject = inner.data.function.thisObject;
								TRY(prependToExpressionList(&unary->operation.data.functionCall, *thisObject));
								inner.data.function.thisObject = NULL;
							}
							DEBUG_START("Setting function call arguments");
							for (size_t parameterNumber = 0; parameterNumber < inner.data.function.parameters.size; parameterNumber++) {
								if (parameterNumber >= unary->operation.data.functionCall.size) {
									RETURN_ERROR("Not enough arguments for function call: expected %lu but found %lu", inner.data.function.parameters.size, unary->operation.data.functionCall.size);
								}
								Declaration declaration = (Declaration) {
									.name = inner.data.function.parameters.data[parameterNumber].name,
									.value = unary->operation.data.functionCall.data[parameterNumber],
								};
								TRY(setVariable(inner.data.function.body.innerScope, declaration));
							}
							DEBUG_END;
							TRY(evaluateBlock(&inner.data.function.body, output));
							DEBUG_END;
							DEBUG_END;
							DEBUG_END;
							DEBUG_END;
							return OK;
						}

						case EXPRESSION_BUILTIN_FUNCTION: {
							bool hadThisObject = false;
							if (inner.data.builtinFunction.thisObject != NULL) {
								TRY(prependToExpressionList(&unary->operation.data.functionCall, *inner.data.builtinFunction.thisObject));
								inner.data.builtinFunction.thisObject = NULL;
								hadThisObject = true;
							}
							TRY(inner.data.builtinFunction.function(&unary->operation.data.functionCall, output));

							if (hadThisObject) {
								TRY(popExpressionList(&unary->operation.data.functionCall));
							}

							DEBUG_END;
							DEBUG_END;
							DEBUG_END;
							return OK;
						}

						// Calling an identifier such as `name()`
						case EXPRESSION_IDENTIFIER: {
							DEBUG_START("Evaluating identifier function call");
							String identifier = inner.data.identifier;

							// Calling `builtin()`
							if (strcmp(identifier, "builtin") == 0) {
								DEBUG_START("Evaluating call to builtin()");

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

								DEBUG_END;
								DEBUG_END;
								DEBUG_END;
								DEBUG_END;
								DEBUG_END;
								RETURN_OK(output, functionExpression);
							}

							// Not `builtin()`
							TRY_LET(Expression*, value, getVariable, *CONTEXT->scope, identifier);

							// Calling a function
							if (value->type == EXPRESSION_FUNCTION) {
								bool hadThisObject = false;
								if (value->data.function.thisObject != NULL) {
									TRY(prependToExpressionList(&unary->operation.data.functionCall, *inner.data.function.thisObject));
									inner.data.function.thisObject = NULL;
									hadThisObject = true;
								}

								DEBUG_START("Setting function call arguments");
								for (size_t parameterNumber = 0; parameterNumber < value->data.function.parameters.size; parameterNumber++) {
									if (parameterNumber >= unary->operation.data.functionCall.size) {
										RETURN_ERROR("Not enough arguments for function call: expected %lu but found %lu", value->data.function.parameters.size, unary->operation.data.functionCall.size);
									}
									Declaration declaration = (Declaration) {
										.name = value->data.function.parameters.data[parameterNumber].name,
										.value = unary->operation.data.functionCall.data[parameterNumber],
									};
									TRY(setVariable(value->data.function.body.innerScope, declaration));
								}
								DEBUG_END;

								DEBUG_START("Evaluating function call body");
								TRY(evaluateBlock(&value->data.function.body, output));
								if (hadThisObject) {
									TRY(popExpressionList(&unary->operation.data.functionCall));
								}
								DEBUG_END;

								DEBUG_END;
								DEBUG_END;
								DEBUG_END;
								DEBUG_END;

								if (isReturning) {
									isReturning = false;
									RETURN_OK(output, returnValue);
								}

								return OK;
							}

							// Calling a builtin function like `print()`
							if (value->type == EXPRESSION_BUILTIN_FUNCTION) {
								bool hadThisObject = false;
								if (value->data.builtinFunction.thisObject != NULL) {
									TRY(prependToExpressionList(&unary->operation.data.functionCall, *inner.data.builtinFunction.thisObject));
									inner.data.builtinFunction.thisObject = NULL;
									hadThisObject = true;
								}
								TRY(value->data.builtinFunction.function(&unary->operation.data.functionCall, output));
								if (hadThisObject) {
									TRY(popExpressionList(&unary->operation.data.functionCall));
								}
								DEBUG_END;
								DEBUG_END;
								DEBUG_END;
								DEBUG_END;
								return OK;
							}

							// Calling something else (like an identifier that resolves to a number)
							RETURN_ERROR("Attempted to call a value that's not a function");
						}

						// Calling something else (like a number literal like `3()`)
						default: {
							DEBUG_END;
							DEBUG_END;
							DEBUG_END;
							RETURN_ERROR("Attempted to call a value that's not a function");
						}
					}
				}

				// Negation
				case UNARY_OPERATION_NOT: {
					DEBUG_START("Evaluating unary not expression");

					if (!isBoolean(inner)) {
						RETURN_ERROR("Attempted to negate a value that's not a boolean");
					}

					bool result = !expression->data.boolean;
					TRY_LET(Expression, resultExpression, booleanExpression, result);
					DEBUG_END;
					DEBUG_END;
					RETURN_OK(output, resultExpression);
				}
			}
		}

		// Binary Expression
		case EXPRESSION_BINARY: {
			DEBUG_START("Evaluating binary expression");

			DEBUG_START("Evaluating left-hand side of binary expression");
			Expression left;
			TRY(evaluateExpression(expression->data.binary.left, &left));
			DEBUG_END;

			DEBUG_START("Evaluating right-hand side of binary expression");
			Expression right;
			TRY(evaluateExpression(expression->data.binary.right, &right));
			DEBUG_END;

			switch (expression->data.binary.operation) {
				// Field access
				case BINARY_OPERATION_DOT: {
					DEBUG_START("Evaluating field access");

					Expression* leftPointer = &left;
					if (leftPointer->type == EXPRESSION_IDENTIFIER) {
						TRY(getVariable(*CONTEXT->scope, left.data.identifier, &leftPointer));
					}

					if (right.type != EXPRESSION_IDENTIFIER) {
						RETURN_ERROR("The right-hand side of a field access must be a field name");
					}

					if (leftPointer->type != EXPRESSION_OBJECT) {
						RETURN_ERROR("The left-hand side of a field access must be an object");
					}
					Expression* fieldValue;
					TRY(getObjectField(*leftPointer->data.object, right.data.identifier, &fieldValue));
					if (fieldValue->type == EXPRESSION_IDENTIFIER) {
						TRY(getVariable(*CONTEXT->scope, fieldValue->data.identifier, &fieldValue));
					}

					if (fieldValue->type == EXPRESSION_FUNCTION) {
						fieldValue->data.function.thisObject = malloc(sizeof(Expression));
						*fieldValue->data.function.thisObject = *leftPointer;
					} else if (fieldValue->type == EXPRESSION_BUILTIN_FUNCTION) {
						fieldValue->data.builtinFunction.thisObject = malloc(sizeof(Expression));
						*fieldValue->data.builtinFunction.thisObject = *leftPointer;
					}

					DEBUG_END;
					DEBUG_END;
					DEBUG_END;
					RETURN_OK(output, *fieldValue);
				}

				case BINARY_OPERATION_PLUS: {
					DEBUG_START("Evaluating addition operation");
					Expression* leftPointer = &left;
					if (leftPointer->type == EXPRESSION_IDENTIFIER) {
						TRY(getVariable(*CONTEXT->scope, left.data.identifier, &leftPointer));
					}
					Expression* rightPointer = &right;
					if (rightPointer->type == EXPRESSION_IDENTIFIER) {
						TRY(getVariable(*CONTEXT->scope, right.data.identifier, &rightPointer));
					}

					if (isNumber(*leftPointer)) {
						TRY_LET(double*, leftNumber, getNumber, *leftPointer);
						TRY_LET(double*, rightNumber, getNumber, *rightPointer);
						TRY_LET(Expression, sum, numberExpression, *leftNumber + *rightNumber);
						DEBUG_END;
						DEBUG_END;
						DEBUG_END;
						RETURN_OK(output, sum);
					}

					if (isString(*leftPointer)) {
						TRY_LET(String*, leftString, getString, *leftPointer);
						TRY_LET(String*, rightString, getString, *rightPointer);
						String result = malloc(strlen(*leftString) + strlen(*rightString) + 1);
						strcpy(result, *leftString);
						strcat(result, *rightString);
						TRY_LET(Expression, sum, stringExpression, result);
						DEBUG_END;
						DEBUG_END;
						DEBUG_END;
						RETURN_OK(output, sum);
					}

					DEBUG_END;
					DEBUG_END;
					DEBUG_END;

					RETURN_ERROR("Attempted to add two unaddable values.");
				}

				case BINARY_OPERATION_LESS_THAN_OR_EQUAL_TO: {
					DEBUG_START("Evaluating less than or equal to operation");

					Expression* leftPointer = &left;
					if (leftPointer->type == EXPRESSION_IDENTIFIER) {
						TRY(getVariable(*CONTEXT->scope, left.data.identifier, &leftPointer));
					}
					Expression* rightPointer = &right;
					if (rightPointer->type == EXPRESSION_IDENTIFIER) {
						TRY(getVariable(*CONTEXT->scope, right.data.identifier, &rightPointer));
					}

					TRY_LET(double*, leftNumber, getNumber, *leftPointer);
					TRY_LET(double*, rightNumber, getNumber, *rightPointer);
					TRY_LET(Expression, result, booleanExpression, *leftNumber <= *rightNumber);
					DEBUG_END;
					DEBUG_END;
					DEBUG_END;
					RETURN_OK(output, result);
				}

				case BINARY_OPERATION_EQUAL: {
					DEBUG_START("Evaluating equal to operation");

					Expression* leftPointer = &left;
					if (leftPointer->type == EXPRESSION_IDENTIFIER) {
						TRY(getVariable(*CONTEXT->scope, left.data.identifier, &leftPointer));
					}
					Expression* rightPointer = &right;
					if (rightPointer->type == EXPRESSION_IDENTIFIER) {
						TRY(getVariable(*CONTEXT->scope, right.data.identifier, &rightPointer));
					}

					TRY_LET(Expression, result, expressionsAreEqual, *leftPointer, *rightPointer);

					DEBUG_END;
					DEBUG_END;
					DEBUG_END;
					RETURN_OK(output, result);
				}

				case BINARY_OPERATION_ASSIGN: {
					DEBUG_START("Evaluating assignment");
					Expression* rightPointer = &right;
					if (rightPointer->type == EXPRESSION_IDENTIFIER) {
						TRY(getVariable(*CONTEXT->scope, right.data.identifier, &rightPointer));
					}
					Declaration declaration = (Declaration) {
						.name = left.data.identifier,
						.value = *rightPointer,
					};
					TRY(reassignVariable(CONTEXT->scope, declaration));

					Expression null = (Expression) {
						.type = EXPRESSION_IDENTIFIER,
						.data = (ExpressionData) {
							.identifier = "null",
						},
					};
					DEBUG_END;
					DEBUG_END;
					DEBUG_END;
					RETURN_OK(output, null);
				}
				default: {
					UNREACHABLE;
				}
			}
		}
	}
}

PRIVATE Result evaluateStatement(Statement* statement) {
	if (isReturning) {
		return OK;
	}

	DEBUG_START("Evaluating statement");
	switch (statement->type) {
		case STATEMENT_EXPRESSION: {
			Expression output;
			TRY(evaluateExpression(&statement->data.expression, &output));
			DEBUG_END;
			return OK;
		}

		case STATEMENT_DECLARATION: {
			DEBUG_START("Evaluating declaration for \"%s\"", statement->data.declaration.name);
			TRY(evaluateExpression(&statement->data.declaration.value, &statement->data.declaration.value));
			if (statement->data.declaration.value.type == EXPRESSION_IDENTIFIER) {
				Expression* value;
				TRY(getVariable(*CONTEXT->scope, statement->data.declaration.value.data.identifier, &value));
				statement->data.declaration.value = *value;
			}
			TRY(setVariable(CONTEXT->scope, statement->data.declaration));
			DEBUG_END;
			DEBUG_END;
			return OK;
		}
		case STATEMENT_RETURN: {
			DEBUG_START("Evaluating return statement");
			isReturning = true;
			TRY(evaluateExpression(&statement->data.returnExpression, &returnValue));
			if (returnValue.type == EXPRESSION_IDENTIFIER) {
				Expression* value;
				TRY(getVariable(*CONTEXT->scope, returnValue.data.identifier, &value));
				returnValue = *value;
			}
			DEBUG_END;
			DEBUG_END;
			return OK;
		}
	}
}

Result run(Program program) {
	FOR_EACH(Statement statement, program.statements) {
		TRY(evaluateStatement(&statement));
	}
	END;

	return OK;
}
