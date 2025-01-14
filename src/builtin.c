#include "../include/builtin.h"
#include "../include/list.h"
#include "../include/parser.h"
#include <string.h>

Result print(List arguments) {
    if (arguments.size != 1) {
        return ERROR(ERROR_INVALID_ARGUMENTS, "Incorrect number of arguments passed to print");
    }

    if (((Expression*) arguments.data[0])->type != EXPRESSION_STRING) {
        return ERROR(ERROR_INVALID_ARGUMENTS, "Argument to print isn't a string");
    }

    printf("%s\n", ((Expression*) arguments.data[0])->data.string);

    return ok(NULL);
}

Result getBuiltin(char* name) {
    if (strcmp(name, "print") == 0) {
        return ok(&print);
    }

    return ERROR(ERROR_INTERNAL, "Unknown builtin: ", name);
}
