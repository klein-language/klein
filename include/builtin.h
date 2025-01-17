#ifndef BUILTIN_H
#define BUILTIN_H

#include "result.h"
#include "util.h"

struct Expression;
struct ExpressionList;

typedef Result (*BuiltinFunction)(struct ExpressionList*, struct Expression*);

Result getBuiltin(String name, BuiltinFunction* output);

#endif
