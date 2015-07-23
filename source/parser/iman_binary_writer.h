/*
 * iman - instruction set manual utility
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#ifndef _IMAN_BINARY_WRITER_H
#define _IMAN_BINARY_WRITER_H

struct iman_binary_writer {
    char *buffer;
    
    size_t position;
    size_t length;
    
    int error_state;
};

void iman_binary_writer_initialise(struct iman_binary_writer *writer, char *buffer, size_t maximum_size);

void iman_binary_writer_put_fixed_string(struct iman_binary_writer *writer, const char *text, unsigned int width);

void iman_binary_writer_put_uint32(struct iman_binary_writer *writer, uint32_t value);

#endif