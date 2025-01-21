#ifndef STDLIB_H
#define STDLIB_H

#define STDLIB                                                             \
	"let print = builtin(\"print\");"                                      \
	"let input = builtin(\"input\");"                                      \
	""                                                                     \
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
