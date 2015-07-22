/*
 * iman - instruction set manual utility
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#ifndef _IMAN_FORM_PARSER_H
#define _IMAN_FORM_PARSER_H

int iman_parse_form(char *line, unsigned int base_column, struct iman_reference_form_definition *form);

#endif