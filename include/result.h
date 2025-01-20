#ifndef RESULT_H
#define RESULT_H

#include "util.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef struct {
	char* errorMessage;
} Result;

bool isOk(Result result);
bool isError(Result result);
Result error(String message);

extern Result OK;

#define TRY(expression__, ...)                                                                                          \
	do {                                                                                                                \
		Result attempt__ = expression__;                                                                                \
		if (attempt__.errorMessage != NULL) {                                                                           \
			String errorMessage__;                                                                                      \
			FORMAT(errorMessage__, __VA_ARGS__);                                                                        \
			if (strcmp(errorMessage__, "") != 0) {                                                                      \
				FORMAT(attempt__.errorMessage, "%s\033[2m\n\twhile %s\033[0m", attempt__.errorMessage, errorMessage__); \
			}                                                                                                           \
			return attempt__;                                                                                           \
		}                                                                                                               \
	} while (false)

#define TRY_LET(declaration__, ...) \
	declaration__;                  \
	TRY(__VA_ARGS__)

#define UNWRAP_LET(declaration__, ...) \
	declaration__;                     \
	UNWRAP(__VA_ARGS__)

#define UNWRAP(expression__)                                                \
	do {                                                                    \
		Result attempt__ = expression__;                                    \
		if (attempt__.errorMessage != NULL) {                               \
			fprintf(stderr, "Error: Attempted to unwrap an error value\n"); \
			exit(1);                                                        \
		}                                                                   \
	} while (false)

/**
 * Asserts that the given expression must be non-NULL. If it is `NULL`, an error
 * `Result` is returned from the current function.
 */
#define ASSERT_NONNULL(expression__)                                \
	do {                                                            \
		void* memory;                                               \
		memory = expression__;                                      \
		if (memory == NULL) {                                       \
			return error("A value that mustn't be null was null."); \
		}                                                           \
	} while (false)

#define RETURN_OK(__output, __expression) \
	do {                                  \
		*output = __expression;           \
		return OK;                        \
	} while (false)

#define UNREACHABLE return error("Unreachable code was reached")

#define RETURN_ERROR(...)              \
	do {                               \
		String output__;               \
		FORMAT(output__, __VA_ARGS__); \
		return error(output__);        \
	} while (false)

#endif
