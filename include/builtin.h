#ifndef BUILTIN_H
#define BUILTIN_H

#include "result.h"
#include "util.h"

struct Expression;
struct ExpressionList;

typedef Result (*BuiltinFunction)(struct ExpressionList*, struct Expression*);

Result getBuiltin(String name, BuiltinFunction* output);
Result expressionsAreEqual(struct Expression left, struct Expression right, struct Expression* output);

#endif
