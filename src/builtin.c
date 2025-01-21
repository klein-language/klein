#include "../include/builtin.h"
#include "../include/list.h"
#include "../include/parser.h"
#include "../include/result.h"
#include "../include/sugar.h"
#include <math.h>
#include <string.h>

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
PRIVATE KleinResult input(ValueList* arguments, Value* output) {
	if (arguments->size > 1) {
		return (KleinResult) {
			.type = KLEIN_ERROR_INCORRECT_ARGUMENT_COUNT,
			.data = (KleinResultData) {
				.incorrectArgumentCount = (KleinIncorrectArgumentCountError) {
					.expected = 1,
					.actual = arguments->size,
				},
			},
		};
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
	getline(&buffer, &length, stdin);
	buffer[strlen(buffer) - 1] = '\0';

	TRY_LET(Value result, stringValue(buffer, &result));

	RETURN_OK(output, result);
}

PRIVATE KleinResult stringLength(ValueList* arguments, Value* output) {
	if (arguments->size != 1) {
		return (KleinResult) {
			.type = KLEIN_ERROR_INCORRECT_ARGUMENT_COUNT,
			.data = (KleinResultData) {
				.incorrectArgumentCount = (KleinIncorrectArgumentCountError) {
					.expected = 1,
					.actual = arguments->size,
				},
			},
		};
	}

	TRY_LET(String * stringValue, getString(arguments->data[0], &stringValue));
	TRY_LET(Value number, numberValue(strlen(*stringValue), &number));

	RETURN_OK(output, number);
}

PRIVATE KleinResult listAppend(ValueList* arguments, Value* output) {
	SUPPRESS_UNUSED(output);

	if (arguments->size != 2) {
		return (KleinResult) {
			.type = KLEIN_ERROR_INCORRECT_ARGUMENT_COUNT,
			.data = (KleinResultData) {
				.incorrectArgumentCount = (KleinIncorrectArgumentCountError) {
					.expected = 2,
					.actual = arguments->size,
				},
			},
		};
	}

	TRY_LET(ValueList * elements, getList(arguments->data[0], &elements));
	appendToValueList(elements, arguments->data[1]);

	return OK;
}

KleinResult valueToString(Value value, String* output) {
	if (isNumber(value)) {
		UNWRAP_LET(double* number, getNumber(value, &number));

		// Integer
		if (floor(*number) == *number) {
			String result;
			FORMAT(result, "%d", (int) *number);
			RETURN_OK(output, result);
		}

		int len = snprintf(NULL, 0, "%f", *number);
		String result = malloc((unsigned long) len + 1);
		snprintf(result, (unsigned long) len + 1, "%f", *number);

		RETURN_OK(output, result);
	}

	if (isString(value)) {
		UNWRAP_LET(String * result, getString(value, &result));
		RETURN_OK(output, *result);
	}

	if (isList(value)) {
		StringList strings = emptyStringList();
		appendToStringList(&strings, "[");
		UNWRAP_LET(ValueList * elements, getList(value, &elements));
		size_t length = 0;
		FOR_EACHP(Value element, elements) {
			TRY_LET(String string, valueToString(element, &string));
			appendToStringList(&strings, string);
			length += strlen(string);
		}
		END;

		String result = malloc(sizeof(char) * (length + 1));
		*result = '\0';
		FOR_EACH(String element, strings) {
			strcat(result, element);
		}
		END;

		RETURN_OK(output, result);
	}

	if (isNull(value)) {
		RETURN_OK(output, "null");
	}

	UNREACHABLE;
}

KleinResult valuesAreEqual(Value left, Value right, Value* output) {
	if (isNumber(left) && isNumber(right)) {
		UNWRAP_LET(double* leftNumber, getNumber(left, &leftNumber));
		UNWRAP_LET(double* rightNumber, getNumber(right, &rightNumber));
		TRY_LET(Value result, booleanValue(*leftNumber == *rightNumber, &result));
		RETURN_OK(output, result);
	}

	UNREACHABLE;
}

PRIVATE KleinResult numberMod(ValueList* arguments, Value* output) {
	if (arguments->size != 2) {
		return (KleinResult) {
			.type = KLEIN_ERROR_INCORRECT_ARGUMENT_COUNT,
			.data = (KleinResultData) {
				.incorrectArgumentCount = (KleinIncorrectArgumentCountError) {
					.expected = 2,
					.actual = arguments->size,
				},
			},
		};
	}

	TRY_LET(double* leftNumber, getNumber(arguments->data[0], &leftNumber));
	TRY_LET(double* rightNumber, getNumber(arguments->data[1], &rightNumber));

	double result = (int) *leftNumber % (int) *rightNumber;
	return numberValue(result, output);
}

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
PRIVATE KleinResult print(ValueList* arguments, Value* output) {
	SUPPRESS_UNUSED(output);

	if (arguments->size < 1 || arguments->size > 2) {
		return (KleinResult) {
			.type = KLEIN_ERROR_INCORRECT_ARGUMENT_COUNT,
			.data = (KleinResultData) {
				.incorrectArgumentCount = (KleinIncorrectArgumentCountError) {
					.expected = 2,
					.actual = arguments->size,
				},
			},
		};
	}

	// Default options
	Value defaultOptions;
	ValueFieldList* fields = emptyHeapValueFieldList();
	TRY_LET(Value trueValue, booleanValue(true, &trueValue));
	appendToValueFieldList(fields, (ValueField) {.name = "newline", .value = trueValue});
	defaultOptions = (Value) {
		.fields = fields,
	};

	Value options;
	if (arguments->size == 2) {
		options = arguments->data[1];
	} else {
		options = defaultOptions;
	}

	TRY_LET(String stringValue, valueToString(arguments->data[0], &stringValue));

	String newline = "\n";
	TRY_LET(Value * useNewline, getValueField(options, "newline", &useNewline));
	TRY_LET(bool* newlineBoolean, getBoolean(*useNewline, &newlineBoolean));
	if (!*newlineBoolean) {
		newline = "";
	}

	printf("%s%s", stringValue, newline);

	return OK;
}

KleinResult getBuiltin(String name, BuiltinFunction* output) {
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

	if (strcmp(name, "Number.mod") == 0) {
		RETURN_OK(output, &numberMod);
	}

	UNREACHABLE;
}

KleinResult builtinFunctionToValue(BuiltinFunction function, Value* output) {
	InternalList internals = emptyInternalList();
	appendToInternalList(&internals, (Internal) {.key = INTERNAL_KEY_BUILTIN_FUNCTION, .value = (void*) function});

	ValueFieldList* fields = emptyHeapValueFieldList();

	Value result = (Value) {.fields = fields, .internals = internals};
	RETURN_OK(output, result);
}
