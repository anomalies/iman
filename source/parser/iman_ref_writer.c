/*
 * iman - instruction set manual utility
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#include "../iman.h"
#include "iman_reference.h"
#include "iman_binary_writer.h"
#include "iman_ref_writer.h"
#include <zlib.h>

#define IMAN_MAX_PATH 1024

static int write_index_entry(struct iman_ref_writer *writer, const char *name, long offset);
static int write_table_entry(struct iman_ref_writer *writer, struct iman_reference_block *block, long block_offset);

int iman_ref_writer_open(struct iman_ref_writer *writer, const char *target_dir, const char *arch_name) {
    char path_buffer[IMAN_MAX_PATH];
    
    memset(writer, 0, sizeof (*writer));
    
    if (snprintf(path_buffer, IMAN_MAX_PATH, "%s/%s" IMAN_REF_INDEX_EXT, target_dir, arch_name) >= IMAN_MAX_PATH) {
        puts("Error: output index path is too long");
        return IMAN_FALSE;
    }
    
    writer->index_output = fopen(path_buffer, "wb");
    
    if (writer->index_output == NULL) {
        printf("Error: unable to open index output file %s\n", path_buffer);
        return IMAN_FALSE;
    }
    
    printf("Info: index file is %s\n", path_buffer);
    
    if (snprintf(path_buffer, IMAN_MAX_PATH, "%s/%s" IMAN_REF_TABLE_EXT, target_dir, arch_name) >= IMAN_MAX_PATH) {
        puts("Error: output table path is too long");
        return IMAN_FALSE;
    }
    
    writer->table_output = fopen(path_buffer, "wb");
    
    if (writer->table_output == NULL) {
        printf("Error: unable to open table output file %s\n", path_buffer);
        fclose(writer->index_output);
        return IMAN_FALSE;
    }
    
    printf("Info: table file is %s\n", path_buffer);
    
    return IMAN_TRUE;
}

int iman_ref_writer_close(struct iman_ref_writer *writer) {
    fclose(writer->index_output);
    fclose(writer->table_output);
    
    return IMAN_TRUE;
}

int iman_ref_writer_add(struct iman_ref_writer *writer, struct iman_reference_block *block) {
    struct iman_reference_term_definition *term;
    long block_offset;
    
    block_offset = ftell(writer->table_output);
    
    for(term = block->terms; term != NULL; term = term->next) {
        unsigned int x;
        
        for (x = 0; x < term->name_count; ++x) {
            if (write_index_entry(writer, term->names[x], block_offset) != IMAN_TRUE) {
                printf("Error: unable to write an index entry for %s\n", term->names[x]);
                return IMAN_FALSE;
            }
        }
    }
    
    return write_table_entry(writer, block, block_offset);
}

static int write_index_entry(struct iman_ref_writer *writer, const char *name, long offset) {
    struct iman_binary_writer binary_writer;
    char buffer[64];
    
    iman_binary_writer_initialise(&binary_writer, buffer, sizeof(buffer));
    
    iman_binary_writer_put_fixed_string(&binary_writer, name, IMAN_INDEX_NAME_WIDTH);
    iman_binary_writer_put_uint32(&binary_writer, (uint32_t)offset);

    return fwrite(binary_writer.buffer, binary_writer.position, 1, writer->index_output) == 1 ? IMAN_TRUE : IMAN_FALSE;
}

static int write_table_entry(struct iman_ref_writer *writer, struct iman_reference_block *block, long block_offset) {
    struct iman_binary_writer binary_writer;
    char buffer[32];
    
    IMAN_UNUSED(block_offset);
    
    iman_binary_writer_initialise(&binary_writer, buffer, sizeof(buffer));
    iman_binary_writer_put_uint32(&binary_writer, IMAN_FIELD_ID_DESCRIPTION);
    iman_binary_writer_put_uint32(&binary_writer, block->desc.offset + 1);
    
    if (fwrite(binary_writer.buffer, binary_writer.position, 1, writer->table_output) != 1) {
        return IMAN_FALSE;
    }
    
    if (fwrite(block->desc.buffer, block->desc.offset + 1, 1, writer->table_output) != 1) {
        return IMAN_FALSE;
    }
    
    return IMAN_TRUE;
}