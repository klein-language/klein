#ifndef IO_H
#define IO_H

#include "result.h"

/**
 * Reads the contents of a file into a string. If successful, the returned
 * `Result` contains a null terminated `char*` allocated on the heap, which
 * the caller is responsible for freeing.
 *
 * # Parameters
 *
 * - `path` - The file path, as specified by the parameter to `fopen`.
 *
 * # Returns
 *
 * The file contents as a null-terminated `char*` in a `Result`.
 *
 * # Errors
 *
 * If memory failed to allocate, an error is returned. If the given file
 * doesn't exist or is inaccessible, an error is returned.
 */
Result readFile(char* path);

#endif
