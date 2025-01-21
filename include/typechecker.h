#ifndef TYPECHECKER_H
#define TYPECHECKER_H

#include "list.h"

typedef struct Function Function;
typedef struct TypeDeclaration TypeDeclaration;

typedef struct Expression Expression;
Result typeOf(Expression expression, Type* output);

#endif
