#ifndef BUILTIN_H
#define BUILTIN_H

#include "result.h"
#include "util.h"

struct Expression;
struct ExpressionList;
struct Context;

typedef Result (*BuiltinFunction)(struct Context*, struct ExpressionList*, struct Expression*);

Result getBuiltin(String name, BuiltinFunction* output);

#endif
