#include "../include/typechecker.h"
#include "../include/parser.h"

Result typeOf(Expression expression, Type* output) {
	switch (expression.type) {
		default: {
			RETURN_ERROR("get type unimplemented");
		}
	}
}
