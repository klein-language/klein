#ifndef RESULT_H
#define RESULT_H

#include "../bindings/c/klein.h"
#include "util.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

bool isOk(KleinResult result);
bool isError(KleinResult result);

extern KleinResult OK;

#define TRY(expression__)                     \
	do {                                      \
		KleinResult attempt__ = expression__; \
		if (attempt__.type != KLEIN_OK) {     \
			return attempt__;                 \
		}                                     \
	} while (false)

#define TRY_LET(declaration__, expression__) \
	declaration__;                           \
	TRY(expression__)

#define UNWRAP_LET(declaration__, expression__) \
	declaration__;                              \
	UNWRAP(expression__)

#define UNWRAP(expression__)                                                \
	do {                                                                    \
		KleinResult attempt__ = expression__;                               \
		if (attempt__.type != KLEIN_OK) {                                   \
			fprintf(stderr, "Error: Attempted to unwrap an error value\n"); \
			exit(1);                                                        \
		}                                                                   \
	} while (false)

/**
 * Asserts that the given expression must be non-NULL. If it is `NULL`, an error
 * `Result` is returned from the current function.
 */
#define ASSERT_NONNULL(expression__)                         \
	do {                                                     \
		void* memory;                                        \
		memory = expression__;                               \
		if (memory == NULL) {                                \
			return (KleinResult) {.type = KLEIN_ERROR_NULL}; \
		}                                                    \
	} while (false)

#define RETURN_OK(__output, __expression) \
	do {                                  \
		*output = __expression;           \
		return OK;                        \
	} while (false)

#define UNREACHABLE \
	return (KleinResult) { .type = KLEIN_ERROR_INTERNAL }

#endif
