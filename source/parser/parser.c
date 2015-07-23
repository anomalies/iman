/*
 * iman - instruction set manual utility
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#include "../iman.h"
#include "iman_lexer.h"
#include "iman_reference.h"
#include "iman_ref_writer.h"
#include "iman_parser.h"

#define IMAN_INSN_FILENAME "instruction.iman"
#define MAX_PATH_LENGTH 1024

static int iman_build_table(char *source_name, char *output_dir, char *arch);

int main(int argc, char **argv) {
    char path_buffer[MAX_PATH_LENGTH];
    
    if (argc != 4) {
        printf("Usage: %s sourcedir arch targetdir\nGenerates the index and compressed reference table.\n",
            argc > 0 ? argv[0] : "iman-parser"
        );
        
        return -1;
    }
    
    if (snprintf(path_buffer, MAX_PATH_LENGTH, "%s/%s/" IMAN_INSN_FILENAME, argv[1], argv[2]) >= MAX_PATH_LENGTH) {
        printf("Error: specified path %s was too long\n", argv[1]);
        return -1;
    }
    
    return iman_build_table(path_buffer, argv[3], argv[2]);
}

static int iman_build_table(char *source_name, char *output_dir, char *arch) {
    struct iman_parser parser;
    struct iman_ref_writer writer;
    int result = 0;
    
    if (iman_parser_initialise(&parser, source_name) != IMAN_TRUE) {
        printf("Error: unable to open source file %s\n", source_name);
        return -1;
    }
    
    if (iman_ref_writer_open(&writer, output_dir, arch) != IMAN_TRUE) {
        iman_parser_release(&parser);
        return -2;
    }
    
    printf("Info: starting to parse %s\n", source_name);
    
    
    while(iman_parser_is_eof(&parser) == IMAN_FALSE) {
        if (iman_parser_read_block(&parser) != IMAN_TRUE) {
            /*long offset, end = ftell(parser.lexer.source);*/
            iman_reference_block_release(&parser.block);
            result = -3;
            break;
        }
        
        printf("Info: parsed block %s\n", parser.block.terms->names[0]);
        
        if (iman_ref_writer_add(&writer, &parser.block) != IMAN_TRUE) {
            iman_reference_block_release(&parser.block);
            result = -4;
            break;
        }
        
        printf("Info: wrote block %s\n", parser.block.terms->names[0]);
        
        fflush(writer.index_output);
        fflush(writer.table_output);
        
        iman_reference_block_release(&parser.block);
    }
    
    iman_parser_release(&parser);
    iman_ref_writer_close(&writer);
    return result;
}