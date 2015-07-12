/*
 * iman - instruction set manual utility
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#include <stdio.h>
#include "iman.h"

struct iman_option_definition {
    const char *command;
    const char *command_alt;
    const char *help_text;
    
    int (*handler)(int right_args, char ***pargv, struct iman_options *options);
} iman_option_definition;

static int iman_process_argument(int right_args, char ***pargv, struct iman_options *options);
static int iman_process_input(int right_args, char **argv, struct iman_options *options);

static int iman_option_arch_handler(int right_args, char ***pargv, struct iman_options *options);
static int iman_option_english_handler(int right_args, char ***pargv, struct iman_options *options);

static const char * iman_default_architecture_name = "intel";

static const struct iman_option_definition iman_command_line_options[] = {
    { "-arch", "-a", "-arch, -a <architecture>: Sets the target architecture", &iman_option_arch_handler },
    { "-english", "-e", "-english, -e: Describes the instruction in plain English", &iman_option_english_handler },

    { NULL, NULL, NULL, NULL }
};

void iman_set_default_options(struct iman_options *options)
{
    options->architecture = iman_default_architecture_name;
    options->mode = IMAN_OUTPUT_MODE_DOC;
}

void iman_print_usage(const char *binary_path) 
{
    const struct iman_option_definition *definition = iman_command_line_options;
    
    printf("Usage: %s <options> <instruction name>\n", binary_path);
    puts("Options:");
    
    for(; definition->command != NULL; ++definition) {
        printf("\t%s\n", definition->help_text);
    }
}

int iman_parse_arguments(int argc, char **argv, struct iman_options *options)
{
    char **arguments_end = &argv[argc];
    
    if (argc <= 1) {
        return 0;
    }
    
    /* Skip the binary path argument */
    argv = &argv[1];
    
    while(argv < arguments_end) {
        char *argument = *argv;
        
        /* Process as a command line option */
        if (*argument == '-') {
            if (iman_process_argument((int)(arguments_end - argv) - 1, &argv, options) == 0) {
                return 0;
            }
            
        } else {
            return iman_process_input((int)(arguments_end - argv), argv, options);
        }
    }
    
    return 0;
}

static int iman_process_argument(int right_args, char ***pargv, struct iman_options *options) 
{
    const struct iman_option_definition *definition = iman_command_line_options;
    char * argument = **pargv;
    
    for(; definition->command != NULL; ++definition) {
        if (strcmp(argument, definition->command) == 0 || strcmp(argument, definition->command_alt) == 0) {
            return definition->handler(right_args, pargv, options);
        }
    }
    
    printf("Unrecognised option %s\n", argument);
    return 0;
}

static int iman_process_input(int right_args, char **argv, struct iman_options *options)
{
    options->input_body.count = right_args;
    options->input_body.args = argv;
    
    return right_args;
}

static int iman_option_arch_handler(int right_args, char ***pargv, struct iman_options *options) 
{
    char **argv = *pargv;
    
    if (right_args < 1) {
        printf("%s expects an architecture name.\n", *argv);
        
        return 0;
    }
    
    options->architecture = argv[1];
    
    *pargv = &argv[2];
    return 1;
}

static int iman_option_english_handler(int right_args, char ***pargv, struct iman_options *options)
{
    IMAN_UNUSED(right_args);

    options->mode = IMAN_OUTPUT_MODE_TO_ENGLISH;
    
    *pargv = *pargv + 1;
    return 1;
}