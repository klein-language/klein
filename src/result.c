#include "../include/result.h"
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

Result OK = (Result) {.errorMessage = NULL};

bool isOk(Result result) {
	return result.errorMessage == NULL;
}

bool isError(Result result) {
	return result.errorMessage != NULL;
}

Result error(String message) {
	return (Result) {
		.errorMessage = strdup(message),
	};
}
