#ifndef STDLIB_H
#define STDLIB_H

#define STDLIB                                                \
	"let print = builtin(\"print\");"                         \
	"let input = builtin(\"input\");"                         \
	""                                                        \
	"let String = type {"                                     \
	"    length: function(): Number"                          \
	"};"                                                      \
	""                                                        \
	"let range = function(low: Number, high: Number): List {" \
	"    let numbers = [];"                                   \
	"    let current = low;"                                  \
	"    while current <= high {"                             \
	"        numbers.append(current);"                        \
	"        current = current + 1;"                          \
	"    };"                                                  \
	"    return numbers;"                                     \
	"};"                                                      \
	""

#endif
