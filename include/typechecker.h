#ifndef TYPECHECKER_H
#define TYPECHECKER_H

#include "list.h"

typedef struct Function Function;
typedef struct TypeDeclaration TypeDeclaration;

typedef union {
	char* identifier;
	Function* function;
	TypeDeclaration* typeDeclaration;
} TypeLiteralData;

typedef enum {
	TYPE_LITERAL_FUNCTION,
	TYPE_LITERAL_IDENTIFIER,
	TYPE_LITERAL_TYPE_DECLARATION,
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

typedef struct Expression Expression;
Result typeOf(Expression expression, Type* output);

DEFINE_LIST(Type)

#endif
