#include "../include/result.h"
#include "../include/string.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
void* unwrapUnsafe(Result result) {
    return result.data.data;
}

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
Result ok(void* data) {
    return (Result) {
        .success = true,
        .data = (ResultData) {
            .data = data,
        },
    };
}

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
Result error(char* message, ...) {
    // Set up varargs
    va_list args;
    va_start(args, message);

    // Get total length
    char* string = message;
    int totalLength = 0;
    while (string != NULL) {
        totalLength += strlen(string);
        string = va_arg(args, char*);
    }

    // Allocate result
    char* result = malloc(totalLength + 1);

    // Restart varargs
    va_end(args);
    va_start(args, message);

    // Build string
    string = message;
    result[0] = '\0';
    while (string != NULL) {
        strncat(result, string, strlen(string));
        string = va_arg(args, char*);
    }
    result[totalLength] = '\0';

    // Clean up varargs
    va_end(args);

    // Return
    return (Result) {
        .success = false,
        .data = (ResultData) {
            .errorMessage = result,
        },
    };
}
