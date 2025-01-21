#ifndef RUNNER_H
#define RUNNER_H

#include "parser.h"
#include "result.h"

KleinResult evaluateExpression(Expression expression, Value* output);
KleinResult run(Program program);

#endif
