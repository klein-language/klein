#ifndef PARSER_H
#define PARSER_H

#include "../bindings/c/klein.h"
#include "util.h"

bool hasInternal(Value value, InternalKey key);
KleinResult getValueInternal(Value value, InternalKey key, void** output);
KleinResult getValueField(Value value, String name, Value** output);

#endif
