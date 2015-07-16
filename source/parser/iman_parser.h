/*
 * iman - instruction set manual utility
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#ifndef _IMAN_PARSER_H
#define _IMAN_PARSER_H

#define IMAN_REFERENCE_TERM_MAX_NAMES 8
#define IMAN_REFERENCE_TITLE_MAX_SIZE 256

enum iman_parser_status {
    IMAN_PARSER_STATUS_SUCCESS = 0,
    IMAN_PARSER_STATUS_ERROR
};

struct iman_reference_term_definition {
    struct iman_reference_term_definition *next;
    
    unsigned int name_count;
    char *names[IMAN_REFERENCE_TERM_MAX_NAMES];
    
    char title[IMAN_REFERENCE_TITLE_MAX_SIZE];
};

struct iman_reference_block {
    struct iman_reference_term_definition *terms;
    
    struct {
        char *buffer;
        unsigned int size;
        unsigned int offset;
    } desc;
    
    struct iman_reference_block * next_block;
};

struct iman_parser {
    struct iman_lexer lexer;
    
    enum iman_parser_status status;
    
    struct iman_reference_block block;
};

int iman_parser_initialise(FILE *source, struct iman_parser *parser);

int iman_parser_read_block(struct iman_parser *parser);

#endif