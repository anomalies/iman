/*
 * iman - instruction set manual utility
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#include "../iman.h"
#include "iman_parser.h"

#define IMAN_PARSER_BUFFER_SIZE 4096
#define IMAN_REFERENCE_DEFAULT_NAME_SIZE 4

enum iman_parser_status {
    IMAN_PARSER_STATUS_SUCCESS = 0,
    IMAN_PARSER_STATUS_ERROR
};

struct iman_parser_state {
    const struct iman_parser_options *options;
    struct iman_reference_source *refsource;
    
    struct {
        unsigned int line;
        unsigned int column;
        unsigned int tab_depth;
    } position;
    
    char * current_line;
    unsigned int line_length;
    
    enum iman_parser_status status;
    
    struct iman_reference_source_definition * active_definition;
};

static char line_buffer[IMAN_PARSER_BUFFER_SIZE];

/*
 * Basic format is:
 * <block start>
 * \tinner
 * \t\tinner child
 * 
 * <next block start>
 */

static int iman_parser_consume_block(struct iman_parser_state *state);
static int iman_parser_read_line(struct iman_parser_state *state);
static int iman_parser_consume_term_declaration(struct iman_parser_state *state);
static int iman_parser_expect_identifier(struct iman_parser_state *state, char **identifier);
static int iman_parser_accept_syntax(struct iman_parser_state *state, char syntax);

static void iman_parse_reference_add_name(struct iman_reference_source_definition *definition, char *name);
static int iman_parser_is_eol(struct iman_parser_state *state);

static int iman_field_forms_handler(struct iman_parser_state *state);
static int iman_field_description_handler(struct iman_parser_state *state);
static int iman_field_exceptions_handler(struct iman_parser_state *state);
static int iman_field_flags_handler(struct iman_parser_state *state);
static int iman_field_operation_handler(struct iman_parser_state *state);
static int iman_field_meta_handler(struct iman_parser_state *state);

struct iman_major_field_handler {
    const char * name;
    
    int (*handle)(struct iman_parser_state *state);
} major_field_handlers[] = {
    { "forms",       &iman_field_forms_handler       },
    { "description", &iman_field_description_handler },
    { "exceptions",  &iman_field_exceptions_handler  },
    { "flags",       &iman_field_flags_handler       },
    { "operation",   &iman_field_operation_handler   },
    { "meta",        &iman_field_meta_handler        },
    { NULL, NULL                                     }
};

int iman_parse_reference_source(const struct iman_parser_options *options, struct iman_reference_source *refsource) {
    struct iman_parser_state parser_state;
    
    parser_state.options = options;
    parser_state.refsource = refsource;
    
    parser_state.position.line = 0;
    parser_state.position.column = 0;
    
    parser_state.status = IMAN_PARSER_STATUS_SUCCESS;
    
    refsource->head = NULL;
    refsource->tail = &refsource->head;
    
    while(iman_parser_consume_block(&parser_state) != IMAN_FALSE)
        ;
    
    return parser_state.status == IMAN_PARSER_STATUS_SUCCESS ? IMAN_TRUE : IMAN_FALSE;
}

int iman_reference_source_free(struct iman_reference_source *refsource) {
    IMAN_UNUSED(refsource);
    return IMAN_TRUE;
}

static int iman_parser_consume_block(struct iman_parser_state *state) {    
    struct iman_reference_source_definition * new_definition;
    
    if (iman_parser_read_line(state) != IMAN_TRUE)
        return IMAN_FALSE;
    
    /* Add a new block item to the state */
    new_definition = malloc(sizeof(struct iman_reference_source_definition));
    memset(new_definition, 0, sizeof(struct iman_reference_source_definition));
    
    *state->refsource->tail = new_definition;
    state->refsource->tail = &new_definition->next_definition;
    state->refsource->count++;
    
    state->active_definition = new_definition;
    
    if (state->position.tab_depth != 0) {
        fprintf(stderr, "Error (L%u:C%u): this shouldn't be indented. A definition block must begin with an unintented term declaration.\n",
            state->position.line,
            state->position.column + 1
        );
        
        state->status = IMAN_PARSER_STATUS_ERROR;
        return IMAN_FALSE;
    }
    
    do {
        if (iman_parser_consume_term_declaration(state) != IMAN_TRUE)
            return IMAN_FALSE;
        
        if (iman_parser_read_line(state) != IMAN_TRUE) {
            if (state->status == IMAN_PARSER_STATUS_SUCCESS) {
                fprintf(stderr, "Error (L%u:C%u): expected either a term declaration or a block definition.\n",
                        state->position.line,
                        state->position.column + 1
                );
                
                state->status = IMAN_PARSER_STATUS_ERROR;
            }
            
            return IMAN_FALSE;
        }
        
    } while(state->position.tab_depth == 0);
    
    /* TODO: Parse the block */
    
    do {
        char * field_identifier = NULL;
        
        if (iman_parser_expect_identifier(state, &field_identifier) != IMAN_TRUE) {
            fprintf(stderr, "Error (L%u:C%u): expected a block header; e.g. 'forms' or 'definition'\n",
                    state->position.line,
                    state->position.column + 1
            );
            
            state->status = IMAN_PARSER_STATUS_ERROR;
            return IMAN_FALSE;
        }
        
        if (iman_parser_is_eol(state) == IMAN_FALSE) {
            fprintf(stderr, "Error (L%u:C%u): expected a block header; e.g. 'forms' or 'definition' followed immediately by a new-line\n",
                    state->position.line,
                    state->position.column + 1
            );
            
            state->status = IMAN_PARSER_STATUS_ERROR;
            return IMAN_FALSE;
        }
        
        /* TODO: parse field's contents */
        
        fprintf(state->options->output, "(L%u:C%u): Reached block field %s\n",
                state->position.line,
                state->position.column + 1,
                field_identifier);
        
        
        state->status = IMAN_PARSER_STATUS_ERROR;
        return IMAN_FALSE;
    } while(state->position.tab_depth == 1);
    
    return IMAN_TRUE;
}

static int iman_parser_read_line(struct iman_parser_state *state) {
    int tabs, length;
    
    if (fgets(line_buffer, sizeof(line_buffer), state->options->source) == NULL) {
        state->current_line = NULL;
        return IMAN_FALSE;
    }
    
    state->position.line++;
    state->position.column = 0;
    
    
    /* Count the leading tabs */
    for (tabs = 0; line_buffer[tabs] == '\t'; ++tabs)
        ;
    
    state->position.tab_depth = tabs;
    state->position.column = tabs;
    state->current_line = line_buffer;
    
    /* Delete the trailing new-line */
    for (length = tabs; line_buffer[length] != '\0'; ++length) {
        if (line_buffer[length] == '\n') {
            line_buffer[length] = '\0';
            
            state->line_length = length;
            return IMAN_TRUE;
        }
    }
    
    /* If we reached here the input line was longer than the maximum allowable */
    fprintf(stderr, "Error: line %u of source exceeded the maximum allowable line length %d\n", 
            state->position.line, 
            IMAN_PARSER_BUFFER_SIZE
    );
    
    state->status = IMAN_PARSER_STATUS_ERROR;
    return IMAN_FALSE;
}

static int iman_parser_consume_term_declaration(struct iman_parser_state *state) {
    struct iman_reference_source_definition_name * new_name;
    unsigned int title_length;
    
    /*
     * <term>/<alias 1>/.../<alias n>=<title>
     */
    
    new_name = malloc(sizeof(struct iman_reference_source_definition_name));
    memset(new_name, 0, sizeof(struct iman_reference_source_definition_name));
    
    new_name->next_name = state->active_definition->names;
    new_name->name_count = IMAN_REFERENCE_DEFAULT_NAME_SIZE;
    new_name->names = malloc(sizeof(char *) * new_name->name_count);
    state->active_definition->names = new_name;
    
    for(;;) {
        char * identifier = NULL;
        
        if (iman_parser_expect_identifier(state, &identifier) != IMAN_TRUE) {
            fprintf(stderr, "Error (L%u: C%u): expected a valid definition name.\n",
                state->position.line,
                state->position.column + 1
            );
            
            state->status = IMAN_PARSER_STATUS_ERROR;
            return IMAN_FALSE;
        }
        
        iman_parse_reference_add_name(state->active_definition, identifier);
        
        if (iman_parser_accept_syntax(state, '/') == IMAN_TRUE) {
            /* Definition alias */
            
            continue;
        } else if (iman_parser_accept_syntax(state, '=') == IMAN_TRUE) {
            /* Assign the title */
            
            break;
        }
        
        fprintf(stderr, "Error (L%u, C%u): expected either a '/' for name aliases or an '=' to assign a title.\n",
            state->position.line,
            state->position.column + 1
        );
        
        state->status = IMAN_PARSER_STATUS_ERROR;
        return IMAN_FALSE;
    }
    
    title_length = state->line_length - state->position.column;
    /* The remainder of the string is the title */
    new_name->title = malloc(sizeof(char) * (title_length + 1));
    memcpy(new_name->title, &state->current_line[state->position.column], title_length);
    
    new_name->title[title_length] = '\0';
    state->position.column += title_length;
    
    return IMAN_TRUE;
}

static int iman_parser_expect_identifier(struct iman_parser_state *state, char **identifier) {
    unsigned int length, start_column = state->position.column;
    char * ident_buffer;
    
    while(state->current_line[state->position.column] != '\0') {
        char c = state->current_line[state->position.column];
        
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
            state->position.column++;
            continue;
        }
        
        break;
    }
    
    if (start_column == state->position.column) {
        return IMAN_FALSE;
    }
    
    length = state->position.column - start_column;
    ident_buffer = malloc(sizeof(char) * (length + 1));
    memcpy(ident_buffer, &state->current_line[start_column], length);
    
    ident_buffer[length] = '\0';
    *identifier = ident_buffer;
    
    return IMAN_TRUE;
}

static int iman_parser_accept_syntax(struct iman_parser_state *state, char syntax) {
    while(state->current_line[state->position.column] != '\0') {
        if (state->current_line[state->position.column] == ' ') {
            state->position.column++;
            continue;
        }
        
        break;
    }
    
    if (state->current_line[state->position.column] == syntax) {
        state->position.column++;
        return IMAN_TRUE;
    }
    
    return IMAN_FALSE;
}

static void iman_parse_reference_add_name(struct iman_reference_source_definition *definition, char *name) {
    struct iman_reference_source_definition_name * cur_name = definition->names;
    unsigned int x, new_count;
    char ** new_array;
    
    for(x = 0; x < cur_name->name_count; ++x) {
        if (cur_name->names[x] == NULL) {
            cur_name->names[x] = name;
            
            return;
        }
    }
    
    /* Realloc */
    new_count = cur_name->name_count * 2;
    new_array = malloc(sizeof(char *) * new_count);
    memset(new_array, 0, sizeof(char *) * new_count);
    
    for(x = 0; x < cur_name->name_count; ++x) {
        new_array[x] = cur_name->names[x];
    }
    
    new_array[x + 1] = name;
    
    free(cur_name->names);
    
    cur_name->name_count = new_count;
    cur_name->names = new_array;
    return;
}

static int iman_parser_is_eol(struct iman_parser_state *state) {
    return state->line_length > state->position.column ? IMAN_FALSE : IMAN_TRUE;
}

static int iman_field_forms_handler(struct iman_parser_state *state) {
    IMAN_UNUSED(state);
    return IMAN_FALSE;
}

static int iman_field_description_handler(struct iman_parser_state *state) {
    IMAN_UNUSED(state);
    return IMAN_FALSE;
}

static int iman_field_exceptions_handler(struct iman_parser_state *state) {
    IMAN_UNUSED(state);
    return IMAN_FALSE;
}

static int iman_field_flags_handler(struct iman_parser_state *state) {
    IMAN_UNUSED(state);
    return IMAN_FALSE;
}

static int iman_field_operation_handler(struct iman_parser_state *state) {
    IMAN_UNUSED(state);
    return IMAN_FALSE;
}

static int iman_field_meta_handler(struct iman_parser_state *state) {
    IMAN_UNUSED(state);
    return IMAN_FALSE;
}