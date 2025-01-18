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
Result declareNewVariable(Scope* scope, Declaration declaration) {
	DEBUG_START("Declaring", "new variable \"%s\"", declaration.name);
	// Error - null scope
	if (scope == NULL) {
		return ERROR_INTERNAL;
	}

	// Error - Variable already exists
	Expression* value = NULL;
	if (getVariable(*scope, declaration.name, &value) == OK) {
		DEBUG_ERROR("Variable \"%s\", already exists, declaration.name");
		return ERROR_DUPLICATE_VARIABLE;
	}

	// Add the variable
	TRY(appendToDeclarationList(&scope->variables, declaration));

	// Return ok
	DEBUG_END("Declaring new variable \"%s\"", declaration.name);
	return OK;
}

Result reassignVariable(Scope* scope, Declaration declaration) {
	// Error - null scope
	if (scope == NULL) {
		return ERROR_INTERNAL;
	}

	// Variable already exists
	Expression* value = NULL;
	if (getVariable(*scope, declaration.name, &value) != OK) {
		return ERROR_REFERENCE_NONEXISTENT_VARIABLE;
	}

	// Add the variable
	*value = declaration.value;

	// Return ok
	return OK;
}

Result setVariable(Scope* scope, Declaration declaration) {
	DEBUG_LOG("Setting", "variable \"%s\" to a %s", declaration.name, expressionTypeName(declaration.value.type));

	// Error - null scope
	if (scope == NULL) {
		return ERROR_INTERNAL;
	}

	// Variable already exists
	Expression* value = NULL;
	getVariable(*scope, declaration.name, &value);

	// Add the variable
	if (value == NULL) {
		TRY(appendToDeclarationList(&scope->variables, declaration));
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
Result getVariable(Scope scope, String name, Expression** output) {
	DEBUG_LOG("Resolving", "variable \"%s\"", name);
	Scope* current = &scope;
	while (current != NULL) {
		FOR_EACH_REF(Declaration * variable, current->variables) {
			if (strcmp(variable->name, name) == 0) {
				DEBUG_LOG("Resolved", "variable %s to a %s", name, expressionTypeName(variable->value.type));
				RETURN_OK(output, &variable->value);
			}
		}
		END;
		current = current->parent;
	}

	DEBUG_ERROR("Variable \"%s\" couldn't be resolved", name);
	return ERROR_REFERENCE_NONEXISTENT_VARIABLE;
}

Result enterNewScope() {
	ScopeList children;
	TRY(emptyScopeList(&children));

	DeclarationList variables;
	TRY(emptyDeclarationList(&variables));

	Scope scope = (Scope) {
		.parent = CONTEXT->scope,
		.children = children,
		.variables = variables,
	};

	TRY(appendToScopeList(&CONTEXT->scope->children, scope));
	CONTEXT->scope = &CONTEXT->scope->children.data[CONTEXT->scope->children.size - 1];

	return OK;
}

Result exitScope() {
	if (CONTEXT->scope->parent == NULL) {
		return ERROR_INTERNAL;
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
	TRY(emptyScopeList(&children));

	DeclarationList variables;
	TRY(emptyDeclarationList(&variables));

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

IMPLEMENT_LIST(Scope)
