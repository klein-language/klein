#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>

/**
 * Frees the heap memory pointed to by `expression`, and returns the data that was stored
 * there as a stack value.
 *
 * # Parameters
 *
 * - `expression` - An expression returning a heap pointer to free.
 * - `type` - The type of data being allocated on the heap.
 *
 * # Returns
 *
 * The stack value.
 *
 * # Safety
 *
 * If the given expression isn't freeable, behavior is undefined.
 */
#define FREE(expression, type) ({ \
    type* pointer = expression;   \
    type value = *pointer;        \
    free(pointer);                \
    value;                        \
})

/**
 * Allocates space for the given type on the heap and stores the given value there,
 * returning a pointer to the value.
 */
#define HEAP(expression_, type_) ({                   \
    type_ value_ = (expression_);                     \
    type_* pointer_ = NONNULL(malloc(sizeof(type_))); \
    *pointer_ = value_;                               \
    pointer_;                                         \
})

#define LIBRARY __declspec(dllexport)

#endif
