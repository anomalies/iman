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

typedef void (*intelf2i_column_handler_func)(char *column);

static void intelf2i_parse_line(char * line);
static int intelf2i_consume_tab(char **line, char **column);

static int intelf2i_split(char **value, char delimiter, char **start);
static int intelf2i_split_trim(char **value, char delimiter, char **start);

static void intelf2i_trim_tail(char *value);

static void intelf2i_column_opcode(char *column);
static void intelf2i_column_form(char *column);
static void intelf2i_column_encoding(char *column);
static void intelf2i_column_64bit_mode(char *column);
static void intelf2i_column_longmode(char *column);
static void intelf2i_column_description(char *column);

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
    
    for(position = 0; position < IMAN_INTELF2I_COLUMN_COUNT; ++position) {
        if (intelf2i_consume_tab(&line, &columns[position]) != IMAN_TRUE) {
            fprintf(stderr, "Error: encountered a line with an invalid number of tabs: %d\n", position + 1);
            return;
        }
        
        intelf2i_trim_tail(columns[position]);
    }
    
    putchar('[');
    
    /* Valid line, emit the converted version */
    for(position = 0; position < IMAN_INTELF2I_COLUMN_COUNT; ++position) {
        unsigned int translated = column_handler_remap_index[position];
        char *column = columns[translated];
        
        column_handler_table[translated](column);
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

static void intelf2i_column_opcode(char *column) {
    printf(" (%s),", column);
}

static void intelf2i_column_form(char *column) {
    char *instr_name = NULL;
    char *operand = NULL;
    
    intelf2i_split(&column, ' ', &instr_name);
    
    printf(" (%s), ( ", instr_name);
    
    do {
        if (intelf2i_split_trim(&column, ',', &operand) == IMAN_TRUE)
            printf("%s", operand);
        
    } while(*column != '\0' && printf(", ") > 0);
    
    printf(" ),");
}

static void intelf2i_column_encoding(char *column) {
    /* This is currently useless due to needing a secondary table */
    IMAN_UNUSED(column);
}

static void intelf2i_column_64bit_mode(char *column) {
    if (strcmp(column, "Valid") == 0) {
        printf(" ( 64;");
    } else {
        printf(" ( ");
    }
}

static void intelf2i_column_longmode(char *column) {
    if (strcmp(column, "Valid") == 0) {
        printf(" 32; 16 ),");
    } else {
        printf(" ),");
    }
}

static void intelf2i_column_description(char *column) {
    printf(" (%s)", column);
}