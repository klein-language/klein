#ifndef RUNNER_H
#define RUNNER_H

#include "parser.h"
#include "result.h"

Result evaluateExpression(Expression expression, Value* output);
Result run(Program program);

#endif
