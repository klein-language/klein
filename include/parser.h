#ifndef PARSER_H
#define PARSER_H

#include "list.h"
#include "result.h"

struct Scope;

typedef struct {
    /** The statements in the block. The elements in this list are of type `Statement`. */
    List statements;

    struct Scope* innerScope;

} Block;

typedef enum {
    EXPRESSION_BOOLEAN,
    EXPRESSION_BINARY,
    EXPRESSION_FUNCTION,
    EXPRESSION_NUMBER,
    EXPRESSION_STRING,
    EXPRESSION_BLOCK,
    EXPRESSION_UNARY,
    EXPRESSION_IDENTIFIER,
    EXPRESSION_BUILTIN_FUNCTION
} ExpressionType;

typedef struct {

} Type;

/**
 * A parameter in a function expression.
 */
typedef struct {
    /** The name of the parameter. */
    char* name;

    /** The type of the parameter. */
    Type type;
} Parameter;

typedef struct {
    /**
     * The parameters of this function. This `List` contains elements of
     * type `Parameter`.
     */
    List parameters;

    /** The return type of this function. */
    Type returnType;

    /** The body of this function. */
    Block body;
} Function;

typedef struct Expression Expression;

typedef enum {
    BINARY_OPERATION_PLUS,
    BINARY_OPERATION_MINUS,
    BINARY_OPERATION_TIMES,
    BINARY_OPERATION_DIVIDE,
    BINARY_OPERATION_POWER,
    BINARY_OPERATION_AND,
    BINARY_OPERATION_OR,
    BINARY_OPERATION_LESS_THAN,
    BINARY_OPERATION_GREATER_THAN,
    BINARY_OPERATION_LESS_THAN_OR_EQUAL_TO,
    BINARY_OPERATION_GREATER_THAN_OR_EQUAL_TO
} BinaryOperation;

typedef struct {
    Expression* left;
    BinaryOperation operation;
    Expression* right;
} BinaryExpression;

typedef enum {
    UNARY_OPERATION_NOT,
    UNARY_OPERATION_FUNCTION_CALL,
} UnaryOperationType;

typedef union {
    List functionCall;
} UnaryOperationData;

typedef struct {
    UnaryOperationData data;
    UnaryOperationType type;
} UnaryOperation;

typedef struct {
    Expression* expression;
    UnaryOperation operation;
} UnaryExpression;

typedef union {

    /** A literal string expression. */
    char* string;

    /** A literal number expression. */
    float number;

    /** A block expression. */
    Block block;

    /** A literal number expression. */
    Function function;

    UnaryExpression unary;

    char* identifier;

    /** A binary expression. */
    BinaryExpression binary;

    bool boolean;

    Result (*builtinFunction)(List);
} ExpressionData;

struct Expression {
    ExpressionType type;
    ExpressionData data;
};

typedef enum {
    STATEMENT_DECLARATION,
    STATEMENT_EXPRESSION,
} StatementType;

typedef struct {
    char* name;
    Type* type;
    Expression value;
} Declaration;

typedef union {
    Declaration declaration;
    Expression expression;
} StatementData;

typedef struct {
    StatementData data;
    StatementType type;
} Statement;

/**
 * A program's abstract syntax tree.
 */
typedef struct {
    /** The statements in the program. The elements in this list are of type `Statement`. */
    List statements;
} Program;

/**
 * Parses a program from a stream of tokens into an abstract syntax tree.
 *
 * # Parameters
 *
 * - `tokens` - The program's tokens as a `List` of `Tokens`, generally from the
 *   output of a call to `tokenize()` from `lexer.h`.
 *
 * # Returns
 *
 * The parsed abstract syntax tree.
 *
 * # Errors
 *
 * If the system failed to allocate memory for the abstract syntax tree, an error is
 * returned. If an unexpected token is encountered while parsing (i.e. the user entered
 * malformatted syntax), an error is returned.
 */
Result parse(void* context, List* tokens);

void freeExpression(Expression* expression);
void freeStatement(Statement* statement);

#endif
