#ifndef BUILTIN_H
#define BUILTIN_H

#include "parser.h"
#include "result.h"
#include "util.h"

typedef Result (*BuiltinFunction)(ValueList*, Value*);

Result getBuiltin(String name, BuiltinFunction* output);
Result builtinFunctionToValue(BuiltinFunction function, Value* output);
Result valuesAreEqual(Value left, Value right, Value* output);
Result valueToString(Value left, String* output);

#endif
