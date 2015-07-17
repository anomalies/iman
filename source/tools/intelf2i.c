/*
 * iman - instruction set manual utility
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 * 
 * intelf2i: Utility to convert the Intel instruction set manual form table text into the iman format
 */

#include <stdio.h>
#include "../iman.h"

#define IMAN_INTELF2I_COLUMN_COUNT 6
#define IMAN_INTELF2I_LINE_SIZE 2048
#define IMAN_INTELF2I_MAX_OPERANDS 8

struct intelf2i_form {
    
    struct {
        unsigned int longlong:1, protected:1, real:1;
    } modes;
    
    unsigned int operand_count;
    char *operand[IMAN_INTELF2I_MAX_OPERANDS];
};

typedef void (*intelf2i_column_handler_func)(char *column, struct intelf2i_form *form);

static void intelf2i_parse_line(char * line);
static int intelf2i_consume_tab(char **line, char **column);

static int intelf2i_split(char **value, char delimiter, char **start);
static int intelf2i_split_trim(char **value, char delimiter, char **start);
static void intelf2i_to_lower(char *value);
static void intelf2i_trim_tail(char *value);

static void intelf2i_column_opcode(char *column, struct intelf2i_form *form);
static void intelf2i_column_form(char *column, struct intelf2i_form *form);
static void intelf2i_column_encoding(char *column, struct intelf2i_form *form);
static void intelf2i_column_64bit_mode(char *column, struct intelf2i_form *form);
static void intelf2i_column_longmode(char *column, struct intelf2i_form *form);
static void intelf2i_column_description(char *column, struct intelf2i_form *form);

static void intelf2i_emit_description_word(char *word_start, unsigned int length, struct intelf2i_form *form);

/* Reorders the output rows in line with the format */
static const unsigned int column_handler_remap_index[IMAN_INTELF2I_COLUMN_COUNT] = {
    1, /* Form, operands */
    3, /* Mode: 64bit */
    4, /* Mode: 32bit and 16bit */
    0, /* Opcode */
    5, /* Description */
    2, /* Encoding (unused) */
};

static const intelf2i_column_handler_func column_handler_table[IMAN_INTELF2I_COLUMN_COUNT] = {
    &intelf2i_column_opcode,
    &intelf2i_column_form,
    &intelf2i_column_encoding,
    &intelf2i_column_64bit_mode,
    &intelf2i_column_longmode,
    &intelf2i_column_description
};

static char line_buffer[IMAN_INTELF2I_LINE_SIZE];

int main(void) {
    
    while(fgets(line_buffer, sizeof(line_buffer), stdin) != NULL) {
        intelf2i_parse_line(line_buffer);
    }
    
    return 0;
}

static void intelf2i_parse_line(char * line) {
    unsigned int position;
    char *columns[IMAN_INTELF2I_COLUMN_COUNT];
    struct intelf2i_form form;
    
    for(position = 0; position < IMAN_INTELF2I_COLUMN_COUNT; ++position) {
        if (intelf2i_consume_tab(&line, &columns[position]) != IMAN_TRUE) {
            fprintf(stderr, "Error: encountered a line with an invalid number of tabs: %d\n", position + 1);
            return;
        }
        
        intelf2i_trim_tail(columns[position]);
    }
    
    memset(&form, 0, sizeof(form));
    
    putchar('[');
    
    /* Valid line, emit the converted version */
    for(position = 0; position < IMAN_INTELF2I_COLUMN_COUNT; ++position) {
        unsigned int translated = column_handler_remap_index[position];
        char *column = columns[translated];
        
        column_handler_table[translated](column, &form);
    }
    
    printf(" ]\n");
}

static int intelf2i_consume_tab(char **line, char **column) {
    char *scan = *line;
    *column = scan;
    
    if (*scan == '\0' || *scan == '\n')
        return IMAN_FALSE;
    
    for(; *scan != '\t'; ++scan) {
        if (*scan == '\0' || *scan == '\n') {
            *scan = '\0';
            *line = scan;
            
            return IMAN_TRUE;
        }
    }
    
    *scan = '\0';
    *line = scan + 1;
    
    return IMAN_TRUE;
}

static int intelf2i_split(char **value, char delimiter, char **start) {
    char *scan = *value;
    
    *start = scan;
    
    if (*scan == '\0')
        return IMAN_FALSE;
    
    for(; *scan != '\0'; ++scan) {
        if (*scan == delimiter) {
            *scan = '\0';
            *value = scan + 1;
            
            return IMAN_TRUE;
        }
    }
    
    *value = scan;
    return IMAN_TRUE;
}

static int intelf2i_split_trim(char **value, char delimiter, char **start) {
    char *scan = *value;
    
    if (*scan == '\0')
        return IMAN_FALSE;
    
    for(; *scan == ' '; ++scan)
        ;
    
    *start = scan;
    
    for(; *scan != '\0'; ++scan) {
        if (*scan == delimiter) {
            *value = scan + 1;
            
            for(scan = scan - 1; scan >= *start; --scan) {
                if (*scan != ' ') {
                    *(scan + 1) = '\0';
                    
                    break;
                }
            }
            
            return IMAN_TRUE;
        }
    }
    
    *value = scan;
    
    for(; scan >= *start; --scan) {
        if (*scan != ' ') {
            *(scan + 1) = '\0';

            return IMAN_TRUE;
        }
    }
    
    return IMAN_FALSE;
}

static void intelf2i_to_lower(char *value) {
    
    for(; *value != '\0'; ++value) {
        if (*value >= 'A' && *value <= 'Z') {
            *value |= 0x20;
        }
    }
}

static void intelf2i_trim_tail(char *value) {
    unsigned int x, length = strlen(value);
    
    for(x = length - 1; x != 0; --x) {
        if (value[x] != ' ') {
            value[x + 1] = '\0';
            return;
        }
    }
    
    /* Empty line */
    value[0] = '\0';
}

static void intelf2i_column_opcode(char *column, struct intelf2i_form *form) {
    IMAN_UNUSED(form);
    
    printf(" (%s),", column);
}

static void intelf2i_column_form(char *column, struct intelf2i_form *form) {
    char *instr_name = NULL;
    char *operand = NULL;
    int operand_size = -1;
    
    intelf2i_split(&column, ' ', &instr_name);
    
    intelf2i_to_lower(instr_name);
    
    printf(" (%s), ( ", instr_name);
    
    do {
        if (intelf2i_split_trim(&column, ',', &operand) == IMAN_TRUE) {
            int local_op_size = -1;
            
            form->operand[form->operand_count++] = operand;
            
            if (strncmp(operand, "imm8", 4) == 0) {
                operand = "i8";
                local_op_size = 8;
            } else if (strncmp(operand, "imm16", 5) == 0) {
                operand = "i16";
                local_op_size = 16;
            } else if (strncmp(operand, "imm32", 5) == 0){
                operand = "i32";
                local_op_size = 32;
            } else if (strncmp(operand, "imm64", 5) == 0) {
                operand = "i64";
                local_op_size = 64;
            } else if (strncmp(operand, "r/m8", 4) == 0) {
                operand = "v8";
                local_op_size = 8;
            } else if (strncmp(operand, "r/m16", 5) == 0) {
                operand = "v16";
                local_op_size = 16;
            } else if (strncmp(operand, "r/m32", 5) == 0) {
                operand = "v32";
                local_op_size = 32;
            } else if (strncmp(operand, "r/m64", 5) == 0) {
                operand = "v64";
                local_op_size = 64;
            } else if (strncmp(operand, "r8", 2) == 0) {
                operand = "r8";
                local_op_size = 8;
            } else if (strncmp(operand, "r16", 3) == 0) {
                operand = "r16";
                local_op_size = 16;
            } else if (strncmp(operand, "r32", 3) == 0) {
                operand = "r32";
                local_op_size = 32;
            } else if (strncmp(operand, "r64", 3) == 0) {
                operand = "r64";
                local_op_size = 64;
            }
            
            printf("%s", operand);
            
            if (operand_size == -1) {
                operand_size = local_op_size;
            }
        }
        
    } while(*column != '\0' && printf(", ") > 0);
    
    /* The last parentheses are for clobbers */
    if (operand_size != -1)
        printf(" ), (%d), (),", operand_size);
    else
        printf(" ), (), (),");
}

static void intelf2i_column_encoding(char *column, struct intelf2i_form *form) {
    /* This is currently useless due to needing a secondary table */
    IMAN_UNUSED(column);
    IMAN_UNUSED(form);
}

static void intelf2i_column_64bit_mode(char *column, struct intelf2i_form *form) {
    if (strcmp(column, "Valid") == 0) {
        printf(" ( 64;");
        form->modes.longlong = 1;
    } else {
        printf(" ( ");
    }
}

static void intelf2i_column_longmode(char *column, struct intelf2i_form *form) {
    if (strcmp(column, "Valid") == 0) {
        printf(" 32; 16 ), (),");
        form->modes.protected = 1;
        form->modes.real = 1;
    } else {
        printf(" ), (),");
    }
}

static void intelf2i_column_description(char *column, struct intelf2i_form *form) {
    unsigned int length = strlen(column);
        
    IMAN_UNUSED(form);
    /* Remove the full stop */
    column[length - 1] = '\0';
    
    printf(" (");
    
    while(*column != '\0') {
        char *word_start;
        unsigned int word_len;
        
        for(; *column == ' '; ++column)
            ;
        
        word_start = column;
        
        for (; *column != '\0'; ++column) {
            if (*column == ' ') {
                *column = '\0';
                ++column;
                break;
            }
        }
        
        word_len = (column - word_start) - 1;
        
        intelf2i_emit_description_word(word_start, word_len, form);
        
        if (*column != '\0')
            putchar(' ');
    }
    
    printf(")");
}

static void intelf2i_emit_description_word(char *word_start, unsigned int length, struct intelf2i_form *form) {
    unsigned int offset;
    
    for(offset = 0; offset < form->operand_count; ++offset) {
        if (strncmp(form->operand[offset], word_start, length) == 0) {
            printf("@%u", offset);
            return;
        }
    }
    
    printf("%s", word_start);
}