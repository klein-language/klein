#ifndef STDLIB_H
#define STDLIB_H

#define STDLIB                                                             \
	"let print = builtin(\"print\");"                                      \
	"let input = builtin(\"input\");"                                      \
	""                                                                     \
	"let range = function(low: Number, high: Number): List {"              \
	"    let numbers = [];"                                                \
	"    let current = low;"                                               \
	"    while current <= high {"                                          \
	"        numbers.append(current);"                                     \
	"        current = current + 1;"                                       \
	"    };"                                                               \
	"    return numbers;"                                                  \
	"};"                                                                   \
	""                                                                     \
	"let list_contains = function(list: List, value: Anything): Boolean {" \
	"    for element in list {"                                            \
	"        if element == value {"                                        \
	"            return true;"                                             \
	"        };"                                                           \
	"    };"                                                               \
	"    return false;"                                                    \
	"};"                                                                   \
	""

#endif
