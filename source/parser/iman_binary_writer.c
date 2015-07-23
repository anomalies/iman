/*
 * iman - instruction set manual utility
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#include "../iman.h"
#include "iman_binary_writer.h"

void iman_binary_writer_initialise(struct iman_binary_writer *writer, char *buffer, size_t maximum_size) {
    writer->buffer = buffer;
    writer->length = maximum_size;
    writer->position = 0;
    writer->error_state = 0;
}

void iman_binary_writer_put_fixed_string(struct iman_binary_writer *writer, const char *text, unsigned int width) {
    unsigned int offset;
    char *target;
    
    target = &writer->buffer[writer->position];
    
    for (offset = 0; offset < (width - 1); ++offset) {
        if (text[offset] == '\0')
            break;
        
        target[offset] = text[offset];
    }
    
    memset(&target[offset], 0, width - offset);
    writer->position += width;
}

void iman_binary_writer_put_uint32(struct iman_binary_writer *writer, uint32_t value) {
    char *buffer = &writer->buffer[writer->position];
    
    buffer[0] = value >>  0;
    buffer[1] = value >>  8;
    buffer[2] = value >> 16;
    buffer[3] = value >> 24;
    
    writer->position += sizeof(uint32_t);
}