/*
 * iman - instruction set manual utility
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#include "../iman.h"
#include "iman_lexer.h"

#define IMAN_LEXER_DEFAULT_TEXTBLOCK_SIZE 4096

static int iman_lexer_read_line(struct iman_lexer *lexer);

int iman_lexer_expect_line_start(struct iman_lexer *lexer) {
    if (lexer->pos.column >= lexer->buffer.length) {
        if (iman_lexer_read_line(lexer) != IMAN_TRUE)
            return IMAN_FALSE;
        
        /* Obviously a new line */
        return IMAN_TRUE;
    }
    
    return (lexer->pos.column <= lexer->pos.tabs) ? IMAN_TRUE : IMAN_FALSE;
}

int iman_lexer_expect_name(struct iman_lexer *lexer, char **name) {
    unsigned int start_column, name_length;
    char * new_text;
    
    for (start_column = lexer->pos.column; lexer->pos.column < lexer->buffer.length; ++lexer->pos.column) {
        char c = lexer->buffer.line[lexer->pos.column];
        
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
            continue;
        
        break;
    }
    
    if (lexer->pos.column == start_column) {
        return IMAN_FALSE;
    }
    
    name_length = lexer->pos.column - start_column;
    
    new_text = malloc(name_length + 1);
    memcpy(new_text, &lexer->buffer.line[start_column], name_length);
    new_text[name_length] = '\0';
    
    *name = new_text;
    return IMAN_TRUE;
}


int iman_lexer_expect_indent(struct iman_lexer *lexer, unsigned int depth) {
    if (lexer->pos.column == 0) {
        unsigned int offset;
        
        for(offset = 0; lexer->buffer.line[offset] == '\t'; ++offset)
            ;
        
        if (offset == depth) {
            lexer->pos.column = offset;
            lexer->pos.tabs = offset;
            
            return IMAN_TRUE;
        }
    }
    
    /* TODO: Is this valid logic? */
    return IMAN_FALSE;
}

int iman_lexer_accept_syntax(struct iman_lexer *lexer, char syntax) {
    if (lexer->buffer.line[lexer->pos.column] == syntax) {
        ++lexer->pos.column;
        return IMAN_TRUE;
    }
    
    return IMAN_FALSE;
}

int iman_lexer_accept_indent(struct iman_lexer *lexer, unsigned int depth) {
    if (lexer->pos.column == 0) {
        unsigned int offset;
        
        for(offset = 0; offset < depth; ++offset) {
            if (lexer->buffer.line[offset] != '\t')
                return IMAN_FALSE;
            
        }
        
        lexer->pos.column = offset;
        lexer->pos.tabs = offset;
        
        return IMAN_TRUE;
    }
    
    /* TODO: Is this valid logic? */
    return IMAN_FALSE;
}

int iman_lexer_expect_keyword(struct iman_lexer *lexer, char **keyword, unsigned int *length) {
    unsigned int start_column;
    
    for (start_column = lexer->pos.column; lexer->pos.column < lexer->buffer.length; ++lexer->pos.column) {
        char c = lexer->buffer.line[lexer->pos.column];
        
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
            continue;
        
        break;
    }
    
    if (lexer->pos.column == start_column) {
        return IMAN_FALSE;
    }
    
    *keyword = &lexer->buffer.line[start_column];
    *length = lexer->pos.column - start_column;
    
    return IMAN_TRUE;
}

int iman_lexer_consume_remaining(struct iman_lexer *lexer, char **remaining, unsigned int *length) {
    *remaining = &lexer->buffer.line[lexer->pos.column];
    *length = lexer->buffer.length - lexer->pos.column;
    lexer->pos.column = lexer->buffer.length;
    return IMAN_TRUE;
}

static int iman_lexer_read_line(struct iman_lexer *lexer) {
    unsigned int offset;
    
    lexer->pos.column = 0;
    lexer->pos.tabs = 0;
    lexer->pos.line++;
    
    if (fgets(lexer->buffer.line, IMAN_LEXER_MAX_LINE_LENGTH, lexer->source) == NULL) {
        return IMAN_FALSE;
    }
    
    for (offset = 0; offset < IMAN_LEXER_MAX_LINE_LENGTH; ++offset) {
        if (lexer->buffer.line[offset] == '\n') {
            lexer->buffer.line[offset] = '\0';
            
            lexer->buffer.length = offset;
            return IMAN_TRUE;
        }
    }
    
    /* TODO: Make a fuss about being larger than the maximum line length */
    return IMAN_FALSE;
}