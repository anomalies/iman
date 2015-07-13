/*
 * iman - instruction set manual utility
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#ifndef _IMAN_OPTIONS_H
#define _IMAN_OPTIONS_H

enum iman_output_mode {
    IMAN_OUTPUT_MODE_DOC = 0,
    IMAN_OUTPUT_MODE_TO_ENGLISH
};

struct iman_options {
    const char *architecture;
    
    enum iman_output_mode mode;
    
    struct {
        int count;
        char **args;
    } input_body;
};

void iman_set_default_options(
    struct iman_options *options
);

int iman_parse_arguments(
    int argc, 
    char **argv,
    struct iman_options *options
);

void iman_print_usage(
    const char *binary_path
);

#endif