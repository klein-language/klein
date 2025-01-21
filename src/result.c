#include "../include/result.h"
#include <stdarg.h>
#include <stdbool.h>

KleinResult OK = (KleinResult) {.type = KLEIN_OK};

bool isOk(KleinResult result) {
	return result.type == KLEIN_OK;
}

bool isError(KleinResult result) {
	return !isOk(result);
}
