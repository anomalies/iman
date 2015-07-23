/*
 * iman - instruction set manual utility
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#include "iman.h"
#include "iman_options.h"
#include "parser/iman_lexer.h"
#include "parser/iman_reference.h"
#include "parser/iman_parser.h"

int main(int argc, char **argv) 
{
    struct iman_options options = { 0 };
    
    if (iman_parse_arguments(argc, argv, &options) != IMAN_TRUE) {
        iman_print_usage(argv[0]);
        
        return -1;
    }
    
    switch(options.mode) {
        case IMAN_OUTPUT_MODE_DOC:
            break;
        
        case IMAN_OUTPUT_MODE_TO_ENGLISH:
            puts("Error: this feature hasn't been implemented.");
            break;
            
        default:
            break;
    }
    
    return 0;
}