/*
 * iman - instruction set manual utility
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#ifndef _IMAN_PARSER_H
#define _IMAN_PARSER_H

enum iman_parser_status {
    IMAN_PARSER_STATUS_SUCCESS = 0,
    IMAN_PARSER_STATUS_ERROR
};

struct iman_parser {
    struct iman_lexer lexer;
    
    enum iman_parser_status status;
    
    struct iman_reference_block block;
};

int iman_parser_initialise(struct iman_parser *parser, const char *filename);

void iman_parser_release(struct iman_parser *parser);

int iman_parser_is_eof(struct iman_parser *parser);

int iman_parser_read_block(struct iman_parser *parser);

#endif