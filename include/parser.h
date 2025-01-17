#ifndef PARSER_H
#define PARSER_H

#include "builtin.h"
#include "lexer.h"
#include "list.h"
#include "result.h"
#include "typechecker.h"

struct Scope;
struct ExpressionList;
typedef struct StatementList StatementList;
typedef struct UnaryExpression UnaryExpression;
typedef struct Object Object;

typedef struct {
	StatementList* statements;
	struct Scope* innerScope;
} Block;

typedef enum {
	EXPRESSION_BOOLEAN,
	EXPRESSION_BINARY,
	EXPRESSION_FUNCTION,
	EXPRESSION_NUMBER,
	EXPRESSION_BLOCK,
	EXPRESSION_UNARY,
	EXPRESSION_IDENTIFIER,
	EXPRESSION_BUILTIN_FUNCTION,
	EXPRESSION_TYPE,
	EXPRESSION_OBJECT,
} ExpressionType;

typedef struct Expression Expression;

/**
 * A parameter in a function expression.
 */
typedef struct {
	/** The name of the parameter. */
	String name;

	/** The type of the parameter. */
	Type type;
} Parameter;

DEFINE_LIST(Parameter)

typedef struct {
	Expression* thisObject;
	BuiltinFunction function;
} BuiltinFunctionExpression;

struct Function {
	/**
	 * The parameters of this function. This `List` contains elements of
	 * type `Parameter`.
	 */
	ParameterList parameters;

	/** The return type of this function. */
	Type returnType;

	/** The body of this function. */
	Block body;

	Expression* thisObject;
};

typedef enum {
	BINARY_OPERATION_PLUS,
	BINARY_OPERATION_MINUS,
	BINARY_OPERATION_TIMES,
	BINARY_OPERATION_DIVIDE,
	BINARY_OPERATION_POWER,
	BINARY_OPERATION_AND,
	BINARY_OPERATION_OR,
	BINARY_OPERATION_DOT,
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

struct TypeDeclaration {
	ParameterList fields;
};

typedef union {
	/** A literal number expression. */
	double number;

	/** A block expression. */
	Block* block;

	/** A literal number expression. */
	Function function;

	UnaryExpression* unary;

	String identifier;

	/** A binary expression. */
	BinaryExpression binary;

	bool boolean;

	BuiltinFunctionExpression builtinFunction;

	TypeDeclaration typeDeclaration;

	Object* object;
} ExpressionData;

struct Expression {
	ExpressionType type;
	ExpressionData data;
};

DEFINE_LIST(Expression)

typedef union {
	ExpressionList functionCall;
} UnaryOperationData;

typedef struct {
	UnaryOperationData data;
	UnaryOperationType type;
} UnaryOperation;

struct UnaryExpression {
	Expression expression;
	UnaryOperation operation;
};

typedef struct {
	String name;
	void* value;
} InternalField;

typedef struct {
	char* name;
	Expression value;
} Field;

DEFINE_LIST(Field)
DEFINE_LIST(InternalField)

struct Object {
	FieldList fields;
	InternalFieldList internals;
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

DEFINE_LIST(Declaration)

typedef union {
	Declaration declaration;
	Expression expression;
} StatementData;

typedef struct {
	StatementData data;
	StatementType type;
} Statement;

DEFINE_LIST(Statement)

typedef struct BinaryOperator BinaryOperator;
struct BinaryOperator {
	BinaryOperator* precedent;
	TokenType* tokenTypes;
	size_t tokenTypeCount;
};

/**
 * A program's abstract syntax tree.
 */
typedef struct {
	/** The statements in the program. The elements in this list are of type `Statement`. */
	StatementList statements;
} Program;

Result getObjectInternal(Object object, char* name, void** output);
Result getObjectField(Object object, char* name, Expression** output);

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
Result parse(void* context, TokenList* tokens, Program* output);
void freeProgram(Program program);

#endif
