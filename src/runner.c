#include "../include/runner.h"
#include "../include/builtin.h"
#include "../include/context.h"
#include "../include/parser.h"
#include "../include/sugar.h"
#include <math.h>

static bool isReturning = false;
static Value returnValue;

PRIVATE KleinResult evaluateStatement(Statement statement);
KleinResult evaluateExpression(Expression expression, Value* output);

PRIVATE KleinResult evaluateObject(Object object, Value* output) {
	ValueFieldList* list = emptyHeapValueFieldList();
	FOR_EACH(Field field, object.fields) {
		TRY_LET(Value value, evaluateExpression(field.value, &value));
		ValueField valueField = (ValueField) {
			.name = field.name,
			.value = value,
		};
		appendToValueFieldList(list, valueField);
	}
	END;

	InternalList internals = emptyInternalList();

	Value result = (Value) {.fields = list, .internals = internals};
	RETURN_OK(output, result);
}

PRIVATE KleinResult evaluateBlock(Block block, Value* output) {
	Scope* previousScope = CONTEXT->scope;
	CONTEXT->scope = block.innerScope;

	FOR_EACHP(Statement statement, block.statements) {
		evaluateStatement(statement);
	}
	END;

	CONTEXT->scope = previousScope;

	return nullValue(output);
}

PRIVATE KleinResult evaluateList(ExpressionList list, Value* output) {
	ValueList elements = emptyValueList();
	FOR_EACH(Expression element, list) {
		TRY_LET(Value value, evaluateExpression(element, &value));
		appendToValueList(&elements, value);
	}
	END;

	return listValue(elements, output);
}

PRIVATE KleinResult evaluateForLoop(ForLoop forLoop, Value* output) {
	TRY_LET(Value list, evaluateExpression(forLoop.list, &list));
	TRY_LET(String iterable, valueToString(list, &iterable));
	TRY_LET(ValueList * elements, getList(list, &elements));

	FOR_EACHP(Value value, elements) {
		ScopeDeclaration declaration = (ScopeDeclaration) {
			.name = forLoop.binding,
			.value = value,
		};
		setVariable(forLoop.body.innerScope, declaration);
		TRY_LET(Value blockValue, evaluateBlock(forLoop.body, &blockValue));
	}
	END;

	return nullValue(output);
}

PRIVATE KleinResult evaluateWhileLoop(WhileLoop whileLoop, Value* output) {
	while (true) {
		TRY_LET(Value condition, evaluateExpression(whileLoop.condition, &condition));
		TRY_LET(bool* conditionValue, getBoolean(condition, &conditionValue));

		if (!*conditionValue) {
			break;
		}

		TRY_LET(Value blockValue, evaluateBlock(whileLoop.body, &blockValue));
	}

	return nullValue(output);
}

PRIVATE KleinResult evaluateIfExpression(IfExpressionList ifExpressions, Value* output) {
	FOR_EACH(IfExpression ifExpression, ifExpressions) {
		TRY_LET(Value condition, evaluateExpression(ifExpression.condition, &condition));
		TRY_LET(bool* conditionValue, getBoolean(condition, &conditionValue));

		if (*conditionValue) {
			TRY_LET(Value blockValue, evaluateBlock(ifExpression.body, &blockValue));
			break;
		}
	}
	END;

	return nullValue(output);
}

PRIVATE KleinResult evaluateBinaryExpression(BinaryExpression binary, Value* output) {
	switch (binary.operation) {
		case BINARY_OPERATION_DOT: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left));
			TRY_LET(Value * value, getValueField(left, binary.right.data.identifier, &value));
			Value* this = malloc(sizeof(Value));
			*this = left;
			appendToInternalList(&value->internals, (Internal) {.key = INTERNAL_KEY_THIS_OBJECT, .value = this});
			RETURN_OK(output, *value);
		}
		case BINARY_OPERATION_LESS_THAN_OR_EQUAL_TO: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left));
			TRY_LET(Value right, evaluateExpression(binary.right, &right));
			TRY_LET(double* leftNumber, getNumber(left, &leftNumber));
			TRY_LET(double* rightNumber, getNumber(right, &rightNumber));
			return booleanValue(*leftNumber <= *rightNumber, output);
		}
		case BINARY_OPERATION_LESS_THAN: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left));
			TRY_LET(Value right, evaluateExpression(binary.right, &right));
			TRY_LET(double* leftNumber, getNumber(left, &leftNumber));
			TRY_LET(double* rightNumber, getNumber(right, &rightNumber));
			return booleanValue(*leftNumber < *rightNumber, output);
		}
		case BINARY_OPERATION_GREATER_THAN: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left));
			TRY_LET(Value right, evaluateExpression(binary.right, &right));
			TRY_LET(double* leftNumber, getNumber(left, &leftNumber));
			TRY_LET(double* rightNumber, getNumber(right, &rightNumber));
			return booleanValue(*leftNumber > *rightNumber, output);
		}
		case BINARY_OPERATION_GREATER_THAN_OR_EQUAL_TO: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left));
			TRY_LET(Value right, evaluateExpression(binary.right, &right));
			TRY_LET(double* leftNumber, getNumber(left, &leftNumber));
			TRY_LET(double* rightNumber, getNumber(right, &rightNumber));
			return booleanValue(*leftNumber >= *rightNumber, output);
		}
		case BINARY_OPERATION_PLUS: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left));
			TRY_LET(Value right, evaluateExpression(binary.right, &right));
			TRY_LET(double* leftNumber, getNumber(left, &leftNumber));
			TRY_LET(double* rightNumber, getNumber(right, &rightNumber));
			return numberValue(*leftNumber + *rightNumber, output);
		}
		case BINARY_OPERATION_TIMES: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left));
			TRY_LET(Value right, evaluateExpression(binary.right, &right));
			TRY_LET(double* leftNumber, getNumber(left, &leftNumber));
			TRY_LET(double* rightNumber, getNumber(right, &rightNumber));
			return numberValue(*leftNumber * *rightNumber, output);
		}
		case BINARY_OPERATION_MINUS: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left));
			TRY_LET(Value right, evaluateExpression(binary.right, &right));
			TRY_LET(double* leftNumber, getNumber(left, &leftNumber));
			TRY_LET(double* rightNumber, getNumber(right, &rightNumber));
			return numberValue(*leftNumber - *rightNumber, output);
		}
		case BINARY_OPERATION_DIVIDE: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left));
			TRY_LET(Value right, evaluateExpression(binary.right, &right));
			TRY_LET(double* leftNumber, getNumber(left, &leftNumber));
			TRY_LET(double* rightNumber, getNumber(right, &rightNumber));
			return numberValue(*leftNumber / *rightNumber, output);
		}
		case BINARY_OPERATION_POWER: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left));
			TRY_LET(Value right, evaluateExpression(binary.right, &right));
			TRY_LET(double* leftNumber, getNumber(left, &leftNumber));
			TRY_LET(double* rightNumber, getNumber(right, &rightNumber));
			return numberValue(pow(*leftNumber, *rightNumber), output);
		}
		case BINARY_OPERATION_EQUAL: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left));
			TRY_LET(Value right, evaluateExpression(binary.right, &right));
			return valuesAreEqual(left, right, output);
		}
		case BINARY_OPERATION_AND: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left));
			TRY_LET(Value right, evaluateExpression(binary.right, &right));
			TRY_LET(bool* leftBoolean, getBoolean(left, &leftBoolean));
			TRY_LET(bool* rightBoolean, getBoolean(right, &rightBoolean));
			return booleanValue(*leftBoolean && *rightBoolean, output);
		}
		case BINARY_OPERATION_OR: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left));
			TRY_LET(Value right, evaluateExpression(binary.right, &right));
			TRY_LET(bool* leftBoolean, getBoolean(left, &leftBoolean));
			TRY_LET(bool* rightBoolean, getBoolean(right, &rightBoolean));
			return booleanValue(*leftBoolean || *rightBoolean, output);
		}
		case BINARY_OPERATION_ASSIGN: {
			if (binary.left.type != EXPRESSION_IDENTIFIER) {
				Expression* expression = malloc(sizeof(Expression));
				*expression = binary.left;
				return (KleinResult) {
					.type = KLEIN_ERROR_ASSIGN_TO_NON_IDENTIFIER,
					.data = (KleinResultData) {
						.assignToNonIdentifier = expression,
					},
				};
			}
			TRY_LET(Value right, evaluateExpression(binary.right, &right));
			return reassignVariable(CONTEXT->scope, (ScopeDeclaration) {.name = binary.left.data.identifier, .value = right});
		}
	}
}

PRIVATE KleinResult evaluateFunction(Function expression, Value* output) {
	return functionValue(expression, output);
}

PRIVATE KleinResult evaluateUnaryExpression(UnaryExpression unaryExpression, Value* output) {
	switch (unaryExpression.operation.type) {
		case UNARY_OPERATION_FUNCTION_CALL: {
			// Builtin
			if (unaryExpression.expression.type == EXPRESSION_IDENTIFIER && strcmp(unaryExpression.expression.data.identifier, "builtin") == 0) {
				TRY_LET(BuiltinFunction builtin, getBuiltin(unaryExpression.operation.data.functionCall.data[0].data.string, &builtin));
				return builtinFunctionToValue(builtin, output);
			}

			// Not builtin()
			TRY_LET(Value functionToCall, evaluateExpression(unaryExpression.expression, &functionToCall));

			// Builtin function like `print()`
			if (isBuiltinFunction(functionToCall)) {
				UNWRAP_LET(BuiltinFunction builtin, getValueInternal(functionToCall, INTERNAL_KEY_BUILTIN_FUNCTION, (void**) &builtin));
				ValueList arguments = emptyValueList();
				if (hasInternal(functionToCall, INTERNAL_KEY_THIS_OBJECT)) {
					UNWRAP_LET(Value * this, getValueInternal(functionToCall, INTERNAL_KEY_THIS_OBJECT, (void**) &this));
					appendToValueList(&arguments, *this);
				}
				FOR_EACH(Expression argumentExpression, unaryExpression.operation.data.functionCall) {
					TRY_LET(Value argument, evaluateExpression(argumentExpression, &argument));
					appendToValueList(&arguments, argument);
				}
				END;
				return (*builtin)(&arguments, output);
			}

			// Regular function
			TRY_LET(Function * function, getFunction(functionToCall, &function));

			// Arguments
			if (function->parameters.size != unaryExpression.operation.data.functionCall.size) {
				return (KleinResult) {
					.type = KLEIN_ERROR_INCORRECT_ARGUMENT_COUNT,
					.data = (KleinResultData) {
						.incorrectArgumentCount = (KleinIncorrectArgumentCountError) {
							.expected = function->parameters.size,
							.actual = unaryExpression.operation.data.functionCall.size,
						},
					},
				};
			}
			for (size_t parameterNumber = 0; parameterNumber < function->parameters.size; parameterNumber++) {
				TRY_LET(Value argument, evaluateExpression(unaryExpression.operation.data.functionCall.data[parameterNumber], &argument));
				TRY(setVariable(function->body.innerScope, (ScopeDeclaration) {.name = function->parameters.data[parameterNumber].name, .value = argument}));
			}

			// Body
			TRY_LET(Value result, evaluateBlock(function->body, &result));

			// Return
			if (isReturning) {
				isReturning = false;
				TRY_LET(String returning, valueToString(returnValue, &returning));
				RETURN_OK(output, returnValue);
			}

			return nullValue(output);
		}
		case UNARY_OPERATION_NOT: {
			TRY_LET(Value operand, evaluateExpression(unaryExpression.expression, &operand));
			TRY_LET(bool* boolean, getBoolean(operand, &boolean));
			return booleanValue(!*boolean, output);
		}
		case UNARY_OPERATION_INDEX: {
			TRY_LET(Value operand, evaluateExpression(unaryExpression.expression, &operand));
			TRY_LET(Value index, evaluateExpression(unaryExpression.operation.data.index, &index));

			if (isString(index)) {
				UNWRAP_LET(String * string, getString(index, &string));
				TRY_LET(Value * field, getValueField(operand, *string, &field));
				RETURN_OK(output, *field);
			}

			if (isNumber(index) && isList(operand)) {
				UNWRAP_LET(double* number, getNumber(index, &number));
				UNWRAP_LET(ValueList * list, getList(operand, &list));
				RETURN_OK(output, list->data[(int) *number]);
			}

			return (KleinResult) {
				.type = KLEIN_ERROR_INVALID_INDEX,
			};
		}
	}
}

KleinResult evaluateExpression(Expression expression, Value* output) {
	switch (expression.type) {
		case EXPRESSION_OBJECT: {
			return evaluateObject(*expression.data.object, output);
		}
		case EXPRESSION_IDENTIFIER: {
			TRY_LET(Value * result, getVariable(*CONTEXT->scope, expression.data.identifier, &result));
			RETURN_OK(output, *result);
		}
		case EXPRESSION_BLOCK: {
			return evaluateBlock(*expression.data.block, output);
		}
		case EXPRESSION_FOR_LOOP: {
			return evaluateForLoop(*expression.data.forLoop, output);
		}
		case EXPRESSION_WHILE_LOOP: {
			return evaluateWhileLoop(*expression.data.whileLoop, output);
		}
		case EXPRESSION_IF: {
			return evaluateIfExpression(*expression.data.ifExpression, output);
		}
		case EXPRESSION_BINARY: {
			return evaluateBinaryExpression(*expression.data.binary, output);
		}
		case EXPRESSION_STRING: {
			return stringValue(expression.data.string, output);
		}
		case EXPRESSION_NUMBER: {
			return numberValue(expression.data.number, output);
		}
		case EXPRESSION_LIST: {
			return evaluateList(*expression.data.list, output);
		}
		case EXPRESSION_FUNCTION: {
			return evaluateFunction(expression.data.function, output);
		}
		case EXPRESSION_UNARY: {
			return evaluateUnaryExpression(*expression.data.unary, output);
		}
		case EXPRESSION_BOOLEAN: {
			return booleanValue(expression.data.boolean, output);
		}
		case EXPRESSION_BUILTIN_FUNCTION: {
			UNREACHABLE;
		}
	}
}

PRIVATE KleinResult evaluateStatement(Statement statement) {
	if (isReturning) {
		return OK;
	}

	switch (statement.type) {
		case STATEMENT_EXPRESSION: {
			TRY_LET(Value value, evaluateExpression(statement.data.expression, &value));
			return OK;
		}
		case STATEMENT_DECLARATION: {
			TRY_LET(Value value, evaluateExpression(statement.data.declaration.value, &value));
			ScopeDeclaration declaration = (ScopeDeclaration) {
				.name = statement.data.declaration.name,
				.value = value,
			};
			setVariable(CONTEXT->scope, declaration);
			return OK;
		}
		case STATEMENT_RETURN: {
			evaluateExpression(statement.data.returnExpression, &returnValue);
			isReturning = true;
			return OK;
		}
	}
}

KleinResult run(Program program) {
	FOR_EACH(Statement statement, program.statements) {
		evaluateStatement(statement);
	}
	END;

	return OK;
}
