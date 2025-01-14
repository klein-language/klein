#include "../include/builtin.h"
#include "../include/context.h"
#include "../include/list.h"
#include "../include/parser.h"
#include "../include/result.h"
#include <string.h>

Result print(Context* context, List arguments) {
    if (arguments.size != 1) {
        return ERROR(ERROR_INVALID_ARGUMENTS, "Incorrect number of arguments passed to print");
    }

    Expression* expression = (Expression*) arguments.data[0];
    if (expression->type == EXPRESSION_IDENTIFIER) {
        expression = TRY(getVariable(*context->scope, expression->data.identifier));
    }

    if (expression->type != EXPRESSION_STRING) {
        return ERROR(ERROR_INVALID_ARGUMENTS, "Argument to print isn't a string");
    }

    printf("%s\n", expression->data.string);

    return ok(NULL);
}

Result input(Context* context, List arguments) {
    if (arguments.size != 0) {
        return ERROR(ERROR_INVALID_ARGUMENTS, "Incorrect number of arguments passed to input");
    }

    char* buffer;
    size_t length = 0;
    getline(&buffer, &length, stdin);

    return ok(HEAP(((Expression) {.type = EXPRESSION_STRING, .data = (ExpressionData) {.string = buffer}}), Expression));
}

Result getBuiltin(char* name) {
    if (strcmp(name, "print") == 0) {
        return ok(&print);
    }

    if (strcmp(name, "input") == 0) {
        return ok(&input);
    }

    return ERROR(ERROR_INTERNAL, "Unknown builtin: ", name);
}
