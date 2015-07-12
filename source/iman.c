/*
 * iman - instruction set manual utility
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#include <stdio.h>
#include "iman.h"

int main(int argc, char **argv) 
{
    struct iman_options options = { 0 };
    
    if (iman_parse_arguments(argc, argv, &options) == 0) {
        iman_print_usage(argv[0]);
        
        return -1;
    }
    
    switch(options.mode) {
        case IMAN_OUTPUT_MODE_DOC:
            break;
        
        case IMAN_OUTPUT_MODE_TO_ENGLISH:
            break;
            
        default:
            break;
    }
    
    return 0;
}