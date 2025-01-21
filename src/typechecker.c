#include "../include/typechecker.h"
#include "../include/result.h"

KleinResult typeOf(Expression expression, Type* output) {
	switch (expression.type) {
		default: {
			RETURN_ERROR("get type unimplemented");
		}
	}
}
