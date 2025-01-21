#include "../include/runner.h"
#include "../include/builtin.h"
#include "../include/context.h"
#include "../include/parser.h"
#include "../include/sugar.h"
#include <math.h>

static bool isReturning = false;
static Value returnValue;

PRIVATE Result evaluateStatement(Statement statement);
Result evaluateExpression(Expression expression, Value* output);

PRIVATE Result evaluateObject(Object object, Value* output) {
	TRY_LET(ValueFieldList * list, emptyHeapValueFieldList(&list), "creating the field list for an object's value");
	FOR_EACH(Field field, object.fields) {
		TRY_LET(Value value, evaluateExpression(field.value, &value), "evaluating the value of the field \"%s\" on an object", field.name);
		ValueField valueField = (ValueField) {
			.name = field.name,
			.value = value,
		};
		TRY(appendToValueFieldList(list, valueField), "appending to a value's field list");
	}
	END;

	InternalList internals;
	TRY(emptyInternalList(&internals), "creating an object's internals list");

	Value result = (Value) {.fields = list, .internals = internals};
	RETURN_OK(output, result);
}

PRIVATE Result evaluateBlock(Block block, Value* output) {
	Scope* previousScope = CONTEXT->scope;
	CONTEXT->scope = block.innerScope;

	FOR_EACHP(Statement statement, block.statements) {
		TRY(evaluateStatement(statement), "evaluating a statement in a block");
	}
	END;

	CONTEXT->scope = previousScope;

	return nullValue(output);
}

PRIVATE Result evaluateList(ExpressionList list, Value* output) {
	ValueList elements;
	TRY(emptyValueList(&elements), "creating a list literal's element list");
	FOR_EACH(Expression element, list) {
		TRY_LET(Value value, evaluateExpression(element, &value), "evaluating an element of a list literal");
		TRY(appendToValueList(&elements, value), "appending a value to a list literal's elements");
	}
	END;

	return listValue(elements, output);
}

PRIVATE Result evaluateForLoop(ForLoop forLoop, Value* output) {
	TRY_LET(Value list, evaluateExpression(forLoop.list, &list), "evaluating the list to iterate on in a forloop");
	TRY_LET(String iterable, valueToString(list, &iterable), "");
	TRY_LET(ValueList * elements, getList(list, &elements), "getting the elements of the list to iterate on in a for loop");

	FOR_EACHP(Value value, elements) {
		ScopeDeclaration declaration = (ScopeDeclaration) {
			.name = forLoop.binding,
			.value = value,
		};
		setVariable(forLoop.body.innerScope, declaration);
		TRY_LET(Value blockValue, evaluateBlock(forLoop.body, &blockValue), "evaluating the body of a for loop");
	}
	END;

	return nullValue(output);
}

PRIVATE Result evaluateWhileLoop(WhileLoop whileLoop, Value* output) {
	while (true) {
		TRY_LET(Value condition, evaluateExpression(whileLoop.condition, &condition), "evaluating the condition of a while loop");
		TRY_LET(bool* conditionValue, getBoolean(condition, &conditionValue), "getting the boolean value of a while loop's condition");

		if (!*conditionValue) {
			break;
		}

		TRY_LET(Value blockValue, evaluateBlock(whileLoop.body, &blockValue), "evaluating the body of a while loop");
	}

	return nullValue(output);
}

PRIVATE Result evaluateIfExpression(IfExpressionList ifExpressions, Value* output) {
	FOR_EACH(IfExpression ifExpression, ifExpressions) {
		TRY_LET(Value condition, evaluateExpression(ifExpression.condition, &condition), "evaluating the condition of an if-expression");
		TRY_LET(bool* conditionValue, getBoolean(condition, &conditionValue), "getting the boolean value of an if-expression's condition");

		if (*conditionValue) {
			TRY_LET(Value blockValue, evaluateBlock(ifExpression.body, &blockValue), "evaluating the body of an if-expression");
			break;
		}
	}
	END;

	return nullValue(output);
}

PRIVATE Result evaluateBinaryExpression(BinaryExpression binary, Value* output) {
	switch (binary.operation) {
		case BINARY_OPERATION_DOT: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left), "evaluating the left-hand side of a field access");
			TRY_LET(Value * value, getValueField(left, binary.right.data.identifier, &value), "getting the field \"%s\" on an object", binary.right.data.identifier);
			Value* this = malloc(sizeof(Value));
			*this = left;
			TRY(appendToInternalList(&value->internals, (Internal) {.key = INTERNAL_KEY_THIS_OBJECT, .value = this}), "setting the this-object for a field");
			RETURN_OK(output, *value);
		}
		case BINARY_OPERATION_LESS_THAN_OR_EQUAL_TO: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left), "evaluating the left-hand side of a binary expression");
			TRY_LET(Value right, evaluateExpression(binary.right, &right), "evaluating the right-hand side of a binary expression");
			TRY_LET(double* leftNumber, getNumber(left, &leftNumber), "getting the left-hand side of a <= expression as a number");
			TRY_LET(double* rightNumber, getNumber(right, &rightNumber), "getting the right-hand side of a <= expression as a number");
			return booleanValue(*leftNumber <= *rightNumber, output);
		}
		case BINARY_OPERATION_LESS_THAN: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left), "evaluating the left-hand side of a binary expression");
			TRY_LET(Value right, evaluateExpression(binary.right, &right), "evaluating the right-hand side of a binary expression");
			TRY_LET(double* leftNumber, getNumber(left, &leftNumber), "getting the left-hand side of a <= expression as a number");
			TRY_LET(double* rightNumber, getNumber(right, &rightNumber), "getting the right-hand side of a <= expression as a number");
			return booleanValue(*leftNumber < *rightNumber, output);
		}
		case BINARY_OPERATION_GREATER_THAN: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left), "evaluating the left-hand side of a binary expression");
			TRY_LET(Value right, evaluateExpression(binary.right, &right), "evaluating the right-hand side of a binary expression");
			TRY_LET(double* leftNumber, getNumber(left, &leftNumber), "getting the left-hand side of a <= expression as a number");
			TRY_LET(double* rightNumber, getNumber(right, &rightNumber), "getting the right-hand side of a <= expression as a number");
			return booleanValue(*leftNumber > *rightNumber, output);
		}
		case BINARY_OPERATION_GREATER_THAN_OR_EQUAL_TO: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left), "evaluating the left-hand side of a binary expression");
			TRY_LET(Value right, evaluateExpression(binary.right, &right), "evaluating the right-hand side of a binary expression");
			TRY_LET(double* leftNumber, getNumber(left, &leftNumber), "getting the left-hand side of a <= expression as a number");
			TRY_LET(double* rightNumber, getNumber(right, &rightNumber), "getting the right-hand side of a <= expression as a number");
			return booleanValue(*leftNumber >= *rightNumber, output);
		}
		case BINARY_OPERATION_PLUS: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left), "evaluating the left-hand side of a binary expression");
			TRY_LET(Value right, evaluateExpression(binary.right, &right), "evaluating the right-hand side of a binary expression");
			TRY_LET(double* leftNumber, getNumber(left, &leftNumber), "getting the left-hand side of a + expression as a number");
			TRY_LET(double* rightNumber, getNumber(right, &rightNumber), "getting the right-hand side of a + expression as a number");
			return numberValue(*leftNumber + *rightNumber, output);
		}
		case BINARY_OPERATION_TIMES: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left), "evaluating the left-hand side of a binary expression");
			TRY_LET(Value right, evaluateExpression(binary.right, &right), "evaluating the right-hand side of a binary expression");
			TRY_LET(double* leftNumber, getNumber(left, &leftNumber), "getting the left-hand side of a + expression as a number");
			TRY_LET(double* rightNumber, getNumber(right, &rightNumber), "getting the right-hand side of a + expression as a number");
			return numberValue(*leftNumber * *rightNumber, output);
		}
		case BINARY_OPERATION_MINUS: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left), "evaluating the left-hand side of a binary expression");
			TRY_LET(Value right, evaluateExpression(binary.right, &right), "evaluating the right-hand side of a binary expression");
			TRY_LET(double* leftNumber, getNumber(left, &leftNumber), "getting the left-hand side of a + expression as a number");
			TRY_LET(double* rightNumber, getNumber(right, &rightNumber), "getting the right-hand side of a + expression as a number");
			return numberValue(*leftNumber - *rightNumber, output);
		}
		case BINARY_OPERATION_DIVIDE: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left), "evaluating the left-hand side of a binary expression");
			TRY_LET(Value right, evaluateExpression(binary.right, &right), "evaluating the right-hand side of a binary expression");
			TRY_LET(double* leftNumber, getNumber(left, &leftNumber), "getting the left-hand side of a + expression as a number");
			TRY_LET(double* rightNumber, getNumber(right, &rightNumber), "getting the right-hand side of a + expression as a number");
			return numberValue(*leftNumber / *rightNumber, output);
		}
		case BINARY_OPERATION_POWER: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left), "evaluating the left-hand side of a binary expression");
			TRY_LET(Value right, evaluateExpression(binary.right, &right), "evaluating the right-hand side of a binary expression");
			TRY_LET(double* leftNumber, getNumber(left, &leftNumber), "getting the left-hand side of a + expression as a number");
			TRY_LET(double* rightNumber, getNumber(right, &rightNumber), "getting the right-hand side of a + expression as a number");
			return numberValue(pow(*leftNumber, *rightNumber), output);
		}
		case BINARY_OPERATION_EQUAL: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left), "evaluating the left-hand side of a binary expression");
			TRY_LET(Value right, evaluateExpression(binary.right, &right), "evaluating the right-hand side of a binary expression");
			return valuesAreEqual(left, right, output);
		}
		case BINARY_OPERATION_AND: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left), "evaluating the left-hand side of a binary expression");
			TRY_LET(Value right, evaluateExpression(binary.right, &right), "evaluating the right-hand side of a binary expression");
			TRY_LET(bool* leftBoolean, getBoolean(left, &leftBoolean), "getting the left-hand side of an \"and\" expression as a boolean");
			TRY_LET(bool* rightBoolean, getBoolean(right, &rightBoolean), "getting the left-hand side of an \"and\" expression as a boolean");
			return booleanValue(*leftBoolean && *rightBoolean, output);
		}
		case BINARY_OPERATION_OR: {
			TRY_LET(Value left, evaluateExpression(binary.left, &left), "evaluating the left-hand side of a binary expression");
			TRY_LET(Value right, evaluateExpression(binary.right, &right), "evaluating the right-hand side of a binary expression");
			TRY_LET(bool* leftBoolean, getBoolean(left, &leftBoolean), "getting the left-hand side of an \"or\" expression as a boolean");
			TRY_LET(bool* rightBoolean, getBoolean(right, &rightBoolean), "getting the left-hand side of an \"or\" expression as a boolean");
			return booleanValue(*leftBoolean || *rightBoolean, output);
		}
		case BINARY_OPERATION_ASSIGN: {
			if (binary.left.type != EXPRESSION_IDENTIFIER) {
				RETURN_ERROR("Attempted to assign to non-identifier");
			}
			TRY_LET(Value right, evaluateExpression(binary.right, &right), "evaluating the right-hand side of a binary expression");
			return reassignVariable(CONTEXT->scope, (ScopeDeclaration) {.name = binary.left.data.identifier, .value = right});
		}
	}
}

PRIVATE Result evaluateFunction(Function expression, Value* output) {
	return functionValue(expression, output);
}

PRIVATE Result evaluateUnaryExpression(UnaryExpression unaryExpression, Value* output) {
	switch (unaryExpression.operation.type) {
		case UNARY_OPERATION_FUNCTION_CALL: {
			// Builtin
			if (unaryExpression.expression.type == EXPRESSION_IDENTIFIER && strcmp(unaryExpression.expression.data.identifier, "builtin") == 0) {
				TRY_LET(BuiltinFunction builtin, getBuiltin(unaryExpression.operation.data.functionCall.data[0].data.string, &builtin), "getting a builtin function");
				return builtinFunctionToValue(builtin, output);
			}

			// Not builtin()
			TRY_LET(Value functionToCall, evaluateExpression(unaryExpression.expression, &functionToCall), "evaluting the function to call in a function call expression");

			// Builtin function like `print()`
			if (isBuiltinFunction(functionToCall)) {
				UNWRAP_LET(BuiltinFunction builtin, getValueInternal(functionToCall, INTERNAL_KEY_BUILTIN_FUNCTION, (void**) &builtin));
				TRY_LET(ValueList arguments, emptyValueList(&arguments), "creating a builtin function call's argument list");
				if (hasInternal(functionToCall, INTERNAL_KEY_THIS_OBJECT)) {
					UNWRAP_LET(Value * this, getValueInternal(functionToCall, INTERNAL_KEY_THIS_OBJECT, (void**) &this));
					TRY(appendToValueList(&arguments, *this), "appending to a builtin function call's argument list");
				}
				FOR_EACH(Expression argumentExpression, unaryExpression.operation.data.functionCall) {
					TRY_LET(Value argument, evaluateExpression(argumentExpression, &argument), "evaluating an argument to a function call");
					TRY(appendToValueList(&arguments, argument), "appending to a builtin function call's argument list");
				}
				END;
				return (*builtin)(&arguments, output);
			}

			// Regular function
			TRY_LET(Function * function, getFunction(functionToCall, &function), "getting a function call's operand as a function");

			// Arguments
			if (function->parameters.size != unaryExpression.operation.data.functionCall.size) {
				RETURN_ERROR("Incorrect number of arguments to function call: Expected %lu but received %lu", function->parameters.size, unaryExpression.operation.data.functionCall.size);
			}
			for (size_t parameterNumber = 0; parameterNumber < function->parameters.size; parameterNumber++) {
				TRY_LET(Value argument, evaluateExpression(unaryExpression.operation.data.functionCall.data[parameterNumber], &argument), "evaluating an argument to a function call");
				TRY(setVariable(function->body.innerScope, (ScopeDeclaration) {.name = function->parameters.data[parameterNumber].name, .value = argument}), "passing the argument of the function parameter %s", function->parameters.data[parameterNumber].name);
			}

			// Body
			TRY_LET(Value result, evaluateBlock(function->body, &result), "evaluating the body of a function when calling it");

			// Return
			if (isReturning) {
				isReturning = false;
				TRY_LET(String returning, valueToString(returnValue, &returning), "");
				RETURN_OK(output, returnValue);
			}

			return nullValue(output);
		}
		case UNARY_OPERATION_NOT: {
			TRY_LET(Value operand, evaluateExpression(unaryExpression.expression, &operand), "evaluating the operand of a unary not expression");
			TRY_LET(bool* boolean, getBoolean(operand, &boolean), "getting the boolean value in a unary not expression");
			return booleanValue(!*boolean, output);
		}
		case UNARY_OPERATION_INDEX: {
			TRY_LET(Value operand, evaluateExpression(unaryExpression.expression, &operand), "evaluating the operand of a unary not expression");
			TRY_LET(Value index, evaluateExpression(unaryExpression.operation.data.index, &index), "evaluating the index of an index expression");

			if (isString(index)) {
				UNWRAP_LET(String * string, getString(index, &string));
				TRY_LET(Value * field, getValueField(operand, *string, &field), "getting a field from an index expression");
				RETURN_OK(output, *field);
			}

			if (isNumber(index) && isList(operand)) {
				UNWRAP_LET(double* number, getNumber(index, &number));
				UNWRAP_LET(ValueList * list, getList(operand, &list));
				RETURN_OK(output, list->data[(int) *number - 1]);
			}

			RETURN_ERROR("Invalid index expression");
		}
	}
}

Result evaluateExpression(Expression expression, Value* output) {
	switch (expression.type) {
		case EXPRESSION_OBJECT: {
			return evaluateObject(*expression.data.object, output);
		}
		case EXPRESSION_IDENTIFIER: {
			TRY_LET(Value * result, getVariable(*CONTEXT->scope, expression.data.identifier, &result), "evaluating an identifier");
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

PRIVATE Result evaluateStatement(Statement statement) {
	if (isReturning) {
		return OK;
	}

	switch (statement.type) {
		case STATEMENT_EXPRESSION: {
			TRY_LET(Value value, evaluateExpression(statement.data.expression, &value), "evaluating an expression statement");
			return OK;
		}
		case STATEMENT_DECLARATION: {
			TRY_LET(Value value, evaluateExpression(statement.data.declaration.value, &value), "evaluating the value of a declaration");
			ScopeDeclaration declaration = (ScopeDeclaration) {
				.name = statement.data.declaration.name,
				.value = value,
			};
			TRY(setVariable(CONTEXT->scope, declaration), "declaring the variable \"%s\"", declaration.name);
			return OK;
		}
		case STATEMENT_RETURN: {
			TRY(evaluateExpression(statement.data.returnExpression, &returnValue), "evaluating the value of a return statement");
			isReturning = true;
			return OK;
		}
	}
}

Result run(Program program) {
	FOR_EACH(Statement statement, program.statements) {
		TRY(evaluateStatement(statement), "evaluating a top-level statement");
	}
	END;

	return OK;
}
