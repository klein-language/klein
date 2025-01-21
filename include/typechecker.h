#ifndef TYPECHECKER_H
#define TYPECHECKER_H

#include "../bindings/c/klein.h"

KleinResult typeOf(Expression expression, Type* output);

#endif
