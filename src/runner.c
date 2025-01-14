#include "../include/builtin.h"
#include "../include/context.h"
#include "../include/parser.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static const char* EXPRESSION_TYPE_NAMES[] = {
    "boolean",
    "binary expression",
    "function expression",
    "number",
    "string expression",
    "block",
    "unary expression",
    "identifier",
    "builtin expression",
    "type expression",
    "object",
};

Result
evaluateStatement(Context* context, Statement* statement);

Result evaluateBlock(Context* context, Block* block) {
    for (int statementNumber = 0; statementNumber < block->statements.size; statementNumber++) {
        TRY(evaluateStatement(context, block->statements.data[statementNumber]));
    }

    return ok(block);
}

Result evaluateExpression(Context* context, Expression* expression) {
    switch (expression->type) {

        // Literals
        case EXPRESSION_NUMBER:
        case EXPRESSION_STRING:
        case EXPRESSION_BOOLEAN:
        case EXPRESSION_IDENTIFIER:
        case EXPRESSION_TYPE:
        case EXPRESSION_OBJECT:
        case EXPRESSION_FUNCTION: {
            return ok(expression);
        }

        // Block
        case EXPRESSION_BLOCK: {
            return evaluateBlock(context, &expression->data.block);
        }

        case EXPRESSION_BUILTIN_FUNCTION: {
            return ERROR(ERROR_INTERNAL, "unreachable");
        }

        // Unary expressions
        case EXPRESSION_UNARY: {
            UnaryExpression unary = expression->data.unary;
            unary.expression = TRY(evaluateExpression(context, unary.expression));
            switch (unary.operation.type) {

                // Function call
                case UNARY_OPERATION_FUNCTION_CALL: {
                    switch (unary.expression->type) {
                        case EXPRESSION_BUILTIN_FUNCTION: {
                            if (unary.expression->data.builtinFunction.thisObject != NULL) {
                                prependToList(&unary.operation.data.functionCall, unary.expression->data.builtinFunction.thisObject);
                            }
                            return unary.expression->data.builtinFunction.function(context, unary.operation.data.functionCall);
                        }
                        case EXPRESSION_FUNCTION: {
                            return evaluateBlock(context, &unary.expression->data.function.body);
                        }
                        case EXPRESSION_IDENTIFIER: {
                            char* identifier = unary.expression->data.identifier;

                            // Builtin function
                            if (strcmp(identifier, "builtin") == 0) {
                                char* builtinName = TRY(getObjectInternal(((Expression*) unary.operation.data.functionCall.data[0])->data.object, "string_value"));
                                Result (*function)(void*, List) = TRY(getBuiltin(builtinName));
                                return ok(HEAP(((Expression) {.type = EXPRESSION_BUILTIN_FUNCTION, .data = (ExpressionData) {.builtinFunction = {.function = function, .thisObject = NULL}}}), Expression));
                            }

                            Expression* value = TRY(getVariable(*context->scope, identifier));
                            if (value->type == EXPRESSION_FUNCTION) {
                                return evaluateBlock(context, &value->data.function.body);
                            }

                            if (value->type == EXPRESSION_BUILTIN_FUNCTION) {
                                if (value->data.builtinFunction.thisObject != NULL) {
                                    prependToList(&unary.operation.data.functionCall, unary.expression->data.builtinFunction.thisObject);
                                }
                                return value->data.builtinFunction.function(context, unary.operation.data.functionCall);
                            }

                            return ERROR(ERROR_CALL_NON_FUNCTION, "Attempted to call a non-function");
                        }
                        default: {
                            return ERROR(ERROR_CALL_NON_FUNCTION, "Attempted to call a ", EXPRESSION_TYPE_NAMES[unary.expression->type], " as a function");
                        }
                    }
                }

                // Negation
                case UNARY_OPERATION_NOT: {
                    if (unary.expression->type != EXPRESSION_BOOLEAN) {
                        return ERROR(ERROR_INVALID_OPERAND, "Attempted to negate non-boolean");
                    }

                    bool result = !expression->data.boolean;
                    return ok(HEAP(((Expression) {.type = EXPRESSION_BOOLEAN, .data = (ExpressionData) {.boolean = result}}), Expression));
                }
            }
        }

        // Binary Expression
        case EXPRESSION_BINARY: {
            Expression* left = TRY(evaluateExpression(context, expression->data.binary.left));
            Expression* right = TRY(evaluateExpression(context, expression->data.binary.right));

            switch (expression->data.binary.operation) {
                // Field access
                case BINARY_OPERATION_DOT: {
                    if (right->type != EXPRESSION_IDENTIFIER) {
                        return ERROR(ERROR_INVALID_OPERAND, "Right hand side of dot must be an identifier");
                    }

                    if (left->type != EXPRESSION_OBJECT) {
                        return ERROR(ERROR_INVALID_OPERAND, "Left hand side of dot must be an object");
                    }
                    Expression* fieldValue = TRY(getObjectField(left->data.object, right->data.identifier));
                    if (fieldValue->type == EXPRESSION_FUNCTION) {
                        fieldValue->data.function.thisObject = left;
                    } else if (fieldValue->type == EXPRESSION_BUILTIN_FUNCTION) {
                        fieldValue->data.builtinFunction.thisObject = left;
                    }

                    return ok(fieldValue);
                }

                // Addition
                case BINARY_OPERATION_PLUS: {
                    switch (left->type) {

                        // Add two strings
                        case EXPRESSION_STRING: {
                            if (right->type != EXPRESSION_STRING) {
                                return ERROR(ERROR_INVALID_OPERAND, "Attempted to add a non-string to a string");
                            }

                            int leftLength = strlen(left->data.string);
                            char* result = malloc(leftLength + strlen(right->data.string) + 1);
                            strncat(result, left->data.string, leftLength);
                            strcat(result, left->data.string);

                            return ok(HEAP(((Expression) {.type = EXPRESSION_STRING, .data = (ExpressionData) {.string = result}}), Expression));
                        }

                        // Add two numbers
                        case EXPRESSION_NUMBER: {
                            if (right->type != EXPRESSION_NUMBER) {
                                return ERROR(ERROR_INVALID_OPERAND, "Attempted to add a non-number to a number");
                            }

                            float result = left->data.number + right->data.number;
                            return ok(HEAP(((Expression) {.type = EXPRESSION_NUMBER, .data = (ExpressionData) {.number = result}}), Expression));
                        }

                        // Add non-strings and non-numbers
                        default: {
                            return ERROR(ERROR_INVALID_OPERAND, "Attempted to add to a non-addable value");
                        }
                    }
                }

                // Subtraction
                case BINARY_OPERATION_MINUS: {
                    if (left->type != EXPRESSION_NUMBER && right->type != EXPRESSION_NUMBER) {
                        return ERROR(ERROR_INVALID_OPERAND, "Attempted to subtract non-numbers");
                    }

                    float result = left->data.number - right->data.number;
                    return ok(HEAP(((Expression) {.type = EXPRESSION_NUMBER, .data = (ExpressionData) {.number = result}}), Expression));
                }

                // Multiplication
                case BINARY_OPERATION_TIMES: {
                    if (left->type != EXPRESSION_NUMBER && right->type != EXPRESSION_NUMBER) {
                        return ERROR(ERROR_INVALID_OPERAND, "Attempted to multiply non-numbers");
                    }

                    float result = left->data.number * right->data.number;
                    return ok(HEAP(((Expression) {.type = EXPRESSION_NUMBER, .data = (ExpressionData) {.number = result}}), Expression));
                }

                // Division
                case BINARY_OPERATION_DIVIDE: {
                    if (left->type != EXPRESSION_NUMBER && right->type != EXPRESSION_NUMBER) {
                        return ERROR(ERROR_INVALID_OPERAND, "Attempted to divide non-numbers");
                    }

                    float result = left->data.number / right->data.number;
                    return ok(HEAP(((Expression) {.type = EXPRESSION_NUMBER, .data = (ExpressionData) {.number = result}}), Expression));
                }

                // Exponentiation
                case BINARY_OPERATION_POWER: {
                    if (left->type != EXPRESSION_NUMBER && right->type != EXPRESSION_NUMBER) {
                        return ERROR(ERROR_INVALID_OPERAND, "Attempted to exponentiate non-numbers");
                    }

                    float result = pow(left->data.number, right->data.number);
                    return ok(HEAP(((Expression) {.type = EXPRESSION_NUMBER, .data = (ExpressionData) {.number = result}}), Expression));
                }

                // Less than
                case BINARY_OPERATION_LESS_THAN: {
                    if (left->type != EXPRESSION_NUMBER && right->type != EXPRESSION_NUMBER) {
                        return ERROR(ERROR_INVALID_OPERAND, "Attempted to compare non-numbers");
                    }

                    bool result = left->data.number < right->data.number;
                    return ok(HEAP(((Expression) {.type = EXPRESSION_BOOLEAN, .data = (ExpressionData) {.boolean = result}}), Expression));
                }

                // Greater than
                case BINARY_OPERATION_GREATER_THAN: {
                    if (left->type != EXPRESSION_NUMBER && right->type != EXPRESSION_NUMBER) {
                        return ERROR(ERROR_INVALID_OPERAND, "Attempted to compare non-numbers");
                    }

                    bool result = left->data.number > right->data.number;
                    return ok(HEAP(((Expression) {.type = EXPRESSION_BOOLEAN, .data = (ExpressionData) {.boolean = result}}), Expression));
                }

                // less than or equal to
                case BINARY_OPERATION_LESS_THAN_OR_EQUAL_TO: {
                    if (left->type != EXPRESSION_NUMBER && right->type != EXPRESSION_NUMBER) {
                        return ERROR(ERROR_INVALID_OPERAND, "Attempted to compare non-numbers");
                    }

                    bool result = left->data.number <= right->data.number;
                    return ok(HEAP(((Expression) {.type = EXPRESSION_BOOLEAN, .data = (ExpressionData) {.boolean = result}}), Expression));
                }

                // Greater than or equal to
                case BINARY_OPERATION_GREATER_THAN_OR_EQUAL_TO: {
                    if (left->type != EXPRESSION_NUMBER && right->type != EXPRESSION_NUMBER) {
                        return ERROR(ERROR_INVALID_OPERAND, "Attempted to compare non-numbers");
                    }

                    bool result = left->data.number >= right->data.number;
                    return ok(HEAP(((Expression) {.type = EXPRESSION_BOOLEAN, .data = (ExpressionData) {.boolean = result}}), Expression));
                }

                // AND-ing
                case BINARY_OPERATION_AND: {
                    if (left->type != EXPRESSION_BOOLEAN && right->type != EXPRESSION_BOOLEAN) {
                        return ERROR(ERROR_INVALID_OPERAND, "Attempted to and non-booleans");
                    }

                    bool result = left->data.boolean && right->data.boolean;
                    return ok(HEAP(((Expression) {.type = EXPRESSION_BOOLEAN, .data = (ExpressionData) {.boolean = result}}), Expression));
                }

                // OR-ing
                case BINARY_OPERATION_OR: {
                    if (left->type != EXPRESSION_BOOLEAN && right->type != EXPRESSION_BOOLEAN) {
                        return ERROR(ERROR_INVALID_OPERAND, "Attempted to and non-booleans");
                    }

                    bool result = left->data.boolean || right->data.boolean;
                    return ok(HEAP(((Expression) {.type = EXPRESSION_BOOLEAN, .data = (ExpressionData) {.boolean = result}}), Expression));
                }
            }
        }
    }

    return ERROR(ERROR_INTERNAL, "Invalid expression type: ", expression->type);
}

Result evaluateStatement(Context* context, Statement* statement) {
    switch (statement->type) {
        case STATEMENT_EXPRESSION: {
            TRY(evaluateExpression(context, &statement->data.expression));
            break;
        }

        case STATEMENT_DECLARATION: {
            statement->data.declaration.value = *(Expression*) TRY(evaluateExpression(context, &statement->data.declaration.value));
            TRY(declareNewVariable(context->scope, statement->data.declaration));
            break;
        }
    }

    return ok(NULL);
}

Result run(Context* context, Program program) {
    for (int statementNumber = 0; statementNumber < program.statements.size; statementNumber++) {
        TRY(evaluateStatement(context, program.statements.data[statementNumber]));
    }

    return ok(NULL);
}
