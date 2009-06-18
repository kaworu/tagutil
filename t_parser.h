#ifndef T_PARSER_H
#define T_PARSER_H
/*
 * t_parser.h
 *
 * a LL(1) recursive descend parser for tagutil.
 * used by the filter function.
 *
 *
 * tagutil's filter grammar:
 *
 * Filter       ::= <Condition>
 * Condition    ::= <Condition> ( '||' | '&&' ) <Condition>
 * Condition    ::= <Value> ( '==' | '<' | '<=' | '>' | '>=' | '!=' ) <Value>
 * Condition    ::= <Value> ( '=~' | '!~' ) <REGEX>
 * Condition    ::= <REGEX> ( '=~' | '!~' ) <Value>
 * Condition    ::= '!' '(' <Condition> ')'
 * Condition    ::= '(' <Condition> ')'
 * Value        ::= <TAGKEY> | <KEYWORD> | <INTEGER> | <DOUBLE> | <STRING>
 * TAGKEY       ::= <SIMPLETAGKEY> | <BRACETAGKEY>
 * SIMPLETAGKEY ::= '%' [A-Za-z\-_]+
 * BRACETAGKEY  ::= '%{' ( '\' [^=] | [^}=] )* ( '=' [0-9]+ )? '}'
 * KEYWORD      ::= 'filename' | 'undef' | 'backend'
 * INTEGER      ::= ( '+' | '-' )? '0' | [1-9][0-9]*
 * DOUBLE       ::= <INTEGER>? '.' [0-9]+
 * STRING       ::= '"' ('\' . | [^"] )* '"'
 * REGEX        ::= '/' ('\' . | [^/] )* '/' <REGOPTS>?
 * REGOPTS      ::= 'i' | 'm' | 'im' | 'mi'
 */
#include "t_config.h"
#include "t_lexer.h"


struct ast {
    int start, end;
    struct ast *lhs, *rhs;
    struct token *token;
};


/*
 * free given ast recursivly (victim can be NULL).
 */
void destroy_ast(struct ast *restrict victim);

/*
 * parse the filter with the given lexer.
 * free the given lexer after parsing.
 *
 * returned value has to be free()d (use destroy_ast).
 */
_t__nonnull(1)
struct ast * parse_filter(struct lexer *restrict L);

#endif /* not T_PARSER_H */
