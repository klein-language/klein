#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * A typical String. Using `String` is equivalent to `char*`, but they
 * convey different intent. `char*` is meant to refer to a pointer to
 * a single character (or maybe character array in some situations),
 * whereas `String` refers specifically to a null-terminated character
 * array.
 */
typedef char* String;

/** Suppresses an "unused variable" warning.*/
#define SUPPRESS_UNUSED(variable) (void) variable

/**
 * Declares that a function isn't called outside of this compilation unit,
 * meaning it doesn't need a declaration in a header file.
 */
#define PRIVATE static

#ifdef DEBUG_ON
#define DEBUG(...) fprintf(stderr, __VA_ARGS__);
#else
#define DEBUG(message)
#endif

void debug(String message);

#endif
