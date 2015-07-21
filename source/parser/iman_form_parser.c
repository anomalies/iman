/*
 * iman - instruction set manual utility
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#include "../iman.h"
#include "iman_reference.h"
#include "iman_form_parser.h"

struct iman_form_lexer {
    char *line;
    unsigned int column;
};

static void iman_form_lexer_skip_whitespace(struct iman_form_lexer *lexer);
static int iman_form_lexer_accept_syntax(struct iman_form_lexer *lexer, char syntax);
static int iman_form_parse_column(struct iman_form_lexer *lexer, struct iman_reference_form_definition *form, unsigned int column);

int iman_parse_form(char *line, struct iman_reference_form_definition *form) {
    struct iman_form_lexer lexer;
    unsigned int column_number = 0;
    
    lexer.line = line;
    lexer.column = 0;
    
    if (iman_form_lexer_accept_syntax(&lexer, '[') != IMAN_TRUE) {
        return IMAN_FALSE;
    }
    
    do {
        if (iman_form_parse_column(&lexer, form, column_number) != IMAN_TRUE)
            return IMAN_FALSE;
        
        column_number++;
    } while (iman_form_lexer_accept_syntax(&lexer, ',') == IMAN_TRUE);
    
    if (iman_form_lexer_accept_syntax(&lexer, ']') != IMAN_TRUE) {
        return IMAN_FALSE;
    }
    
    return IMAN_TRUE;
}

static void iman_form_lexer_skip_whitespace(struct iman_form_lexer *lexer) {
    for(; lexer->line[lexer->column] != '\0'; ++lexer->column) {
        char c = lexer->line[lexer->column];
        if (c != '\t' && c != ' ')
            return;
    }
    
    return;
}

static int iman_form_lexer_accept_syntax(struct iman_form_lexer *lexer, char syntax) {
    iman_form_lexer_skip_whitespace(lexer);
    
    if (lexer->line[lexer->column] == syntax) {
        ++lexer->column;
        return IMAN_TRUE;
    }
    
    return IMAN_FALSE;
}

static int iman_form_parse_column(struct iman_form_lexer *lexer, struct iman_reference_form_definition *form, unsigned int column) {
    if (iman_form_lexer_accept_syntax(lexer, '(') != IMAN_TRUE) {
        printf("Form error, c %d: expected an opening parenthesis, did you add one too many commas?\n", lexer->column);
        
        return IMAN_FALSE;
    }
    
    IMAN_UNUSED(form);
    IMAN_UNUSED(column);
    
    return IMAN_TRUE;
}