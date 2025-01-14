#ifndef RESULT_H
#define RESULT_H

#include <stdbool.h>
#include <stdio.h>

typedef enum {
    ERROR_NULL,
    ERROR_DUPLICATE_VARIABLE,
    ERROR_INVALID_ARGUMENTS,
    ERROR_CALL_NON_FUNCTION,
    ERROR_REFERENCE_NONEXISTENT_VARIABLE,
    ERROR_UNEXPECTED_TOKEN,
    ERROR_UNRECOGNIZED_TOKEN,
    ERROR_BAD_COMMAND,
    ERROR_INVALID_OPERAND,
    ERROR_INTERNAL,
} Error;

typedef union {
    void* data;
    char* errorMessage;
} ResultData;

typedef struct {
    bool success;
    ResultData data;
} Result;

/**
 * Creates a success `Result` with the given data.
 *
 * # Parameters
 *
 * - `data` - The data inside the success result.
 *
 * # Returns
 *
 * The success `Result`. The result is valid for the lifetime of the given
 * data pointer.
 */
Result ok(void* data);

/**
 * Creates an error `Result` with the given error message.
 *
 * **The last argument passed must be `NULL`, due to a quirk with varargs.**
 *
 * # Parameters
 *
 * - `message` - The error message.
 *
 * # Returns
 *
 * The error `Result`.
 */
Result error(char* message, ...);

/**
 * Unwraps the given result into its inner data value.
 *
 * # Returns
 *
 * The data stored in the result.
 *
 * # Safety
 *
 * If the given `Result` is an error value, the behavior of this function is
 * undefined.
 */
void* unwrapUnsafe(Result result);

#define TRY(expression)              \
    ({                               \
        Result attempt = expression; \
        if (!attempt.success) {      \
            return attempt;          \
        }                            \
        attempt.data.data;           \
    })

/**
 * Asserts that the given expression must be non-NULL. If it is `NULL`, an error
 * `Result` is returned from the current function.
 */
#define NONNULL(expression__)                                               \
    ({                                                                      \
        void* memory__ = expression__;                                      \
        if (memory__ == NULL) {                                             \
            return ERROR(ERROR_NULL, "Expected null value to be non-null"); \
        }                                                                   \
        memory__;                                                           \
    })

#define ERROR(code, ...) ({                                   \
    char target[10];                                          \
    sprintf(target, "%d", code);                              \
    error("Error (code E", target, "): ", __VA_ARGS__, NULL); \
})

#endif
