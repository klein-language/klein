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

#ifndef STYLE
#define STYLE(text, color, style) "\033[" style ";3" color "m" text "\033[0m"
#define COLOR(text, color) STYLE(text, color, NORMAL)

#define RED "1"
#define GREEN "2"
#define YELLOW "3"
#define BLUE "4"
#define PURPLE "5"
#define CYAN "6"
#define WHITE "7"

#define NORMAL "0"
#define BOLD "1"
#define UNDERLINE "4"
#endif

#define FORMAT(output__, ...)                                 \
	do {                                                      \
		int size__ = snprintf(NULL, 0, __VA_ARGS__);          \
		String buffer__ = malloc((unsigned long) size__ + 1); \
		sprintf(buffer__, __VA_ARGS__);                       \
		output__ = buffer__;                                  \
	} while (0)

#define MAX(x, y) (((x) >= (y)) ? (x) : (y))
#define MIN(x, y) (((x) <= (y)) ? (x) : (y))

void debug(String message);

typedef struct Context Context;
extern Context* CONTEXT;

#endif
