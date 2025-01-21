#include "../include/sugar.h"
#include "../include/builtin.h"
#include "../include/parser.h"

Result evaluateExpression(Expression expression, Value* output);

Result stringValue(String string, Value* output) {

	// Internals
	InternalList internals;
	TRY(emptyInternalList(&internals), "creating the internal field list for a string expression");
	String* pointer = malloc(sizeof(String));
	*pointer = string;
	TRY(appendToInternalList(&internals, (Internal) {.key = INTERNAL_KEY_STRING, .value = pointer}), "appending a string's value to its internal fields");

	// Fields
	ValueFieldList* fields;
	TRY(emptyHeapValueFieldList(&fields), "creating the field list for a string expression");

	// .length()
	BuiltinFunction function;
	TRY(getBuiltin("String.length", &function), "getting the built-in function String.length()");
	TRY_LET(Value length, builtinFunctionToValue(function, &length), "converting a builtin function to a value");
	TRY(appendToValueFieldList(fields, (ValueField) {.name = "length", .value = length}), "appending the function String.length() to a string's field list");

	// Create value
	Value value = (Value) {
		.fields = fields,
		.internals = internals,
	};

	RETURN_OK(output, value);
}

Result getString(Value value, String** output) {
	return getValueInternal(value, INTERNAL_KEY_STRING, (void**) output);
}

bool isString(Value value) {
	String* output;
	return isOk(getString(value, &output));
}

Result numberValue(double number, Value* output) {

	// Internals
	InternalList internals;
	TRY(emptyInternalList(&internals), "creating the internal field list for a number expression");
	double* heapValue = malloc(sizeof(double));
	*heapValue = number;
	TRY(appendToInternalList(&internals, (Internal) {.key = INTERNAL_KEY_NUMBER, .value = heapValue}), "appending a number's value to its internal field list");

	// Fields
	ValueFieldList* fields;
	TRY(emptyHeapValueFieldList(&fields), "creating the field list for a number expression");

	// .to()
	String to = ""
				"function(low: Number, high: Number): List {"
				"    let numbers = [];"
				"    let current = low;"
				"    while current <= high {"
				"        numbers.append(current);"
				"        current = current + 1;"
				"    };"
				"    return numbers;"
				"}";
	TRY_LET(Expression parsed, parseKleinExpression(to, &parsed), "parsing Number.to()");
	TRY_LET(Value toValue, evaluateExpression(parsed, &toValue), "evaluating Number.to()");
	TRY(appendToValueFieldList(fields, (ValueField) {.name = "to", .value = toValue}), "appending the field Number.to() to a number's field list");

	// .mod()
	BuiltinFunction function;
	TRY(getBuiltin("Number.mod", &function), "getting the builtin function Number.mod()");
	TRY_LET(Value mod, builtinFunctionToValue(function, &mod), "converting the built-in function Number.mod() to a value");
	TRY(appendToValueFieldList(fields, (ValueField) {.name = "mod", .value = mod}), "appending the field Number.mod() to a number's field list");

	// Create value
	Value value = (Value) {
		.fields = fields,
		.internals = internals,
	};
	RETURN_OK(output, value);
}

Result getNumber(Value value, double** output) {
	return getValueInternal(value, INTERNAL_KEY_NUMBER, (void**) output);
}

bool isNumber(Value value) {
	double* output;
	return isOk(getNumber(value, &output));
}

Result booleanValue(bool boolean, Value* output) {

	// Internals
	InternalList internals;
	TRY(emptyInternalList(&internals), "creating the internal field list for a boolean expression");
	bool* heapValue = malloc(sizeof(bool));
	*heapValue = boolean;
	TRY(appendToInternalList(&internals, (Internal) {.key = INTERNAL_KEY_BOOLEAN, .value = heapValue}), "appending the boolean value of a boolean expression to its internal fields");

	// Fields
	ValueFieldList* fields;
	TRY(emptyHeapValueFieldList(&fields), "creating the field list for a boolean expression");

	// Create value
	Value value = (Value) {
		.fields = fields,
		.internals = internals,
	};
	RETURN_OK(output, value);
}

Result getBoolean(Value value, bool** output) {
	return getValueInternal(value, INTERNAL_KEY_BOOLEAN, (void**) output);
}

bool isBoolean(Value value) {
	bool* output;
	return isOk(getBoolean(value, &output));
}

Result listValue(ValueList values, Value* output) {

	// Internals
	InternalList internals;
	TRY(emptyInternalList(&internals), "creating the internal field list for a list expression");
	ValueList* heapValues = malloc(sizeof(ValueList));
	*heapValues = values;
	TRY(appendToInternalList(&internals, (Internal) {.key = INTERNAL_KEY_LIST, .value = heapValues}), "appending the internal list elements to a list expression's internal field list");

	// Fields
	ValueFieldList* fields;
	TRY(emptyHeapValueFieldList(&fields), "creating the field list for a list expression");

	// .append()
	BuiltinFunction function;
	TRY(getBuiltin("List.append", &function), "getting the builtin function List.append()");
	TRY_LET(Value append, builtinFunctionToValue(function, &append), "converting the builtin function List.append() to a value");
	TRY(appendToValueFieldList(fields, (ValueField) {.name = "append", .value = append}), "appending the function List.append() to a list's field list");

	// Create value
	Value value = (Value) {
		.fields = fields,
		.internals = internals,
	};
	RETURN_OK(output, value);
}

Result getList(Value value, ValueList** output) {
	return getValueInternal(value, INTERNAL_KEY_LIST, (void**) output);
}

bool isList(Value value) {
	ValueList* output;
	return isOk(getList(value, &output));
}

Result nullValue(Value* output) {
	// Fields
	ValueFieldList* fields;
	TRY(emptyHeapValueFieldList(&fields), "creating the field list for a null value");

	InternalList internals;
	TRY(emptyInternalList(&internals), "creating null's internal list");
	TRY(appendToInternalList(&internals, (Internal) {.key = INTERNAL_KEY_NULL, .value = NULL}), "appending to null's internal list");

	Value result = (Value) {.fields = fields, .internals = internals};
	RETURN_OK(output, result);
}

Result functionValue(Function value, Value* output) {
	// Fields
	ValueFieldList* fields;
	TRY(emptyHeapValueFieldList(&fields), "creating the field list for a function value");

	InternalList internals;
	TRY(emptyInternalList(&internals), "creating a function value's internal list");

	Function* function = malloc(sizeof(Function));
	*function = value;
	TRY(appendToInternalList(&internals, (Internal) {.key = INTERNAL_KEY_FUNCTION, .value = function}), "appending to a function value's internal list");

	Value result = (Value) {.fields = fields, .internals = internals};
	RETURN_OK(output, result);
}

Result getFunction(Value value, Function** output) {
	return getValueInternal(value, INTERNAL_KEY_FUNCTION, (void**) output);
}

bool isBuiltinFunction(Value value) {
	return hasInternal(value, INTERNAL_KEY_BUILTIN_FUNCTION);
}

bool isNull(Value value) {
	return hasInternal(value, INTERNAL_KEY_NULL);
}
