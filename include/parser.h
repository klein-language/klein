#ifndef PARSER_H
#define PARSER_H

#include "../bindings/c/klein.h"
#include "util.h"

bool hasInternal(Value value, InternalKey key);
Result getValueInternal(Value value, InternalKey key, void** output);
Result getValueField(Value value, String name, Value** output);

#endif
