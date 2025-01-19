#ifndef RESULT_H
#define RESULT_H

#include "util.h"
#include <stdbool.h>
#include <stdio.h>

typedef struct {
	char* errorMessage;
} Result;

bool isOk(Result result);
bool isError(Result result);
Result error(String message);

extern Result OK;

#define TRY(expression__)                     \
	do {                                      \
		Result attempt__ = expression__;      \
		if (attempt__.errorMessage != NULL) { \
			return attempt__;                 \
		}                                     \
	} while (false)

#define UNWRAP(expression__)                                                \
	do {                                                                    \
		Result attempt__ = expression__;                                    \
		if (attempt__.errorMessage != NULL) {                               \
			fprintf(stderr, "Error: Attempted to unwrap an error value\n"); \
			exit(1);                                                        \
		}                                                                   \
	} while (false)

#define TRY_LET(__type, __output, __expression, ...) \
	__type __output;                                 \
	TRY(__expression(__VA_ARGS__, &__output))

#define UNWRAP_LET(__type, __output, __expression, ...) \
	__type __output;                                    \
	UNWRAP(__expression(__VA_ARGS__, &__output))

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

#define RETURN_ERROR(...)                                     \
	do {                                                      \
		int size__ = snprintf(NULL, 0, __VA_ARGS__);          \
		String buffer__ = malloc((unsigned long) size__ + 1); \
		sprintf(buffer__, __VA_ARGS__);                       \
		return error(buffer__);                               \
	} while (false)

#endif
