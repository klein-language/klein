#include "../include/result.h"
#include <stdarg.h>
#include <stdbool.h>

char* errorMessage(Result error) {
	switch (error) {
		case OK: {
			return "Success.";
		}
		case ERROR_NULL: {
			return "A value that mustn't be null was null.";
		}
		case ERROR_PRINT: {
			return "An system error occurred while attempting to print to stdout.";
		}
		case ERROR_DUPLICATE_VARIABLE: {
			return "Variable defined twice.";
		}
		case ERROR_INTERNAL: {
			return "An internal error has occurred.";
		}
		case ERROR_UNEXPECTED_TOKEN: {
			return "Unexpected token.";
		}
		case ERROR_INVALID_ARGUMENTS: {
			return "Invalid arguments to function call.";
		}
		case ERROR_REFERENCE_NONEXISTENT_VARIABLE: {
			return "Attempted to reference a variable that doesn't exist.";
		}
		case ERROR_CALL_NON_FUNCTION: {
			return "Attempted to call a value that's not a function.";
		}
		case ERROR_INVALID_OPERAND: {
			return "Invalid operands provided to operation.";
		}
		case ERROR_UNRECOGNIZED_TOKEN: {
			return "Unrecognized token.";
		}
	}
}
