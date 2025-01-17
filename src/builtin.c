#include "../include/builtin.h"
#include "../include/context.h"
#include "../include/parser.h"
#include "../include/result.h"
#include "../include/sugar.h"
#include <string.h>

/**
 * The built-in `print` function. This converts it's argument into a string and prints it,
 * ending with a trailing newline, and returning `null`.
 *
 * # Parameters
 *
 * - `context` - Data about the current part of the program being compiled. In this case,
 *   the current scope (stored on the `context` is used to resolve an identifier in the case
 *   that the given argument is an identifier.
 *
 * - `arguments` - The arguments passed to `print`, as an expression list.
 *
 * - `output` - Where to place the return value from `print`. In the case of `print`, nothing
 *   is returned, so this is unused. However, to conform to the `BuiltinFunction` function pointer
 *   type and called as a generic return value from `getBuiltin()`, the parameter is included.
 *
 * # Errors
 *
 * If the given argument list doesnt contain exactly 1 argument, an error is returned.
 * If the given argument is an identifier that can't be resolved, an error is returned.
 * If the given argument isn't a string, an error is returned.
 * If the underlying `printf` call fails (for any reason), an error is returned.
 */
PRIVATE Result print(ExpressionList* arguments, Expression* output) {
	DEBUG_START("Evaluating", "builtin function call print()");
	SUPPRESS_UNUSED(output);

	if (arguments->size < 1) {
		return ERROR_INVALID_ARGUMENTS;
	}

	Object* options;

	// Default options
	Object defaultOptions;
	FieldList fields;
	TRY(emptyFieldList(&fields));
	Expression trueExpression = (Expression) {
		.type = EXPRESSION_BOOLEAN,
		.data = (ExpressionData) {
			.boolean = true,
		},
	};
	TRY(appendToFieldList(&fields, (Field) {.name = "newline", .value = trueExpression}));
	defaultOptions = (Object) {
		.fields = fields,
	};

	if (arguments->size == 2) {
		options = arguments->data[1].data.object;
	} else {
		options = &defaultOptions;
	}

	Expression* expression = arguments->data;
	if (expression->type == EXPRESSION_IDENTIFIER) {
		TRY(getVariable(*CONTEXT->scope, expression->data.identifier, &expression));
	}

	TRY_LET(String*, stringValue, getString, *expression);

	String newline = "\n";
	TRY_LET(Expression*, useNewline, getObjectField, *options, "newline");
	if (!useNewline->data.boolean) {
		newline = "";
	}

	if (printf("%s%s", *stringValue, newline) < 0) {
		return ERROR_PRINT;
	}

	return OK;
}

/**
 * The built-in `input` function. This reads a line from `stdin` and returns it as a string
 * expression.
 *
 * # Parameters
 *
 * - `context` - Data about the current part of the program being compiled. `input` doesn't
 *   actually need this parameter, so it's unused; It's included to conform to the `BuiltinFunction`
 *   function pointer type.
 *
 * - `arguments` - The arguments passed to `print`, as an expression list.
 *
 * - `output` - Where to place the return value from `input`. It will be a string expression,
 *   such as the return value from `stringExpression()` from `sugar.h`. The underlying
 *   C string is allocated on the heap and valid until manually freed.
 *
 * # Errors
 *
 * If the given argument list isn't empty, an error is returned.
 * If the underlying `getline` call fails (for any reason), an error is returned.
 * If creating the string expression fails for any reason, an error is returned.
 */
PRIVATE Result input(ExpressionList* arguments, Expression* output) {
	if (arguments->size > 1) {
		return ERROR_INVALID_ARGUMENTS;
	}

	String defaultPrompt = "";
	String* prompt = &defaultPrompt;
	if (arguments->size == 1) {
		TRY(getString(arguments->data[0], &prompt));
	};

	// Print prompt
	printf("%s", *prompt);
	fflush(stdout);

	// Read from stdin
	String buffer = NULL;
	size_t length = 0;
	if (getline(&buffer, &length, stdin) < 0) {
		return ERROR_INTERNAL;
	}
	buffer[strlen(buffer) - 1] = '\0';

	Expression result;
	TRY(stringExpression(buffer, &result));

	RETURN_OK(output, result);
}

PRIVATE Result stringLength(ExpressionList* arguments, Expression* output) {
	if (arguments->size != 1) {
		return ERROR_INVALID_ARGUMENTS;
	}

	Expression* expression = arguments->data;
	if (expression->type == EXPRESSION_IDENTIFIER) {
		TRY(getVariable(*CONTEXT->scope, expression->data.identifier, &expression));
	}

	String* stringValue;
	TRY(getString(*expression, &stringValue));

	Expression number;
	TRY(numberExpression((double) strlen(*stringValue), &number));

	RETURN_OK(output, number);
}

PRIVATE Result listAppend(ExpressionList* arguments, Expression* output) {
	if (arguments->size != 2) {
		return ERROR_INVALID_ARGUMENTS;
	}

	Expression list = arguments->data[0];
	TRY_LET(ExpressionList*, elements, getList, list);

	TRY(appendToExpressionList(elements, arguments->data[1]));

	return OK;
}

Result getBuiltin(String name, BuiltinFunction* output) {
	if (strcmp(name, "print") == 0) {
		RETURN_OK(output, &print);
	}

	if (strcmp(name, "input") == 0) {
		RETURN_OK(output, &input);
	}

	if (strcmp(name, "String.length") == 0) {
		RETURN_OK(output, &stringLength);
	}

	if (strcmp(name, "List.append") == 0) {
		RETURN_OK(output, &listAppend);
	}

	return ERROR_INTERNAL;
}
