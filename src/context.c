#include "../include/context.h"
#include "../include/parser.h"
#include "../include/util.h"
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
    // Error - null scope
    if (scope == NULL) {
        return ERROR(ERROR_INTERNAL, "Attempted to declare a new variable into a null scope");
    }

    // Error - Variable already exists
    Result attempt = getVariable(*scope, declaration.name);
    if (attempt.success) {
        return ERROR(ERROR_DUPLICATE_VARIABLE, "Attempted to declare a new variable with the name \"", declaration.name, "\", but there's already a variable with that name where the declaration was made.");
    } else {
        free(attempt.data.errorMessage);
    }

    // Add the variable
    TRY(appendToList(&scope->variables, HEAP(declaration, Declaration)));

    // Return ok
    return ok(NULL);
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
Result getVariable(Scope scope, char* name) {
    Scope* current = &scope;
    while (current != NULL) {
        for (int variableNumber = 0; variableNumber < current->variables.size; variableNumber++) {
            Declaration* variable = current->variables.data[variableNumber];
            if (strcmp(variable->name, name) == 0) {
                return ok(&variable->value);
            }
        }
        current = current->parent;
    }

    return ERROR(ERROR_REFERENCE_NONEXISTENT_VARIABLE, "Attempted to reference a variable called \"", name, "\", but no variable with that name exists where it was referenced.");
}

Result enterNewScope(Context* context) {
    Scope* scope = NONNULL(malloc(sizeof(Scope)));
    scope->parent = context->scope;
    TRY(appendToList(&context->scope->children, scope));
    scope->children = EMPTY_LIST();
    scope->variables = EMPTY_LIST();
    context->scope = scope;
    return ok(NULL);
}

Result exitScope(Context* context) {
    if (context->scope->parent == NULL) {
        return ERROR(ERROR_INTERNAL, "Attempted to exit the global scope");
    }

    context->scope = context->scope->parent;

    return ok(NULL);
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
Result newContext() {
    return ok(HEAP(((Context) {.scope = HEAP(((Scope) {.parent = NULL, .children = EMPTY_LIST(), .variables = EMPTY_LIST()}), Scope)}), Context));
}

void freeScope(Scope* scope) {
    for (int variableNumber = 0; variableNumber < scope->variables.size; variableNumber++) {
        Declaration* declaration = scope->variables.data[variableNumber];
        freeExpression(&declaration->value);
        free(declaration->name);
        free(declaration);
    }

    for (int childNumber = 0; childNumber < scope->children.size; childNumber++) {
        freeScope(scope->children.data[childNumber]);
    }

    free(scope->variables.data);
    free(scope->children.data);
    free(scope);
}

void freeContext(Context* context) {
    // Free scopes
    Scope* currentScope = context->scope;
    while (currentScope->parent != NULL) {
        currentScope = currentScope->parent;
    }
    freeScope(currentScope);
}
