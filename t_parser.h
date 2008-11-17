#ifndef T_PARSER_H
#define T_PARSER_H
/*
 * t_parser.c
 *
 * a LL(1) recursive descend parser for tagutil.
 * used by the filter function.
 *
 *
 * tagutil's filter grammar:
 *
 * Filter     ::= <Condition>
 * Condition  ::= <Condition> ( '||' | '&&' ) <Condition>
 * Condition  ::= <IntKeyword> ( '==' | '<' | '<=' | '>' | '>=' | '!=' ) <INTEGER>
 * Condition  ::= <StrKeyword> ( '==' | '<' | '<=' | '>' | '>=' | '!=' ) <STRING>
 * Condition  ::= <StrKeyword> ( '==' | '<' | '<=' | '>' | '>=' | '!=' ) <StrKeyword>
 * Condition  ::= <StrKeyword> '=~' <REGEX>
 * Condition  ::= '!' '(' <Condition> ')'
 * Condition  ::=     '(' <Condition> ')'
 * IntKeyword ::= 'track' | 'year'
 * StrKeyword ::= 'title' | 'album' | 'artist' | 'comment' | 'genre'
 * INTEGER    ::= [0-9]+
 * STRING     ::= '"' ('\' . | [^"] )* '"'
 * REGEX      ::= '/' ('\' . | [^/] )* '/' <REGOPTS>?
 * REGOPTS    ::= 'i' | 'm' | 'im' | 'mi'
 */
#include "t_config.h"

#include <regex.h>

#include "t_lexer.h"


enum astkind {
    ANODE,
    ALEAF,
};

struct ast {
    enum astkind kind;
    struct ast *lhs, *rhs; /* defined if kind == ANODE */

    enum tokenkind tkind;
    union {
        char *string;   /* defined if tkind == TSTRING */
        int integer;    /* defined if tkind == TINT    */
        regex_t regex;  /* defined if tkind == TREGEX  */
    } value;
};


/*
 * free given ast recursivly.
 */
void destroy_ast(struct ast *restrict victim);

/*
 * parse the filter with the given lexer.
 * free the given lexer after parsing.
 *
 * returned value has to be freed (use destroy_ast)
 */
__t__nonnull(1)
struct ast * parse_filter(struct lexer *restrict L);

#endif /* not T_PARSER_H */
