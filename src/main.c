#include "../include/context.h"
#include "../include/io.h"
#include "../include/lexer.h"
#include "../include/list.h"
#include "../include/parser.h"
#include "../include/result.h"
#include "../include/runner.h"
#include "../include/util.h"
#include <stdio.h>

/**
 * Runs the given Micro file.
 *
 * # Parameters
 *
 * - `filePath` - The path to the micro file as a null-terminated string.
 *
 * # Returns
 *
 * `NULL`
 *
 * # Errors
 *
 * If an error occurred during tokenization, parsing, or running. For specific documentation
 * on possible errors, see:
 *
 * - `tokenize()` from `lexer.h`
 * - `parse()` from `parser.h`
 * - `evaluate()` from `runner.h`
 */
Result runFile(char* filePath) {
    char* sourceCode = TRY(readFile(filePath));
    List tokens = FREE(TRY(tokenize(sourceCode)), List);
    free(sourceCode);
    Token** tokenStart = (Token**) tokens.data;
    Context context = FREE(TRY(newContext()), Context);
    Program program = FREE(TRY(parse(&context, &tokens)), Program);
    free(tokenStart);
    TRY(run(&context, program));
    for (int statementNumber = 0; statementNumber < program.statements.size; statementNumber++) {
        freeStatement(program.statements.data[statementNumber]);
    }
    free(program.statements.data);
    freeContext(&context);
    return ok(NULL);
}

/**
 * Wrapper around `main` that returns a `Result` instead of an `int`.
 *
 * More specifically, main wraps *this* function, and converts the `Result` into
 * an exit code. This handles the main logic of the program.
 */
Result mainWrapper(int numberOfArguments, char** arguments) {
    if (numberOfArguments != 2) {
        return ERROR(ERROR_BAD_COMMAND, "One argument expected.");
    }

    return runFile(arguments[1]);
}

/**
 * The program's main entry point. Calls `mainWrapper` and converts the result into an exit
 * code.
 */
int main(int numberOfArguments, char** arguments) {
    Result attempt = mainWrapper(numberOfArguments, arguments);

    if (!attempt.success) {
        fprintf(stderr, "Error: %s\n", attempt.data.errorMessage);
        free(attempt.data.errorMessage);
        return 1;
    }

    return 0;
}
