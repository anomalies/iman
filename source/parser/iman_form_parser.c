/*
 * iman - instruction set manual utility
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#include "../iman.h"
#include "iman_reference.h"
#include "iman_form_parser.h"

#define IMAN_FORM_MAX_COLUMNS 8

struct iman_form_lexer {
    char *line;
    unsigned int column;
    unsigned int base_col;
};

static int iman_form_parse_column(struct iman_form_lexer *lexer, struct iman_reference_form_definition *form, unsigned int column);
static void iman_form_lexer_skip_whitespace(struct iman_form_lexer *lexer);
static int iman_form_lexer_accept_syntax(struct iman_form_lexer *lexer, char syntax);
static int iman_form_lexer_accept_text(struct iman_form_lexer *lexer, char **ptext, unsigned int *plength);

static int iman_form_parse_insn_name(struct iman_form_lexer *lexer, struct iman_reference_form_definition *form);
static int iman_form_parse_operands(struct iman_form_lexer *lexer, struct iman_reference_form_definition *form);
static int iman_form_parse_width(struct iman_form_lexer *lexer, struct iman_reference_form_definition *form);
static int iman_form_parse_clobbers(struct iman_form_lexer *lexer, struct iman_reference_form_definition *form);
static int iman_form_parse_architectures(struct iman_form_lexer *lexer, struct iman_reference_form_definition *form);
static int iman_form_parse_features(struct iman_form_lexer *lexer, struct iman_reference_form_definition *form);
static int iman_form_parse_opcodes(struct iman_form_lexer *lexer, struct iman_reference_form_definition *form);
static int iman_form_parse_description(struct iman_form_lexer *lexer, struct iman_reference_form_definition *form);

static int (* const iman_form_column_parsers[IMAN_FORM_MAX_COLUMNS])(struct iman_form_lexer *lexer, struct iman_reference_form_definition *form) = {
    iman_form_parse_insn_name,
    iman_form_parse_operands,
    iman_form_parse_width,
    iman_form_parse_clobbers,
    iman_form_parse_architectures,
    iman_form_parse_features,
    iman_form_parse_opcodes,
    iman_form_parse_description
};

int iman_parse_form(char *line, unsigned int base_column, struct iman_reference_form_definition *form) {
    struct iman_form_lexer lexer;
    unsigned int column_number = 0;
    
    lexer.line = line;
    lexer.column = 0;
    lexer.base_col = base_column;
    
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

static int iman_form_parse_column(struct iman_form_lexer *lexer, struct iman_reference_form_definition *form, unsigned int column) {
    if (column >= IMAN_FORM_MAX_COLUMNS) {
        printf("Form error, c %d: too many columns in a form definition, the maximum is %d\n", 
               lexer->column + lexer->base_col,
               IMAN_FORM_MAX_COLUMNS
        );
        
        return IMAN_FALSE;
    }
    
    if (iman_form_lexer_accept_syntax(lexer, '(') != IMAN_TRUE) {
        printf("Form error, c %d: expected an opening parenthesis, did you add one too many commas?\n", lexer->column + lexer->base_col);
        
        return IMAN_FALSE;
    }
    
    if (iman_form_column_parsers[column](lexer, form) != IMAN_TRUE)
        return IMAN_FALSE;
    
    if (iman_form_lexer_accept_syntax(lexer, ')') != IMAN_TRUE) {
        printf("Form error, c %d: expected a closing parenthesis, did you add one too many commas?\n", lexer->column + lexer->base_col);
        
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

static int iman_form_lexer_accept_text(struct iman_form_lexer *lexer, char **ptext, unsigned int *plength) {
    unsigned int length;
    
    iman_form_lexer_skip_whitespace(lexer);
    
    for(length = 0; lexer->line[lexer->column + length] != '\0'; ++length) {
        char c = lexer->line[lexer->column + length];
        
        if (isalnum(c) == 0)
            break;
    }
    
    if (length == 0) {
        return IMAN_FALSE;
    }
    
    *plength = length;
    *ptext = &lexer->line[lexer->column];
    
    lexer->column += length;
    
    return IMAN_TRUE;
}

static int iman_form_parse_insn_name(struct iman_form_lexer *lexer, struct iman_reference_form_definition *form) {
    unsigned int length = 0;
    char *text = NULL;
    
    if (iman_form_lexer_accept_text(lexer, &text, &length) != IMAN_TRUE) {
        printf("Form error, c %d: expected an instruction mnemonic.\n", lexer->column + lexer->base_col);
        return IMAN_FALSE;
    }
    
    memcpy(form->mnemonic, text, length);
    form->mnemonic[length] = '\0';
    
    return IMAN_TRUE;
}

static int iman_form_parse_operands(struct iman_form_lexer *lexer, struct iman_reference_form_definition *form) {
    form->operand.count = 0;
    
    do {
        unsigned int type_length = 0;
        char *type_text = NULL;
        
        if (form->operand.count >= IMAN_REFERENCE_MAX_OPERANDS) {
            printf("Form error, c %d: too many operands, the maximum allowed is %d\n", 
                   lexer->column + lexer->base_col, 
                   IMAN_REFERENCE_MAX_OPERANDS
            );
            
            return IMAN_FALSE;
        }
        
        if (iman_form_lexer_accept_text(lexer, &type_text, &type_length) != IMAN_TRUE) {
            printf("Form error, c %d: expected an operand type name\n", lexer->column + lexer->base_col);
            return IMAN_FALSE;
        }
        
        if (type_length + 1 >= IMAN_REFERENCE_OPERAND_LENGTH) {
            printf("Form error, c %d: operand type exceeds the maximum allowable length, %d, with a length of: %u\n",
                   lexer->column + lexer->base_col, 
                   IMAN_REFERENCE_OPERAND_LENGTH - 1, 
                   type_length
            );
            
            return IMAN_FALSE;
        }
        
        memcpy(form->operand.type[form->operand.count], type_text, type_length);
        form->operand.type[form->operand.count][type_length] = '\0';
        form->operand.count++;
    } while (iman_form_lexer_accept_syntax(lexer, ',') == IMAN_TRUE);
    
    return IMAN_TRUE;
}

static int iman_form_parse_width(struct iman_form_lexer *lexer, struct iman_reference_form_definition *form) {
    unsigned int width_length = 0;
    char *width_text = NULL;
    
    /* An empty width is acceptable */
    if (iman_form_lexer_accept_text(lexer, &width_text, &width_length) != IMAN_TRUE) {
        form->width = -1;
        return IMAN_TRUE;
    }
    
    if (width_length != 0) {
        /* TODO: make this a sane implementation */
        
        if (strncmp(width_text, "64", width_length) == 0) {
            form->width = 64;
        } else if (strncmp(width_text, "32", width_length) == 0) {
            form->width = 32;
        } else if (strncmp(width_text, "16", width_length) == 0) {
            form->width = 16;
        } else if (strncmp(width_text, "8", width_length) == 0) {
            form->width = 8;
        } else {
            printf("Form error, c %d: invalid instruction width specified.\n", lexer->column + lexer->base_col);
            return IMAN_FALSE;
        }
    }
    
    return IMAN_TRUE;
}

static int iman_form_parse_clobbers(struct iman_form_lexer *lexer, struct iman_reference_form_definition *form) {
    form->clobber.count = 0;
    
    do {
        unsigned int type_length = 0;
        char *type_text = NULL;
        
        if (form->clobber.count >= IMAN_REFERENCE_MAX_CLOBBERS) {
            printf("Form error, c %d: too many clobbers, the maximum allowed is %d\n", 
                   lexer->column + lexer->base_col, 
                   IMAN_REFERENCE_MAX_CLOBBERS
            );
            
            return IMAN_FALSE;
        }
        
        if (iman_form_lexer_accept_text(lexer, &type_text, &type_length) != IMAN_TRUE) {
            printf("Form error, c %d: expected a clobber type name\n", lexer->column + lexer->base_col);
            return IMAN_FALSE;
        }
        
        if (type_length + 1 >= IMAN_REFERENCE_OPERAND_LENGTH) {
            printf("Form error, c %d: operand type exceeds the maximum allowable length, %d, with a length of: %u\n",
                   lexer->column + lexer->base_col, 
                   IMAN_REFERENCE_OPERAND_LENGTH - 1, 
                   type_length
            );
            
            return IMAN_FALSE;
        }
        
        memcpy(form->clobber.type[form->clobber.count], type_text, type_length);
        form->clobber.type[form->clobber.count][type_length] = '\0';
        form->clobber.count++;
    } while (iman_form_lexer_accept_syntax(lexer, ',') == IMAN_TRUE);
    
    return IMAN_TRUE;
}

static int iman_form_parse_architectures(struct iman_form_lexer *lexer, struct iman_reference_form_definition *form) {
    form->feature.mode16 = form->feature.mode32 = form->feature.mode64 = 0;
    
    do {
        unsigned int arch_length = 0;
        char *arch_name = NULL;
        
        if (iman_form_lexer_accept_text(lexer, &arch_name, &arch_length) != IMAN_TRUE)
            break;
        
        if (strncmp(arch_name, "64", arch_length) == 0) {
            form->feature.mode64 = 1;
        } else if (strncmp(arch_name, "32", arch_length) == 0) {
            form->feature.mode32 = 1;
        } else if (strncmp(arch_name, "16", arch_length) == 0) {
            form->feature.mode16 = 1;
        } else {
            printf("Form error, c %d: unrecognised architecture type name; expected 64, 32 or 16.\n", lexer->column + lexer->base_col);
            return IMAN_FALSE;
        }
    } while (iman_form_lexer_accept_syntax(lexer, ';') == IMAN_TRUE);
    
    return IMAN_TRUE;
}

static int iman_form_parse_features(struct iman_form_lexer *lexer, struct iman_reference_form_definition *form) {
    do {
        unsigned int feature_length = 0;
        char *feature_name = NULL;
        
        if (form->feature.count >= IMAN_REFERENCE_MAX_FEATURES) {
            printf("Form error, c %d: tried to add more than the allowed number of feature names %d.\n", 
                   lexer->column + lexer->base_col,
                   IMAN_REFERENCE_MAX_FEATURES
            );
            
            return IMAN_FALSE;
        }
        
        if (iman_form_lexer_accept_text(lexer, &feature_name, &feature_length) != IMAN_TRUE)
            break;
        
        if (feature_length >= IMAN_REFERENCE_FEATURE_LENGTH) {
            printf("Form error, c %d: feature name is longer than the maximum allowable %d.\n", 
                   lexer->column + lexer->base_col,
                   IMAN_REFERENCE_FEATURE_LENGTH - 1
            );
            
            return IMAN_FALSE;
        }
        
        memcpy(form->feature.name[form->feature.count], feature_name, feature_length);
        
        form->feature.name[form->feature.count][feature_length] = '\0';
        form->feature.count++;
        
    } while (iman_form_lexer_accept_syntax(lexer, ';') == IMAN_TRUE);
    
    return IMAN_TRUE;
}

static int iman_form_parse_opcodes(struct iman_form_lexer *lexer, struct iman_reference_form_definition *form) {
    unsigned int column;
    
    iman_form_lexer_skip_whitespace(lexer);
    
    for(column = 0; lexer->line[lexer->column + column] != ')'; ++column) {
        if (lexer->line[lexer->column + column] == '\0') {
            printf("Form error, c %d: expected either an opcode definition followed by a closing parenthesis.\n", lexer->column + lexer->base_col);
            return IMAN_FALSE;
        }
    }
    
    if (column <= 1) {
        printf("Form error, c %d: expected either an opcode definition followed by a closing parenthesis.\n", lexer->column + lexer->base_col);
        return IMAN_FALSE;
    }
    
    /* Back away from the closing parenthesis */
    --column;
    
    if (column > IMAN_REFERENCE_MAX_OPCODE_SIZE) {
        printf("Form error, c %d: this opcode definition exceeds the maximum allowable length of %d\n", 
               lexer->column + lexer->base_col,
               IMAN_REFERENCE_MAX_OPCODE_SIZE - 1
        );
        
        return IMAN_FALSE;
    }
    
    memcpy(form->opcode, &lexer->line[lexer->column], column);
    form->opcode[column] = '\0';
    lexer->column += column;
    return IMAN_TRUE;
}

static int iman_form_parse_description(struct iman_form_lexer *lexer, struct iman_reference_form_definition *form) {
    unsigned int desc_length = 0;
    char *desc_text = NULL;
    
    if (iman_form_lexer_accept_text(lexer, &desc_text, &desc_length) != IMAN_TRUE) {
        printf("Form error, c %d: expected a description of this instruction form.\n", lexer->column + lexer->base_col);
        return IMAN_FALSE;
    }
    
    if (desc_length >= IMAN_REFERENCE_MAX_DESCRIPTION) {
        printf("Form error, c %d: description exceeds the maximum allowable length, %d, with a length of: %u\n",
               lexer->column + lexer->base_col, 
               IMAN_REFERENCE_MAX_DESCRIPTION - 1, 
               desc_length
        );
        
        return IMAN_FALSE;
    }
    
    memcpy(form->description, desc_text, desc_length);
    form->description[desc_length] = '\0';
    
    return IMAN_TRUE;
}