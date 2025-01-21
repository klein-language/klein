#include "../include/context.h"
#include "../include/parser.h"
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
Result declareNewVariable(Scope* scope, ScopeDeclaration declaration) {
	// Error - null scope
	if (scope == NULL) {
		RETURN_ERROR("Attempted to declare a variable to a NULL scope.");
	}

	// Error - Variable already exists
	Value* value = NULL;
	if (isOk(getVariable(*scope, declaration.name, &value))) {
		RETURN_ERROR("Duplicate variable declaration for \"%s\"", declaration.name);
	}

	// Add the variable
	TRY(appendToScopeDeclarationList(&scope->variables, declaration), "appending the declaration for \"%s\" to the scope's declarations", declaration.name);

	// Return ok
	return OK;
}

Result reassignVariable(Scope* scope, ScopeDeclaration declaration) {
	// Error - null scope
	if (scope == NULL) {
		RETURN_ERROR("Attempted to reassign a variable in a NULL scope");
	}

	// Variable already exists
	Value* value = NULL;
	if (isError(getVariable(*scope, declaration.name, &value))) {
		RETURN_ERROR("Attempted to reference a variable called \"%s\", but no variable with that name exists where it was referenced.", declaration.name);
	}

	// Add the variable
	*value = declaration.value;

	// Return ok
	return OK;
}

Result setVariable(Scope* scope, ScopeDeclaration declaration) {
	// Error - null scope
	if (scope == NULL) {
		RETURN_ERROR("Attempted to set a variable in a NULL scope");
	}

	// Variable already exists
	Value* value = NULL;
	getVariable(*scope, declaration.name, &value);

	// Add the variable
	if (value == NULL) {
		TRY(appendToScopeDeclarationList(&scope->variables, declaration), "appending the declaration for \"%s\" to the scope's declarations", declaration.name);
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
Result getVariable(Scope scope, String name, Value** output) {
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

	RETURN_ERROR("Attempted to reference a variable called \"%s\", but no variable with that name exists where it was referenced.", name);
}

Result enterNewScope(void) {
	ScopeList children;
	TRY(emptyScopeList(&children), "creating the child scope list for a scope");

	ScopeDeclarationList variables;
	TRY(emptyScopeDeclarationList(&variables), "creating the variable list for a scope");

	Scope scope = (Scope) {
		.parent = CONTEXT->scope,
		.children = children,
		.variables = variables,
	};

	TRY(appendToScopeList(&CONTEXT->scope->children, scope), "appending to the current scope's children list");
	CONTEXT->scope = &CONTEXT->scope->children.data[CONTEXT->scope->children.size - 1];

	return OK;
}

Result exitScope(void) {
	if (CONTEXT->scope->parent == NULL) {
		RETURN_ERROR("Attempted to exit the global scope.");
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
Result newContext(Context* output) {
	ScopeList children;
	TRY(emptyScopeList(&children), "creating the context's scope list");

	ScopeDeclarationList variables;
	TRY(emptyScopeDeclarationList(&variables), "creating the context's variable list");

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
