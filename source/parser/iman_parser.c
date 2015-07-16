/*
 * iman - instruction set manual utility
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#include "../iman.h"
#include "iman_lexer.h"
#include "iman_parser.h"

#define IMAN_REFERENCE_DESC_BASE_SIZE 2048

struct iman_parser_field_handler {
    const char *name;
    
    int (*parse)(struct iman_parser *parser, unsigned int depth);
};

static int iman_parser_read_term(struct iman_parser *parser);
static int iman_parser_read_field(struct iman_parser *parser, unsigned int depth);

static int iman_parser_handle_forms(struct iman_parser *parser, unsigned int depth);
static int iman_parser_handle_description(struct iman_parser *parser, unsigned int depth);
static int iman_parser_handle_exceptions(struct iman_parser *parser, unsigned int depth);
static int iman_parser_handle_flags(struct iman_parser *parser, unsigned int depth);
static int iman_parser_handle_operation(struct iman_parser *parser, unsigned int depth);
static int iman_parser_handle_meta(struct iman_parser *parser, unsigned int depth);

static const struct iman_parser_field_handler iman_major_field_handler_table[] = {
    { "forms",       &iman_parser_handle_forms       },
    { "description", &iman_parser_handle_description },
    { "exceptions",  &iman_parser_handle_exceptions  },
    { "flags",       &iman_parser_handle_flags       },
    { "operation",   &iman_parser_handle_operation   },
    { "meta",        &iman_parser_handle_meta        },
    
    { NULL, NULL }
};

int iman_parser_initialise(FILE *source, struct iman_parser *parser) {
    memset(parser, 0, sizeof(*parser));
    parser->lexer.source = source;
    
    return IMAN_TRUE;
}

int iman_parser_read_block(struct iman_parser *parser) {
    unsigned int term_count = 0;
    
    for (;; ++term_count) {
        if (iman_parser_read_term(parser) != IMAN_TRUE) {
            if (parser->status == IMAN_PARSER_STATUS_SUCCESS)
                break;
            
            return IMAN_FALSE;
        }
    }
    
    if (term_count == 0) {
        printf("Error (L%u: C%u): expected at least one term definition but didn't get any.\n", parser->lexer.pos.line, parser->lexer.pos.column + 1);
        
        parser->status = IMAN_PARSER_STATUS_ERROR;
        return IMAN_FALSE;
    }
    
    for (;;) {
        if (iman_parser_read_field(parser, 1) != IMAN_TRUE) {
            if (parser->status == IMAN_PARSER_STATUS_SUCCESS)
                break;
            
            return IMAN_FALSE;
        }
    }
    
    return IMAN_TRUE;
}

static int iman_parser_read_term(struct iman_parser *parser) {
    char *definition_title = NULL;
    unsigned int title_length = 0;
    struct iman_reference_term_definition * new_def;
    
    if (iman_lexer_expect_line_start(&parser->lexer) != IMAN_TRUE) {
        printf("Error (L%u: C%u): expected to start out on a new line but didn't.\n", parser->lexer.pos.line, parser->lexer.pos.column + 1);
        
        parser->status = IMAN_PARSER_STATUS_ERROR;
        return IMAN_FALSE;
    }
    
    if (iman_lexer_expect_indent(&parser->lexer, 0) != IMAN_TRUE) {
        return IMAN_FALSE;
    }
    
    new_def = malloc(sizeof(struct iman_reference_term_definition));
    memset(new_def, 0, sizeof(struct iman_reference_term_definition));
    
    new_def->next = parser->block.terms;
    parser->block.terms = new_def;
    
    do {
        char *name = NULL;
        
        if (iman_lexer_expect_name(&parser->lexer, &name) != IMAN_TRUE) {
            printf("Error (L%u: C%u): expected a name\n", parser->lexer.pos.line, parser->lexer.pos.column + 1);
            
            parser->status = IMAN_PARSER_STATUS_ERROR;
            return IMAN_FALSE;
        }
        
        printf("Name (L%u: C%u): %s\n", parser->lexer.pos.line, parser->lexer.pos.column + 1, name);
        
        if (parser->block.terms->name_count >= IMAN_REFERENCE_TERM_MAX_NAMES) {
            printf("Error (L%u: C%u): attempted to add one too many term aliases, the maximum being %d. The offender is: %s.\n", 
                   parser->lexer.pos.line, 
                   parser->lexer.pos.column + 1, 
                   parser->block.terms->name_count, 
                   name
            );
            
            free(name);
            parser->status = IMAN_PARSER_STATUS_ERROR;
            return IMAN_FALSE;
        }
        
        parser->block.terms->names[parser->block.terms->name_count++] = name;
        
    } while(iman_lexer_accept_syntax(&parser->lexer, '/') != IMAN_FALSE);
    
    if (iman_lexer_accept_syntax(&parser->lexer, '=') != IMAN_TRUE) {
        printf("Error (L%u: C%u): expected an equals sign (=) followed by the term definition\n", parser->lexer.pos.line, parser->lexer.pos.column + 1);
        
        parser->status = IMAN_PARSER_STATUS_ERROR;
        return IMAN_FALSE;
    }
    
    if (iman_lexer_consume_remaining(&parser->lexer, &definition_title, &title_length) != IMAN_TRUE) {
        printf("Error (L%u: C%u): expected the term's definition title.\n", parser->lexer.pos.line, parser->lexer.pos.column + 1);
        
        parser->status = IMAN_PARSER_STATUS_ERROR;
        return IMAN_FALSE;
    }
    
    printf("Title (L%u: C%u): %s\n", parser->lexer.pos.line, parser->lexer.pos.column + 1,  definition_title);
    
    memcpy(parser->block.terms->title, definition_title, title_length);
    
    /* definition_title is a static allocation, no need to free it */
    
    return IMAN_TRUE;
}

static int iman_parser_read_field(struct iman_parser *parser, unsigned int depth) {
    const struct iman_parser_field_handler *field_handler;
    unsigned int name_length = 0;
    char * name = NULL;
    
    if (iman_lexer_expect_line_start(&parser->lexer) != IMAN_TRUE) {
        printf("Error (L%u: C%u): expected to start on a new line but didn't.\n", parser->lexer.pos.line, parser->lexer.pos.column + 1);
        
        parser->status = IMAN_PARSER_STATUS_ERROR;
        return IMAN_FALSE;
    }
    
    if (iman_lexer_expect_indent(&parser->lexer, depth) != IMAN_TRUE) {
        return IMAN_FALSE;
    }
    
    if (iman_lexer_expect_keyword(&parser->lexer, &name, &name_length) != IMAN_TRUE) {
        printf("Error (L%u: C%u): expected a field name but didn't get it.\n", parser->lexer.pos.line, parser->lexer.pos.column + 1);
        
        parser->status = IMAN_PARSER_STATUS_ERROR;
        return IMAN_FALSE;
    }
    
    name[name_length] = '\0';
    printf("Field name (L%u: C%u): %s\n", parser->lexer.pos.line, parser->lexer.pos.column + 1, name);
    
    for (field_handler = iman_major_field_handler_table; field_handler->name != NULL; ++field_handler) {
        if (strcmp(field_handler->name, name) == 0) {
            return field_handler->parse(parser, depth + 1);
        }
    }
    
    printf("Error (L%u: C%u): this field name (%s) isn't recognised.\n", parser->lexer.pos.line, parser->lexer.pos.column + 1, name);
    
    parser->status = IMAN_PARSER_STATUS_ERROR;
    return IMAN_FALSE;
}

static int iman_parser_handle_forms(struct iman_parser *parser, unsigned int depth) {
    while(iman_lexer_expect_line_start(&parser->lexer) == IMAN_TRUE) {
        char *line = NULL;
        unsigned int line_length = 0;
        
        if (iman_lexer_accept_indent(&parser->lexer, depth) != IMAN_TRUE)
            break;
        
        if (iman_lexer_consume_remaining(&parser->lexer, &line, &line_length) != IMAN_TRUE) {
            printf("Error (L%u: C%u): expected a line of text, with an indent depth of at least %d tabs.\n", parser->lexer.pos.line, parser->lexer.pos.column + 1, depth);
            
            parser->status = IMAN_PARSER_STATUS_ERROR;
            return IMAN_FALSE;
        }
        
        printf("Form line (L%u: C%u): %s\n", parser->lexer.pos.line, parser->lexer.pos.column + 1, line);
    }
    
    return IMAN_TRUE;
}

static int iman_parser_handle_description(struct iman_parser *parser, unsigned int depth) {
    if (parser->block.desc.buffer != NULL) {
        printf("Error (L%u: C%u): redeclaration of the description.\n", parser->lexer.pos.line, parser->lexer.pos.column + 1);
        
        parser->status = IMAN_PARSER_STATUS_ERROR;
        return IMAN_FALSE;
    }
    
    parser->block.desc.buffer = malloc(IMAN_REFERENCE_DESC_BASE_SIZE);
    parser->block.desc.size = IMAN_REFERENCE_DESC_BASE_SIZE;
    parser->block.desc.offset = 0;
    memset(parser->block.desc.buffer, 0, IMAN_REFERENCE_DESC_BASE_SIZE);
    
    while(iman_lexer_expect_line_start(&parser->lexer) == IMAN_TRUE) {
        char *line = NULL;
        unsigned int line_length = 0;
        
        if (iman_lexer_accept_indent(&parser->lexer, depth) != IMAN_TRUE)
            break;
        
        if (iman_lexer_consume_remaining(&parser->lexer, &line, &line_length) != IMAN_TRUE) {
            printf("Error (L%u: C%u): expected a line of text, with an indent depth of at least %d tabs.\n", parser->lexer.pos.line, parser->lexer.pos.column + 1, depth);
            
            parser->status = IMAN_PARSER_STATUS_ERROR;
            return IMAN_FALSE;
        }
        
        printf("Description line (L%u: C%u): %s\n", parser->lexer.pos.line, parser->lexer.pos.column + 1, line);
        
        /* The additional two bytes are for the newline and null terminator */
        if ((parser->block.desc.offset + line_length + 1) > parser->block.desc.size) {
            unsigned int new_size = parser->block.desc.size * 2;
            char *new_block = malloc(new_size);
            
            memset(new_block, 0, new_size);
            memcpy(new_block, parser->block.desc.buffer, parser->block.desc.offset);
            
            free(parser->block.desc.buffer);
            
            parser->block.desc.buffer = new_block;
            parser->block.desc.size = new_size;
        }
        
        memcpy(&parser->block.desc.buffer[parser->block.desc.offset], line, line_length);
        
        parser->block.desc.offset += line_length + 1;
        parser->block.desc.buffer[parser->block.desc.offset - 1] = '\n';
    }
    
    return IMAN_TRUE;
}

static int iman_parser_handle_exceptions(struct iman_parser *parser, unsigned int depth) {
    while(iman_lexer_expect_line_start(&parser->lexer) == IMAN_TRUE) {
        char *line = NULL;
        unsigned int line_length = 0;
        
        if (iman_lexer_accept_indent(&parser->lexer, depth) != IMAN_TRUE)
            break;
        
        if (iman_lexer_consume_remaining(&parser->lexer, &line, &line_length) != IMAN_TRUE) {
            printf("Error (L%u: C%u): expected a line of text, with an indent depth of at least %d tabs.\n", parser->lexer.pos.line, parser->lexer.pos.column + 1, depth);
            
            parser->status = IMAN_PARSER_STATUS_ERROR;
            return IMAN_FALSE;
        }
        
        printf("Exceptions line (L%u: C%u): %s\n", parser->lexer.pos.line, parser->lexer.pos.column + 1, line);
    }
    
    return IMAN_TRUE;
}

static int iman_parser_handle_flags(struct iman_parser *parser, unsigned int depth) {
    while(iman_lexer_expect_line_start(&parser->lexer) == IMAN_TRUE) {
        char *line = NULL;
        unsigned int line_length = 0;
        
        if (iman_lexer_accept_indent(&parser->lexer, depth) != IMAN_TRUE)
            break;
        
        if (iman_lexer_consume_remaining(&parser->lexer, &line, &line_length) != IMAN_TRUE) {
            printf("Error (L%u: C%u): expected a line of text, with an indent depth of at least %d tabs.\n", parser->lexer.pos.line, parser->lexer.pos.column + 1, depth);
            
            parser->status = IMAN_PARSER_STATUS_ERROR;
            return IMAN_FALSE;
        }
        
        printf("Flags line (L%u: C%u): %s\n", parser->lexer.pos.line, parser->lexer.pos.column + 1, line);
    }
    
    return IMAN_TRUE;
}

static int iman_parser_handle_operation(struct iman_parser *parser, unsigned int depth) {
    while(iman_lexer_expect_line_start(&parser->lexer) == IMAN_TRUE) {
        char *line = NULL;
        unsigned int line_length = 0;
        
        if (iman_lexer_accept_indent(&parser->lexer, depth) != IMAN_TRUE)
            break;
        
        if (iman_lexer_consume_remaining(&parser->lexer, &line, &line_length) != IMAN_TRUE) {
            printf("Error (L%u: C%u): expected a line of text, with an indent depth of at least %d tabs.\n", parser->lexer.pos.line, parser->lexer.pos.column + 1, depth);
            
            parser->status = IMAN_PARSER_STATUS_ERROR;
            return IMAN_FALSE;
        }
        
        printf("Operation line (L%u: C%u): %s\n", parser->lexer.pos.line, parser->lexer.pos.column + 1, line);
    }
    
    return IMAN_TRUE;
}

static int iman_parser_handle_meta(struct iman_parser *parser, unsigned int depth) {
    while(iman_lexer_expect_line_start(&parser->lexer) == IMAN_TRUE) {
        char *line = NULL;
        unsigned int line_length = 0;
        
        if (iman_lexer_accept_indent(&parser->lexer, depth) != IMAN_TRUE)
            break;
        
        if (iman_lexer_consume_remaining(&parser->lexer, &line, &line_length) != IMAN_TRUE) {
            printf("Error (L%u: C%u): expected a line of text, with an indent depth of at least %d tabs.\n", parser->lexer.pos.line, parser->lexer.pos.column + 1, depth);
            
            parser->status = IMAN_PARSER_STATUS_ERROR;
            return IMAN_FALSE;
        }
        
        printf("meta line (L%u: C%u): %s\n", parser->lexer.pos.line, parser->lexer.pos.column + 1, line);
    }
    
    return IMAN_TRUE;
}