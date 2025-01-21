#include "../include/result.h"
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

KleinResult OK = (KleinResult) {.errorMessage = NULL};

bool isOk(KleinResult result) {
	return result.errorMessage == NULL;
}

bool isError(KleinResult result) {
	return result.errorMessage != NULL;
}

KleinResult error(String message) {
	return (KleinResult) {
		.errorMessage = strdup(message),
	};
}
