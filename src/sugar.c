#include "../include/sugar.h"
#include "../include/builtin.h"
#include "../include/parser.h"

KleinResult evaluateExpression(Expression expression, Value* output);

KleinResult stringValue(String string, Value* output) {

	// Internals
	InternalList internals = emptyInternalList();
	String* pointer = malloc(sizeof(String));
	*pointer = string;
	appendToInternalList(&internals, (Internal) {.key = INTERNAL_KEY_STRING, .value = pointer});

	// Fields
	ValueFieldList* fields = emptyHeapValueFieldList();

	// .length()
	BuiltinFunction function;
	TRY(getBuiltin("String.length", &function));
	TRY_LET(Value length, builtinFunctionToValue(function, &length));
	appendToValueFieldList(fields, (ValueField) {.name = "length", .value = length});

	// Create value
	Value value = (Value) {
		.fields = fields,
		.internals = internals,
	};

	RETURN_OK(output, value);
}

KleinResult getString(Value value, String** output) {
	return getValueInternal(value, INTERNAL_KEY_STRING, (void**) output);
}

bool isString(Value value) {
	String* output;
	return isOk(getString(value, &output));
}

KleinResult numberValue(double number, Value* output) {

	// Internals
	InternalList internals = emptyInternalList();
	double* heapValue = malloc(sizeof(double));
	*heapValue = number;
	appendToInternalList(&internals, (Internal) {.key = INTERNAL_KEY_NUMBER, .value = heapValue});

	// Fields
	ValueFieldList* fields = emptyHeapValueFieldList();

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
	TRY_LET(Expression parsed, parseKleinExpression(to, &parsed));
	TRY_LET(Value toValue, evaluateExpression(parsed, &toValue));
	appendToValueFieldList(fields, (ValueField) {.name = "to", .value = toValue});

	// .mod()
	BuiltinFunction function;
	TRY(getBuiltin("Number.mod", &function));
	TRY_LET(Value mod, builtinFunctionToValue(function, &mod));
	appendToValueFieldList(fields, (ValueField) {.name = "mod", .value = mod});

	// Create value
	Value value = (Value) {
		.fields = fields,
		.internals = internals,
	};
	RETURN_OK(output, value);
}

KleinResult getNumber(Value value, double** output) {
	return getValueInternal(value, INTERNAL_KEY_NUMBER, (void**) output);
}

bool isNumber(Value value) {
	double* output;
	return isOk(getNumber(value, &output));
}

KleinResult booleanValue(bool boolean, Value* output) {

	// Internals
	InternalList internals = emptyInternalList();
	bool* heapValue = malloc(sizeof(bool));
	*heapValue = boolean;
	appendToInternalList(&internals, (Internal) {.key = INTERNAL_KEY_BOOLEAN, .value = heapValue});

	// Fields
	ValueFieldList* fields = emptyHeapValueFieldList();

	// Create value
	Value value = (Value) {
		.fields = fields,
		.internals = internals,
	};
	RETURN_OK(output, value);
}

KleinResult getBoolean(Value value, bool** output) {
	return getValueInternal(value, INTERNAL_KEY_BOOLEAN, (void**) output);
}

bool isBoolean(Value value) {
	bool* output;
	return isOk(getBoolean(value, &output));
}

KleinResult listValue(ValueList values, Value* output) {

	// Internals
	InternalList internals = emptyInternalList();
	ValueList* heapValues = malloc(sizeof(ValueList));
	*heapValues = values;
	appendToInternalList(&internals, (Internal) {.key = INTERNAL_KEY_LIST, .value = heapValues});

	// Fields
	ValueFieldList* fields = emptyHeapValueFieldList();

	// .append()
	BuiltinFunction function;
	TRY(getBuiltin("List.append", &function));
	TRY_LET(Value append, builtinFunctionToValue(function, &append));
	appendToValueFieldList(fields, (ValueField) {.name = "append", .value = append});

	// Create value
	Value value = (Value) {
		.fields = fields,
		.internals = internals,
	};
	RETURN_OK(output, value);
}

KleinResult getList(Value value, ValueList** output) {
	return getValueInternal(value, INTERNAL_KEY_LIST, (void**) output);
}

bool isList(Value value) {
	ValueList* output;
	return isOk(getList(value, &output));
}

KleinResult nullValue(Value* output) {
	// Fields
	ValueFieldList* fields = emptyHeapValueFieldList();

	InternalList internals = emptyInternalList();
	appendToInternalList(&internals, (Internal) {.key = INTERNAL_KEY_NULL, .value = NULL});

	Value result = (Value) {.fields = fields, .internals = internals};
	RETURN_OK(output, result);
}

KleinResult functionValue(Function value, Value* output) {
	// Fields
	ValueFieldList* fields = emptyHeapValueFieldList();

	InternalList internals = emptyInternalList();

	Function* function = malloc(sizeof(Function));
	*function = value;
	appendToInternalList(&internals, (Internal) {.key = INTERNAL_KEY_FUNCTION, .value = function});

	Value result = (Value) {.fields = fields, .internals = internals};
	RETURN_OK(output, result);
}

KleinResult getFunction(Value value, Function** output) {
	return getValueInternal(value, INTERNAL_KEY_FUNCTION, (void**) output);
}

bool isBuiltinFunction(Value value) {
	return hasInternal(value, INTERNAL_KEY_BUILTIN_FUNCTION);
}

bool isNull(Value value) {
	return hasInternal(value, INTERNAL_KEY_NULL);
}
