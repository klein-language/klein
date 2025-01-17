#include "../include/context.h"
#include "../include/io.h"
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/result.h"
#include "../include/runner.h"
#include "../include/stdlib.h"
#include "../include/util.h"
#include <stdio.h>
#include <string.h>

/**
 * Runs the given Klein file.
 *
 * # Parameters
 *
 * - `filePath` - The path to the klein file as a null-terminated string.
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
PRIVATE Result runFile(String filePath) {
	char* dot = strrchr(filePath, '.');
	if (!dot || strcmp(dot, ".klein")) {
		fprintf(stderr, "\n%s Attempting to run a file that doesn't end with %s. Continue?: ", STYLE("Warning: ", YELLOW, BOLD), COLOR(".klein", CYAN));
		String response = input();
		if (!(strcmp(response, "Y") && strcmp(response, "y") && strcmp(response, "yes") && strcmp(response, "Yes") && strcmp(response, "YES"))) {
			fprintf(stderr, "\n%s\n\n", STYLE("Cancelling.", RED, BOLD));
			exit(1);
		}
		fprintf(stderr, "\n");
	}

	// Read source code
	TRY_LET(String, rawSourceCode, readFile, filePath);

	// Stdlib
	String sourceCode = malloc(strlen(rawSourceCode) + strlen(STDLIB) + 1);
	ASSERT_NONNULL(sourceCode);
	strcpy(sourceCode, STDLIB);

	// Combine
	strcat(sourceCode, rawSourceCode);
	free(rawSourceCode);

	// Tokenize
	TRY_LET(TokenList, tokens, tokenize, sourceCode);
	free(sourceCode);

	// Context
	Context context;
	TRY(newContext(&context));

	// Parse
	TRY_LET(Program, program, parse, &context, &tokens);

	// Run
	TRY(run(&context, program));

	// Cleanup
	freeProgram(program);
	freeContext(context);

	// Done
	return OK;
}

/**
 * Wrapper around `main` that returns a `Result` instead of an `int`.
 *
 * More specifically, main wraps *this* function, and converts the `Result` into
 * an exit code. This handles the main logic of the program.
 */
PRIVATE Result mainWrapper(int numberOfArguments, char** arguments) {
	if (numberOfArguments == 1) {
		printHelp(false);
		return OK;
	};

	return runFile(arguments[1]);
}

/**
 * The program's main entry point. Calls `mainWrapper` and converts the result into an exit
 * code.
 */
int main(int numberOfArguments, char** arguments) {
	Result attempt = mainWrapper(numberOfArguments, arguments);

	if (attempt != OK) {
		fprintf(stderr, "%s %s\n\n", STYLE("Error:", RED, BOLD), errorMessage(attempt));
	}

	return (int) attempt;
}
