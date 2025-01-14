#include "../include/list.h"
#include "../include/result.h"
#include <stdlib.h>

Result emptyHeapList() {
    List* list = NONNULL(malloc(sizeof(List)));
    list->size = 0;
    list->capacity = 8;
    list->data = NONNULL(malloc(sizeof(void*) * list->capacity));
    return ok(list);
}

Result emptyList(List* output) {
    output->size = 0;
    output->capacity = 8;
    output->data = NONNULL(malloc(sizeof(void*) * output->capacity));
    return ok(NULL);
}

/**
 * Appends a value to the end of a list. If the list is full (`size == capacity`),
 * it's capacity will be doubled.
 *
 * # Parameters
 *
 * - `list` - The list to append to
 * - `value` - The value to append, which may be `NULL`.
 *
 * # Returns
 *
 * `NULL`, wrapped in a `Result`, assuming success.
 *
 * # Errors
 *
 * If `list` or is `NULL`, an error is returned. If memory can't be
 * allocated, an error is returned.
 *
 * # Safety
 *
 * If the lifetime of the `list` pointer outlives the lifetime of the `value` pointer,
 * the `list` will be left with a dangling pointer. In other words, when calling this
 * function, `value` must live *at least as long as* `list`, otherwise behavior is
 * undefined.
 */
Result appendToList(List* list, void* value) {
    if (list == NULL) {
        return ERROR(ERROR_INTERNAL, "Attempted to append to a null list");
    }

    if (list->size == list->capacity) {
        list->capacity *= 2;
        list->data = NONNULL(realloc(list->data, sizeof(void*) * list->capacity));
    }

    list->data[list->size] = value;
    list->size++;

    return ok(NULL);
}

Result prependToList(List* list, void* value) {
    if (list == NULL) {
        return ERROR(ERROR_INTERNAL, "Attempted to prepend to a null list");
    }

    if (list->size == list->capacity) {
        list->capacity *= 2;
        list->data = NONNULL(realloc(list->data, sizeof(void*) * list->capacity));
    }

    for (int index = list->size; index > 0; index--) {
        list->data[index] = list->data[index - 1];
    }

    list->data[0] = value;
    list->size++;

    return ok(NULL);
}

bool isListEmpty(List* list) {
    return list->size == 0;
}
