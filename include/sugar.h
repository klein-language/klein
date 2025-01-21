#ifndef SUGAR_H
#define SUGAR_H

#include "parser.h"
#include "result.h"

KleinResult stringValue(String value, Value* output);
KleinResult getString(Value value, String** output);
bool isString(Value value);

KleinResult numberValue(double value, Value* output);
KleinResult getNumber(Value value, double** output);
bool isNumber(Value value);

KleinResult listValue(ValueList values, Value* output);
KleinResult getList(Value value, ValueList** output);
bool isList(Value value);

KleinResult booleanValue(bool value, Value* output);
KleinResult getBoolean(Value value, bool** output);
bool isBoolean(Value vaue);

KleinResult functionValue(Function value, Value* output);
KleinResult getFunction(Value value, Function** output);
bool isBuiltinFunction(Value value);

KleinResult nullValue(Value* output);
bool isNull(Value value);

#endif
