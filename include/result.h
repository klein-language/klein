#ifndef RESULT_H
#define RESULT_H

#include "util.h"
#include <stdbool.h>
#include <stdio.h>

typedef enum {
	OK,
	ERROR_NULL,
	ERROR_PRINT,
	ERROR_DUPLICATE_VARIABLE,
	ERROR_INVALID_ARGUMENTS,
	ERROR_CALL_NON_FUNCTION,
	ERROR_REFERENCE_NONEXISTENT_VARIABLE,
	ERROR_UNEXPECTED_TOKEN,
	ERROR_UNRECOGNIZED_TOKEN,
	ERROR_INVALID_OPERAND,
	ERROR_INTERNAL,
} Result;

String errorMessage(Result error);

#define TRY(expression__)         \
	do {                          \
		Result attempt__;         \
		attempt__ = expression__; \
		if (attempt__ != OK) {    \
			return attempt__;     \
		}                         \
	} while (false)

#define UNWRAP(expression__)                                                \
	do {                                                                    \
		Result attempt__;                                                   \
		attempt__ = expression__;                                           \
		if (attempt__ != OK) {                                              \
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
#define ASSERT_NONNULL(expression__) \
	do {                             \
		void* memory;                \
		memory = expression__;       \
		if (memory == NULL) {        \
			return ERROR_NULL;       \
		}                            \
	} while (false)

#define RETURN_OK(__output, __expression) \
	do {                                  \
		*output = __expression;           \
		return OK;                        \
	} while (false)

#define UNREACHABLE return ERROR_INTERNAL

#endif
