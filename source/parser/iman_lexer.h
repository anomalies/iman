/*
 * iman - instruction set manual utility
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#ifndef _IMAN_LEXER_H
#define _IMAN_LEXER_H

#define IMAN_LEXER_MAX_LINE_LENGTH 2048

struct iman_lexer {
    FILE *source;
    
    struct {
        unsigned int column;
        unsigned int line;
        unsigned int tabs;
    } pos;
    
    struct {
        unsigned int length;
        char line[IMAN_LEXER_MAX_LINE_LENGTH];
    } buffer;
};

int iman_lexer_expect_line_start(struct iman_lexer *lexer);
int iman_lexer_expect_name(struct iman_lexer *lexer, char **name);
int iman_lexer_expect_indent(struct iman_lexer *lexer, unsigned int depth);
int iman_lexer_accept_syntax(struct iman_lexer *lexer, char syntax);
int iman_lexer_accept_indent(struct iman_lexer *lexer, unsigned int depth);
int iman_lexer_expect_keyword(struct iman_lexer *lexer, char **keyword, unsigned int *length);
int iman_lexer_consume_remaining(struct iman_lexer *lexer, char **remaining, unsigned int *length);


#endif