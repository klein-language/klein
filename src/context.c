#include "../include/context.h"
#include <stdlib.h>
#include <string.h>

/**
 * Declares a new variable in the given scope with the given name and
 * value.
 *
 * # Parameters
 *
 * - `scope` - The scope to add the variable to, which will be mutated to
 *   reflect the added variable.
 * - `name` - The name of the variable as a null-terminated string that lives
 *   at least as long as the given scope object.
 * - `value` - The value of the variable.
 *
 * # Returns
 *
 * `NULL` wrapped in a `Result`, assuming success.
 *
 * # Errors
 *
 * If memory fails to allocate for the new variable, an error is returned.
 * If a variable already exists in the given scope (or any of its parent
 * scopes), an error is returned. If the given `scope` or `name` is `NULL`,
 * an error is returned.
 */
KleinResult declareNewVariable(Scope* scope, ScopeDeclaration declaration) {
	// Error - Variable already exists
	Value* value = NULL;
	if (isOk(getVariable(*scope, declaration.name, &value))) {
		return (KleinResult) {
			.type = KLEIN_ERROR_DUPLICATE_VARIABLE_DECLARATION,
			.data = (KleinResultData) {
				.duplicateVariableDeclaration = declaration.name,
			},
		};
	}

	// Add the variable
	appendToScopeDeclarationList(&scope->variables, declaration);

	// Return ok
	return OK;
}

KleinResult reassignVariable(Scope* scope, ScopeDeclaration declaration) {
	// Variable already exists
	Value* value = NULL;
	if (isError(getVariable(*scope, declaration.name, &value))) {
		return (KleinResult) {
			.type = KLEIN_ERROR_REFERENCE_UNDEFINED_VARIABLE,
			.data = (KleinResultData) {
				.referenceUndefinedVariable = declaration.name,
			},
		};
	}

	// Add the variable
	*value = declaration.value;

	// Return ok
	return OK;
}

KleinResult setVariable(Scope* scope, ScopeDeclaration declaration) {
	// Variable already exists
	Value* value = NULL;
	getVariable(*scope, declaration.name, &value);

	// Add the variable
	if (value == NULL) {
		appendToScopeDeclarationList(&scope->variables, declaration);
	} else {
		*value = declaration.value;
	}

	// Return ok
	return OK;
}

/**
 * Returns a pointer to the `Expression` value stored in the variable with the
 * given name in the given scope. This will traverse up the scope hierarchy
 * to the global scope until the variable is found.
 *
 * # Parameters
 *
 * - `scope` - The scope to search in.
 * - `name` - The name of the variable as a null-terminated string. It only
 *   needs to be valid for the duration of the function call; It can be safely
 *   freed as soon as this function is finished. Doing so will not invalidate
 *   the returned `Expression` pointer.
 *
 * # Returns
 *
 * The variable's `Expression` value, wrapped in a `Result`, assuming success.
 *
 * # Errors
 *
 * If the given `name` is `NULL`, an error is returned.
 * If no variable exists with the given name in the given scope,
 * an error is returned.
 */
KleinResult getVariable(Scope scope, String name, Value** output) {
	Scope* current = &scope;
	while (current != NULL) {
		FOR_EACH_REF(ScopeDeclaration * variable, current->variables) {
			if (strcmp(variable->name, name) == 0) {
				RETURN_OK(output, &variable->value);
			}
		}
		END;
		current = current->parent;
	}

	return (KleinResult) {
		.type = KLEIN_ERROR_REFERENCE_UNDEFINED_VARIABLE,
		.data = (KleinResultData) {
			.referenceUndefinedVariable = name,
		},
	};
}

KleinResult enterNewScope(void) {
	ScopeList children = emptyScopeList();
	ScopeDeclarationList variables = emptyScopeDeclarationList();

	Scope scope = (Scope) {
		.parent = CONTEXT->scope,
		.children = children,
		.variables = variables,
	};

	appendToScopeList(&CONTEXT->scope->children, scope);
	CONTEXT->scope = &CONTEXT->scope->children.data[CONTEXT->scope->children.size - 1];

	return OK;
}

KleinResult exitScope(void) {
	if (CONTEXT->scope->parent == NULL) {
		UNREACHABLE;
	}

	CONTEXT->scope = CONTEXT->scope->parent;

	return OK;
}

/**
 * Creates a new context allocated on the heap, wrapped in a
 * `Result`. The caller is responsible for freeing the memory, most
 * likely using `freeContext()`.
 *
 * # Returns
 *
 * The context pointer wrapped in a result.
 *
 * # Errors
 *
 * If memory fails to allocate, an error is returned.
 */
KleinResult newContext(Context* output) {
	ScopeList children = emptyScopeList();
	ScopeDeclarationList variables = emptyScopeDeclarationList();

	Scope globalScope = (Scope) {
		.parent = NULL,
		.children = children,
		.variables = variables,
	};

	*output = (Context) {
		.globalScope = globalScope,
		.debugIndent = 0,
	};
	output->scope = &output->globalScope;

	return OK;
}

PRIVATE void freeScope(Scope scope) {
	FOR_EACH(Scope child, scope.children) {
		freeScope(child);
	}
	END;

	free(scope.children.data);
	free(scope.variables.data);
}

void freeContext(Context context) {
	freeScope(context.globalScope);
}

IMPLEMENT_KLEIN_LIST(Scope)
IMPLEMENT_KLEIN_LIST(ScopeDeclaration)
