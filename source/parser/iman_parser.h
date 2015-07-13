/*
 * iman - instruction set manual utility
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#ifndef _IMAN_PARSER_H
#define _IMAN_PARSER_H

struct iman_parser_options {
    unsigned int verbose;
    FILE * source;
    FILE * output;
};

struct iman_reference_source_definition_name {
    struct iman_reference_source_definition_name * next_name;
    
    char ** names;
    unsigned int name_count;
    
    char * title;
};

struct iman_reference_source_definition {
    struct iman_reference_source_definition * next_definition;
    
    struct iman_reference_source_definition_name * names;
};

struct iman_reference_source {
    struct iman_reference_source_definition *head;
    struct iman_reference_source_definition **tail;
    unsigned int count;
};

int iman_parse_reference_source(
    const struct iman_parser_options *options,
    struct iman_reference_source *refsource
);

int iman_reference_source_free(
    struct iman_reference_source *refsource
);

#endif