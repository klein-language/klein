#include "../include/sugar.h"
#include "../include/builtin.h"
#include "../include/context.h"
#include "../include/parser.h"

/**
 * Converts a C String (null-terminated `char*`) into a Klein string literal.
 *
 * The opposite of this function is `getString()`, which converts a Klein string
 * literal expression back into a C string. In other words, `getString()` can be
 * used on the output of `stringExpression()` to get the original string back.
 *
 * # Parameters
 *
 * - `value` - The C string to convert. It must live at least as long as
 *   `output`, otherwise accessing output is undefined behavior.
 *
 * - `output`- The place to store the created Klein `Expression`. It is only
 *   valid as long as `value` is valid.
 *
 * # Errors
 *
 * If memory fails to allocate, an error is returned.
 * If built-in properties on Strings such as `length()` fail to be found, an
 * error is returned.
 */
Result stringExpression(String value, Expression* output) {

	// Internals
	InternalFieldList internals;
	TRY(emptyInternalFieldList(&internals));
	String* pointer = malloc(sizeof(String));
	*pointer = value;
	TRY(appendToInternalFieldList(&internals, (InternalField) {.name = "string_value", .value = pointer}));

	// Fields
	FieldList fields;
	TRY(emptyFieldList(&fields));

	// .length()
	BuiltinFunction function;
	TRY(getBuiltin("String.length", &function));
	Expression length = (Expression) {
		.type = EXPRESSION_BUILTIN_FUNCTION,
		.data = (ExpressionData) {
			.builtinFunction = (BuiltinFunctionExpression) {
				.function = function,
				.thisObject = NULL,
			},
		},
	};
	TRY(appendToFieldList(&fields, (Field) {.name = "length", .value = length}));

	// Create object
	Object* object = malloc(sizeof(Object));
	ASSERT_NONNULL(object);
	*object = (Object) {
		.internals = internals,
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
 * Takes a string value in Klein as an expression and retrieves the underlying
 * C string in it.
 *
 * # Parameters
 *
 * - `expression` - The string expression. If this is any expression other than
 *   a string, an error is returned.
 *
 * - `output` - The C string stored in the given expression. It points to the same
 *   string originally stored in the expression, most likely through `stringExpression()`,
 *   so it is only valid as long as that string is valid.
 */
Result getString(Expression expression, String** output) {
	if (expression.type != EXPRESSION_OBJECT) {
		DEBUG_ERROR("Attempted to convert a non-string to a string");
		return ERROR_INTERNAL;
	}

	return getObjectInternal(*expression.data.object, "string_value", (void**) output);
}

bool isString(Expression expression) {
	String* output;
	return getString(expression, &output) == OK;
}

bool isNumber(Expression expression) {
	double* output;
	return getNumber(expression, &output) == OK;
}

bool isList(Expression expression) {
	ExpressionList* output;
	return getList(expression, &output) == OK;
}

/**
 * Converts a C number (`double`) into a Klein number literal.
 *
 * The opposite of this function is `getNumber()`, which converts a Klein number
 * literal expression back into a double. In other words, `getNumber()` can be
 * used on the output of `numberExpression()` to get the original number back.
 *
 * The internal double will be stored on the heap, and the caller is responsible
 * for freeing it.
 *
 * This is the number equivalent to `stringExpression()`.
 *
 * # Parameters
 *
 * - `value` - The number to convert.
 *
 * - `output`- The place to store the created Klein `Expression`.
 *
 * # Errors
 *
 * If memory fails to allocate, an error is returned.
 * If built-in properties on numbers such as `abs()` fail to be found, an
 * error is returned.
 */
Result numberExpression(double value, Expression* output) {

	// Internals
	InternalFieldList internals;
	TRY(emptyInternalFieldList(&internals));
	double* heapValue = malloc(sizeof(double));
	*heapValue = value;
	TRY(appendToInternalFieldList(&internals, (InternalField) {.name = "number_value", .value = heapValue}));

	// Fields
	FieldList fields;
	TRY(emptyFieldList(&fields));

	// .to()
	Expression to = (Expression) {
		.type = EXPRESSION_IDENTIFIER,
		.data = (ExpressionData) {
			.identifier = "range",
		},
	};
	TRY(appendToFieldList(&fields, (Field) {.name = "to", .value = to}));

	// .mod()
	BuiltinFunction function;
	TRY(getBuiltin("Number.mod", &function));
	Expression mod = (Expression) {
		.type = EXPRESSION_BUILTIN_FUNCTION,
		.data = (ExpressionData) {
			.builtinFunction = (BuiltinFunctionExpression) {
				.function = function,
				.thisObject = NULL,
			},
		},
	};
	TRY(appendToFieldList(&fields, (Field) {.name = "mod", .value = mod}));

	// Create object
	Object* object = malloc(sizeof(Object));
	ASSERT_NONNULL(object);
	*object = (Object) {
		.internals = internals,
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
 * Takes a number in Klein as an expression and retrieves the underlying
 * double in it.
 *
 * # Parameters
 *
 * - `expression` - The number expression. If this is any expression other than
 *   a number, an error is returned.
 *
 * - `output` - The double stored in the given expression. It points to a double
 *   on the heap, so it's valid as long as the expression hasnt't explicitly
 *   freed it.
 */
Result getNumber(Expression expression, double** output) {
	if (expression.type != EXPRESSION_OBJECT) {
		DEBUG_ERROR("Attempted to convert a non-object to a number");
		return ERROR_INTERNAL;
	}

	return getObjectInternal(*expression.data.object, "number_value", (void**) output);
}

Result listExpression(ExpressionList values, Expression* output) {

	// Internals
	InternalFieldList internals;
	TRY(emptyInternalFieldList(&internals));
	ExpressionList* heapValues = malloc(sizeof(ExpressionList));
	*heapValues = values;
	TRY(appendToInternalFieldList(&internals, (InternalField) {.name = "elements", .value = heapValues}));

	// Fields
	FieldList fields;
	TRY(emptyFieldList(&fields));

	// .append()
	BuiltinFunction function;
	TRY(getBuiltin("List.append", &function));
	Expression append = (Expression) {
		.type = EXPRESSION_BUILTIN_FUNCTION,
		.data = (ExpressionData) {
			.builtinFunction = (BuiltinFunctionExpression) {
				.function = function,
				.thisObject = NULL,
			},
		},
	};
	TRY(appendToFieldList(&fields, (Field) {.name = "append", .value = append}));

	// Create object
	Object* object = malloc(sizeof(Object));
	ASSERT_NONNULL(object);
	*object = (Object) {
		.internals = internals,
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

Result getList(Expression expression, ExpressionList** output) {
	DEBUG_START("Getting", "internal list value");
	if (expression.type != EXPRESSION_OBJECT) {
		return ERROR_INTERNAL;
	}

	TRY(getObjectInternal(*expression.data.object, "elements", (void**) output));
	DEBUG_END("getting internal list value");
	return OK;
}

Result desugar(Expression expression, Expression* output) {
}
