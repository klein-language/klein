/*
 * klein.h
 *
 * The header file for the Klein interpreter. This file contains public declarations
 * visible to anyone embedding Klein into a C project.
 */

#ifndef KLEIN_H
#define KLEIN_H

typedef struct BinaryExpression BinaryExpression;
typedef struct TypeDeclaration TypeDeclaration;
typedef struct Scope Scope;
typedef struct ExpressionList ExpressionList;
typedef struct StatementList StatementList;
typedef struct UnaryExpression UnaryExpression;
typedef struct Object Object;
typedef struct ForLoop ForLoop;
typedef struct WhileLoop WhileLoop;
typedef struct IfExpressionList IfExpressionList;
typedef struct Function Function;

#define DEFINE_KLEIN_LIST(type)                                      \
	typedef struct type##List type##List;                            \
	struct type##List {                                              \
		unsigned long size;                                          \
		unsigned long capacity;                                      \
		type* data;                                                  \
	};                                                               \
                                                                     \
	KleinResult empty##type##List(type##List* output);               \
	KleinResult emptyHeap##type##List(type##List** output);          \
	KleinResult appendTo##type##List(type##List* list, type value);  \
	KleinResult prependTo##type##List(type##List* list, type value); \
	int is##type##ListEmpty(type##List list);                        \
	KleinResult pop##type##List(type##List* list);                   \
	type getFrom##type##ListUnchecked(type##List list, unsigned long index);

typedef struct {
	char* errorMessage;
} KleinResult;

typedef enum {

	// Keywords
	TOKEN_TYPE_KEYWORD_AND,
	TOKEN_TYPE_KEYWORD_DO,
	TOKEN_TYPE_KEYWORD_ELSE,
	TOKEN_TYPE_KEYWORD_FOR,
	TOKEN_TYPE_KEYWORD_FUNCTION,
	TOKEN_TYPE_KEYWORD_IF,
	TOKEN_TYPE_KEYWORD_IN,
	TOKEN_TYPE_KEYWORD_LET,
	TOKEN_TYPE_KEYWORD_NOT,
	TOKEN_TYPE_KEYWORD_OR,
	TOKEN_TYPE_KEYWORD_TYPE,
	TOKEN_TYPE_KEYWORD_WHILE,
	TOKEN_TYPE_KEYWORD_RETURN,

	// Grouping
	TOKEN_TYPE_LEFT_BRACE,
	TOKEN_TYPE_LEFT_BRACKET,
	TOKEN_TYPE_LEFT_PARENTHESIS,
	TOKEN_TYPE_RIGHT_BRACE,
	TOKEN_TYPE_RIGHT_BRACKET,
	TOKEN_TYPE_RIGHT_PARENTHESIS,

	// Operators
	TOKEN_TYPE_ASTERISK,
	TOKEN_TYPE_FORWARD_SLASH,
	TOKEN_TYPE_LESS_THAN,
	TOKEN_TYPE_GREATER_THAN,
	TOKEN_TYPE_LESS_THAN_OR_EQUAL_TO,
	TOKEN_TYPE_GREATER_THAN_OR_EQUAL_TO,
	TOKEN_TYPE_DOUBLE_EQUALS,
	TOKEN_TYPE_NOT_EQUAL,
	TOKEN_TYPE_CARET,
	TOKEN_TYPE_COLON,
	TOKEN_TYPE_COMMA,
	TOKEN_TYPE_DOT,
	TOKEN_TYPE_EQUALS,
	TOKEN_TYPE_MINUS,
	TOKEN_TYPE_PLUS,
	TOKEN_TYPE_SEMICOLON,

	// Literals
	TOKEN_TYPE_STRING,
	TOKEN_TYPE_NUMBER,
	TOKEN_TYPE_IDENTIFIER,

	// Ignored
	TOKEN_TYPE_WHITESPACE,
	TOKEN_TYPE_COMMENT

} TokenType;

typedef struct {
	TokenType type;
	char* value;
} Token;

DEFINE_KLEIN_LIST(Token)

typedef union {
	char* identifier;
	Function* function;
	TypeDeclaration* typeDeclaration;
} TypeLiteralData;

typedef enum {
	TYPE_LITERAL_FUNCTION,
	TYPE_LITERAL_IDENTIFIER,
	TYPE_LITERAL_TYPE_DECLARATION
} TypeLiteralType;

typedef struct {
	TypeLiteralData data;
	TypeLiteralType type;
} TypeLiteral;

typedef struct TypeList TypeList;

typedef union {

	/**
	 * A union type, also known as an arithmetic sum type. This represents
	 * a choice between multiple types.
	 */
	TypeList* typeUnion;

	/**
	 * A single, literal type, such as a function, identifier, etc.
	 */
	TypeLiteral literal;

} TypeData;

typedef enum {
	TYPE_UNION,
	TYPE_LITERAL
} TypeType;

/*
 * The type of an expression. This is the highest level of typing,
 * which things like `let` declarations and function parameters store
 * as types.
 */
typedef struct {
	TypeData data;
	TypeType type;
} Type;

typedef struct {
	StatementList* statements;
	Scope* innerScope;
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
	char* name;

	/** The type of the parameter. */
	Type type;

} Parameter;

DEFINE_KLEIN_LIST(Parameter)

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

typedef enum {
	UNARY_OPERATION_NOT,
	UNARY_OPERATION_FUNCTION_CALL,
	UNARY_OPERATION_INDEX
} UnaryOperationType;

struct TypeDeclaration {
	ParameterList fields;
};

typedef union {
	/** A literal number expression. */
	double number;

	/** A block expression. */
	Block* block;
	int boolean;

	/** A literal number expression. */
	Function function;

	UnaryExpression* unary;

	char* identifier;

	/** A binary expression. */
	BinaryExpression* binary;

	TypeDeclaration typeDeclaration;

	Object* object;

	ForLoop* forLoop;

	WhileLoop* whileLoop;

	ExpressionList* list;

	char* string;

	IfExpressionList* ifExpression;
} ExpressionData;

struct Expression {
	ExpressionType type;
	ExpressionData data;
};

DEFINE_KLEIN_LIST(Expression)

struct BinaryExpression {
	Expression left;
	BinaryOperation operation;
	Expression right;
};

typedef union {
	ExpressionList functionCall;
	Expression index;
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

DEFINE_KLEIN_LIST(Field)

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

DEFINE_KLEIN_LIST(Declaration);

typedef union {
	Declaration declaration;
	Expression expression;
	Expression returnExpression;
} StatementData;

typedef struct {
	StatementData data;
	StatementType type;
} Statement;

DEFINE_KLEIN_LIST(Statement)

typedef struct BinaryOperator BinaryOperator;
struct BinaryOperator {
	BinaryOperator* precedent;
	TokenType* tokenTypes;
	unsigned long tokenTypeCount;
};

struct ForLoop {
	char* binding;
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

DEFINE_KLEIN_LIST(IfExpression)

/**
 * A program's abstract syntax tree.
 */
typedef struct {
	/** The statements in the program. The elements in this list are of type `Statement`. */
	StatementList statements;
} Program;

/**
 * Tokenizes the given string of Klein source code into a list of tokens.
 * This is the first step of interpreting Klein code.
 *
 * # Parameters
 *
 * - `sourceCode` - The original, Klein source code. It may contain
 *   syntax and semantic errors and this function will still return `OK`
 *   as long as each individual token in the code is a valid Klein token.
 *   It mustn't be `NULL`.
 *
 * - `output` - Where to store the resulting `TokenList`. It must point to
 *   some memory (be it stack or heap) that already has enough space to hold
 *   a `TokenList`; i.e. it could be the address of stack token or the result of
 *   `malloc(sizeof(TokenList))`, but it can't be `NULL`, or an error will be returned.
 *   The outputted tokens have copied strings from the original source code (because
 *   they need to be null-terminated and the original source code is contiguous),
 *   so it's not dependent on `sourceCode` being valid; i.e., if the string stored at
 *   `sourceCode` is freed, `output` will still be valid.
 *
 * # Errors
 *
 * If memory for the token list fails to allocate, an error is returned.
 * If the source code contains unrecognized tokens, an error is returned.
 * If the given `sourceCode` or `output` is `NULL`, an error is returned.
 */
KleinResult tokenizeKlein(char* sourceCode, TokenList* output);

KleinResult parseKlein(char* code, Program* output);
KleinResult parseKleinExpression(char* code, Expression* output);

KleinResult runKlein(char* code);

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

DEFINE_KLEIN_LIST(Internal)

typedef struct ValueFieldList ValueFieldList;

typedef struct {
	ValueFieldList* fields;
	InternalList internals;
} Value;

DEFINE_KLEIN_LIST(Value);

typedef struct {
	char* name;
	Value value;
} ValueField;

DEFINE_KLEIN_LIST(ValueField);

#endif
