#ifndef LIST_H
#define LIST_H

#include "result.h"
#include "util.h"
#include <stdbool.h>
#include <stdlib.h>

typedef char Char;

#define DEFINE_LIST(type)                                       \
	typedef struct type##List type##List;                       \
	struct type##List {                                         \
		size_t size;                                            \
		size_t capacity;                                        \
		type* data;                                             \
	};                                                          \
                                                                \
	Result empty##type##List(type##List* output);               \
	Result emptyHeap##type##List(type##List** output);          \
	Result appendTo##type##List(type##List* list, type value);  \
	Result prependTo##type##List(type##List* list, type value); \
	bool is##type##ListEmpty(type##List list);                  \
	Result pop##type##List(type##List* list);                   \
	type getFrom##type##ListUnchecked(type##List list, size_t index);

#define IMPLEMENT_LIST(type)                                                         \
	Result empty##type##List(type##List* output) {                                   \
		*output = (type##List) {                                                     \
			.size = 0,                                                               \
			.capacity = 8,                                                           \
			.data = malloc(sizeof(type) * 8),                                        \
		};                                                                           \
		ASSERT_NONNULL(output->data);                                                \
		return OK;                                                                   \
	}                                                                                \
                                                                                     \
	Result emptyHeap##type##List(type##List** output) {                              \
		*output = malloc(sizeof(type##List));                                        \
		ASSERT_NONNULL(*output);                                                     \
		**output = (type##List) {                                                    \
			.size = 0,                                                               \
			.capacity = 8,                                                           \
			.data = malloc(sizeof(type) * 8),                                        \
		};                                                                           \
		ASSERT_NONNULL((*output)->data);                                             \
		return OK;                                                                   \
	}                                                                                \
                                                                                     \
	Result appendTo##type##List(type##List* list, type value) {                      \
		if (list->size == list->capacity) {                                          \
			list->capacity *= 2;                                                     \
			list->data = (type*) realloc(list->data, sizeof(type) * list->capacity); \
			ASSERT_NONNULL(list->data);                                              \
		}                                                                            \
                                                                                     \
		list->data[list->size] = value;                                              \
		list->size++;                                                                \
                                                                                     \
		return OK;                                                                   \
	}                                                                                \
                                                                                     \
	Result prependTo##type##List(type##List* list, type value) {                     \
		if (list->size == list->capacity) {                                          \
			list->capacity *= 2;                                                     \
			list->data = realloc(list->data, sizeof(type) * list->capacity);         \
			ASSERT_NONNULL(list->data);                                              \
		}                                                                            \
                                                                                     \
		for (size_t index = list->size; index > 0; index--) {                        \
			list->data[index] = list->data[index - 1];                               \
		}                                                                            \
                                                                                     \
		list->data[0] = value;                                                       \
		list->size++;                                                                \
                                                                                     \
		return OK;                                                                   \
	}                                                                                \
                                                                                     \
	Result pop##type##List(type##List* list) {                                       \
		if (list->size == 0) {                                                       \
			return error("Attempted to pop from an empty list");                     \
		}                                                                            \
                                                                                     \
		for (size_t index = 0; index < list->size - 1; index++) {                    \
			list->data[index] = list->data[index + 1];                               \
		}                                                                            \
                                                                                     \
		list->size--;                                                                \
                                                                                     \
		return OK;                                                                   \
	}                                                                                \
                                                                                     \
	bool is##type##ListEmpty(type##List list) {                                      \
		return list.size == 0;                                                       \
	}                                                                                \
                                                                                     \
	type getFrom##type##ListUnchecked(type##List list, size_t index) {               \
		return list.data[index];                                                     \
	}

/**
 * Iterates over the given list, binding the given name to the current value.
 *
 * To iterate over a list pointer, use `FOR_EACHP`.
 * To iterate over values by reference, use `FOR_EACH_REF`.
 *
 * This macro opens a block which must be closed when the loop ends.
 *
 * Note that this macro really confuses `clang-format`; It's recommended to use
 * `END;` afterwards to close loop block.
 */
#define FOR_EACH(name__, list__)                                 \
	for (size_t index__ = 0; index__ < list__.size; index__++) { \
		name__ = list__.data[index__];

#define FOR_EACHP(name__, list__)                                 \
	for (size_t index__ = 0; index__ < list__->size; index__++) { \
		name__ = list__->data[index__];

#define FOR_EACH_REF(name__, list__)                             \
	for (size_t index__ = 0; index__ < list__.size; index__++) { \
		name__ = &list__.data[index__];

#define END }

DEFINE_LIST(Char)
DEFINE_LIST(String)

#endif
