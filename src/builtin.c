#include "../include/builtin.h"
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
PRIVATE Result input(ValueList* arguments, Value* output) {
	if (arguments->size > 1) {
		RETURN_ERROR("Too many arguments passed to builtin function input(): Expected 0-1 but found %lu", arguments->size);
	}

	String defaultPrompt = "";
	String* prompt = &defaultPrompt;
	if (arguments->size == 1) {
		TRY(getString(arguments->data[0], &prompt), "getting the prompt for a call to input()");
	};

	// Print prompt
	printf("%s", *prompt);
	fflush(stdout);

	// Read from stdin
	String buffer = NULL;
	size_t length = 0;
	if (getline(&buffer, &length, stdin) < 0) {
		RETURN_ERROR("The system failed to read from stdin.");
	}
	buffer[strlen(buffer) - 1] = '\0';

	TRY_LET(Value result, stringValue(buffer, &result), "creating the return value from input()");

	RETURN_OK(output, result);
}

PRIVATE Result stringLength(ValueList* arguments, Value* output) {
	if (arguments->size != 1) {
		RETURN_ERROR("Incorrect number of arguments passed to builtin function String.length(): Expected 1 but found %lu", arguments->size);
	}

	String* stringValue;
	TRY(getString(arguments->data[0], &stringValue), "getting the string value passed to String.length()");

	TRY_LET(Value number, numberValue(strlen(*stringValue), &number), "creating the number value to return from String.length()");

	RETURN_OK(output, number);
}

PRIVATE Result listAppend(ValueList* arguments, Value* output) {
	SUPPRESS_UNUSED(output);

	if (arguments->size != 2) {
		RETURN_ERROR("Incorrect number of arguments passed to builtin function List.append(): Expected 2 but found %lu", arguments->size);
	}

	TRY_LET(ValueList * elements, getList(arguments->data[0], &elements), "getting the elements of the list passed to List.append()");

	TRY(appendToValueList(elements, arguments->data[1]), "appending the value in List.append()");

	return OK;
}

Result valueToString(Value value, String* output) {
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
		StringList strings;
		TRY(emptyStringList(&strings), "creating an empty string list when converting a list to a string");
		TRY(appendToStringList(&strings, "["), "appending to a string list when converting a list to a string");
		UNWRAP_LET(ValueList * elements, getList(value, &elements));
		size_t length = 0;
		FOR_EACHP(Value element, elements) {
			TRY_LET(String string, valueToString(element, &string), "converting a list element to a string");
			TRY(appendToStringList(&strings, string), "appending to a string list when converting a list element to a string");
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

	RETURN_ERROR("unimplemented toString()");
}

Result valuesAreEqual(Value left, Value right, Value* output) {
	if (isNumber(left) && isNumber(right)) {
		UNWRAP_LET(double* leftNumber, getNumber(left, &leftNumber));
		UNWRAP_LET(double* rightNumber, getNumber(right, &rightNumber));
		TRY_LET(Value result, booleanValue(*leftNumber == *rightNumber, &result), "creating the boolean return value to equal()");
		RETURN_OK(output, result);
	}

	UNREACHABLE;
}

PRIVATE Result numberMod(ValueList* arguments, Value* output) {
	if (arguments->size != 2) {
		RETURN_ERROR("Incorrect number of arguments passed to builtin function Number.mod(): Expected 2 but found %lu", arguments->size);
	}

	TRY_LET(double* leftNumber, getNumber(arguments->data[0], &leftNumber), "getting the first number passed to Number.mod()");
	TRY_LET(double* rightNumber, getNumber(arguments->data[1], &rightNumber), "getting the second number passed to Number.mod()");

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
PRIVATE Result print(ValueList* arguments, Value* output) {
	SUPPRESS_UNUSED(output);

	if (arguments->size < 1 || arguments->size > 2) {
		RETURN_ERROR("Incorrect number of arguments passed to builtin function print: Expected 1-2 but found %lu", arguments->size);
	}

	// Default options
	Value defaultOptions;
	ValueFieldList* fields;
	TRY(emptyHeapValueFieldList(&fields), "creating the field list for the default options to print()");
	TRY_LET(Value trueValue, booleanValue(true, &trueValue), "creating the default value for the field \"newline\" in print()");
	TRY(appendToValueFieldList(fields, (ValueField) {.name = "newline", .value = trueValue}), "appending the field \"newline\" to the fields for print()");
	defaultOptions = (Value) {
		.fields = fields,
	};

	Value options;
	if (arguments->size == 2) {
		options = arguments->data[1];
	} else {
		options = defaultOptions;
	}

	TRY_LET(String stringValue, valueToString(arguments->data[0], &stringValue), "converting the argument passed to print() into a string");

	String newline = "\n";
	TRY_LET(Value * useNewline, getValueField(options, "newline", &useNewline), "getting the field \"newline\" on the options passed to print()");
	TRY_LET(bool* newlineBoolean, getBoolean(*useNewline, &newlineBoolean), "getting the boolean of the field newline on the options passed to print()");
	if (!*newlineBoolean) {
		newline = "";
	}

	if (printf("%s%s", stringValue, newline) < 0) {
		RETURN_ERROR("The system failed to print to stdout");
	}

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

	if (strcmp(name, "Number.mod") == 0) {
		RETURN_OK(output, &numberMod);
	}

	RETURN_ERROR("Attempted to get a built-in function called \"%s\", but no built-in with that name exists.", name);
}

Result builtinFunctionToValue(BuiltinFunction function, Value* output) {
	InternalList internals;
	TRY(emptyInternalList(&internals), "creating an builtin's internals list");
	TRY(appendToInternalList(&internals, (Internal) {.key = INTERNAL_KEY_BUILTIN_FUNCTION, .value = (void*) function}), "appending to a builtin function value's internals");

	ValueFieldList* fields;
	TRY(emptyHeapValueFieldList(&fields), "creating a builtin function value's fields");

	Value result = (Value) {.fields = fields, .internals = internals};
	RETURN_OK(output, result);
}
