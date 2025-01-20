#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "list.h"
#include "result.h"
#include "typechecker.h"

struct Scope;
struct ExpressionList;
typedef struct StatementList StatementList;
typedef struct UnaryExpression UnaryExpression;
typedef struct Object Object;
typedef struct ForLoop ForLoop;
typedef struct WhileLoop WhileLoop;
typedef struct IfExpressionList IfExpressionList;

typedef struct {
	StatementList* statements;
	struct Scope* innerScope;
} Block;

typedef enum {
	EXPRESSION_BOOLEAN,
	EXPRESSION_BINARY,
	EXPRESSION_FUNCTION,
	EXPRESSION_BLOCK,
	EXPRESSION_UNARY,
	EXPRESSION_IDENTIFIER,
	EXPRESSION_BUILTIN_FUNCTION,
	EXPRESSION_OBJECT,
	EXPRESSION_FOR_LOOP,
	EXPRESSION_WHILE_LOOP,
	EXPRESSION_STRING,
	EXPRESSION_NUMBER,
	EXPRESSION_LIST,
	EXPRESSION_IF
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
	BINARY_OPERATION_ASSIGN,
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
	BINARY_OPERATION_GREATER_THAN_OR_EQUAL_TO,
	BINARY_OPERATION_EQUAL
} BinaryOperation;

typedef struct {
	Expression* left;
	BinaryOperation operation;
	Expression* right;
} BinaryExpression;

typedef enum {
	UNARY_OPERATION_NOT,
	UNARY_OPERATION_FUNCTION_CALL
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

	TypeDeclaration typeDeclaration;

	Object* object;

	ForLoop* forLoop;

	WhileLoop* whileLoop;

	struct ExpressionList* list;

	String string;

	IfExpressionList* ifExpression;
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
	char* name;
	Expression value;
} Field;

DEFINE_LIST(Field)

struct Object {
	FieldList fields;
};

typedef enum {
	STATEMENT_DECLARATION,
	STATEMENT_EXPRESSION,
	STATEMENT_RETURN
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
	Expression returnExpression;
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

struct ForLoop {
	String binding;
	Expression list;
	Block body;
};

struct WhileLoop {
	Expression condition;
	Block body;
};

typedef struct {
	Expression condition;
	Block body;
} IfExpression;

/**
 * A program's abstract syntax tree.
 */
typedef struct {
	/** The statements in the program. The elements in this list are of type `Statement`. */
	StatementList statements;
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
Result parse(TokenList* tokens, Program* output);
void freeProgram(Program program);
Result debugExpression(Expression expression);

typedef enum {
	INTERNAL_KEY_STRING,
	INTERNAL_KEY_NUMBER,
	INTERNAL_KEY_BOOLEAN,
	INTERNAL_KEY_LIST,
	INTERNAL_KEY_NULL,
	INTERNAL_KEY_FUNCTION,
	INTERNAL_KEY_THIS_OBJECT,
	INTERNAL_KEY_BUILTIN_FUNCTION
} InternalKey;

typedef struct {
	InternalKey key;
	void* value;
} Internal;

DEFINE_LIST(Internal)

typedef struct ValueFieldList ValueFieldList;

typedef struct {
	ValueFieldList* fields;
	InternalList internals;
} Value;

Result getValueInternal(Value value, InternalKey key, void** output);
Result getValueField(Value value, String name, Value** output);
bool hasInternal(Value value, InternalKey key);

typedef struct {
	String name;
	Value value;
} ValueField;

DEFINE_LIST(ValueField)
DEFINE_LIST(Value)
DEFINE_LIST(IfExpression)

#endif
