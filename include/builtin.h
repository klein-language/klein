#ifndef BUILTIN_H
#define BUILTIN_H

#include "parser.h"
#include "result.h"
#include "util.h"

typedef KleinResult (*BuiltinFunction)(ValueList*, Value*);

KleinResult getBuiltin(String name, BuiltinFunction* output);
KleinResult builtinFunctionToValue(BuiltinFunction function, Value* output);
KleinResult valuesAreEqual(Value left, Value right, Value* output);
KleinResult valueToString(Value left, String* output);

#endif
