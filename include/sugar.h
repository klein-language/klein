#ifndef SUGAR_H
#define SUGAR_H

#include "parser.h"
#include "result.h"

/**
 * Converts a C String (null-terminated `char*`) into a Klein string literal.
 *
 * The opposite of this function is `getString()`, which converts a Klein string
 * literal expression back into a C string. In other words, `getString()` can be
 * used on the output of `stringExpression()` to get the original string back.
 *
 * This is the string equivalent to `numberExpression()`.
 *
 * # Parameters
 *
 * - `value` - The C string to convert. It must live at least as long as
 *   `output`, otherwise accessing output is undefined behavior.
 *
 * - `output`- The place to store the created Klein `Expression`. It is only
 *   valid as long as `value` is valid.
 *
 * # Errors
 *
 * If memory fails to allocate, an error is returned.
 * If built-in properties on Strings such as `length()` fail to be found, an
 * error is returned.
 */
Result stringExpression(String value, Expression* output);

/**
 * Takes a string value in Klein as an expression and retrieves the underlying
 * C string in it.
 *
 * # Parameters
 *
 * - `expression` - The string expression. If this is any expression other than
 *   a string, an error is returned.
 *
 * - `output` - The C string stored in the given expression. It points to the same
 *   string originally stored in the expression, most likely through `stringExpression()`,
 *   so it is only valid as long as that string is valid.
 */
Result getString(Expression expression, String** output);

bool isString(Expression expression);
bool isNumber(Expression expression);

/**
 * Converts a C number (`double`) into a Klein number literal.
 *
 * The opposite of this function is `getNumber()`, which converts a Klein number
 * literal expression back into a double. In other words, `getNumber()` can be
 * used on the output of `numberExpression()` to get the original number back.
 *
 * The internal double will be stored on the heap, and the caller is responsible
 * for freeing it.
 *
 * This is the number equivalent to `stringExpression()`.
 *
 * # Parameters
 *
 * - `value` - The number to convert.
 *
 * - `output`- The place to store the created Klein `Expression`.
 *
 * # Errors
 *
 * If memory fails to allocate, an error is returned.
 * If built-in properties on numbers such as `abs()` fail to be found, an
 * error is returned.
 */
Result numberExpression(double value, Expression* output);

/**
 * Takes a number in Klein as an expression and retrieves the underlying
 * double in it.
 *
 * # Parameters
 *
 * - `expression` - The number expression. If this is any expression other than
 *   a number, an error is returned.
 *
 * - `output` - The double stored in the given expression. It points to a double
 *   on the heap, so it's valid as long as the expression hasnt't explicitly
 *   freed it.
 */
Result getNumber(Expression expression, double** output);

Result listExpression(ExpressionList values, Expression* output);
Result getList(Expression expression, ExpressionList** output);
bool isList(Expression expression);

#define FIELD(name, value) \
	(Field) { .name = name, .value = value }

#define OBJECT(...) \
	do {            \
	} while (0)

#endif
