#ifndef LIST_H
#define LIST_H

#include "result.h"
#include "util.h"
#include <stdbool.h>

/**
 * A generic, growable list.
 *
 * The elements of this list are stored in `list.data` contiguously in memory.
 * Thus, the `nth` element of a list can be accessed using `list->data[n]`, where
 * `n < list.size`.
 *
 * For information on how this list expands, see `appendToList()`.
 */
typedef struct {

	/**
	 * The data stored in this list, as an array of void pointers. The array is
	 * not (necessarily) null-terminated; Instead, it's size is tracked by `size`.
	 */
	void** data;

	/**
	 * The number of elements currently stored in the list. This is equivalent to the
	 * length of `data`.
	 */
	int size;

	/**
	 * The number of elements that memory has been allocated for in the list. This does
	 * *not* necessarily reflect the number of elements stored in the list, but rather
	 * the maximum number of elements that *could* currently be stored before allocating
	 * more space. For the former, use `size`.
	 */
	int capacity;

} List;

Result emptyHeapList();
Result emptyList(List* list);

bool isListEmpty(List* list);

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
Result appendToList(List* list, void* value);
Result prependToList(List* list, void* value);

/**
 * Creates a new empty list on the stack, returning an error `Result` from the current
 * function if memory couldn't be allocated for it.
 */
#define EMPTY_LIST() ({    \
	List list;             \
	TRY(emptyList(&list)); \
	list;                  \
})

#endif
