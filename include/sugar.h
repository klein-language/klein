#ifndef SUGAR_H
#define SUGAR_H

#include "parser.h"
#include "result.h"

Result stringValue(String value, Value* output);
Result getString(Value value, String** output);
bool isString(Value value);

Result numberValue(double value, Value* output);
Result getNumber(Value value, double** output);
bool isNumber(Value value);

Result listValue(ValueList values, Value* output);
Result getList(Value value, ValueList** output);
bool isList(Value value);

Result booleanValue(bool value, Value* output);
Result getBoolean(Value value, bool** output);
bool isBoolean(Value vaue);

Result functionValue(Function value, Value* output);
Result getFunction(Value value, Function** output);
bool isBuiltinFunction(Value value);

Result nullValue(Value* output);
bool isNull(Value value);

#endif
