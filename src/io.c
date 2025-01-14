#include "../include/result.h"
#include <stdio.h>
#include <stdlib.h>

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
Result readFile(char* path) {
	// Open file
	FILE* file = NONNULL(fopen(path, "rb"));

	// Get file size
	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);

	// Allocate space
	char* buffer = NONNULL(malloc(length + 1));

	// Read & close file
	fread(buffer, 1, length, file);
	fclose(file);
	buffer[length] = '\0';

	return ok(buffer);
}
