/*
 * iman - instruction set manual utility
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#ifndef _IMAN_REF_WRITER_H
#define _IMAN_REF_WRITER_H

struct iman_ref_writer {
    FILE *table_output;
    FILE *index_output;
};

int iman_ref_writer_open(struct iman_ref_writer *writer, const char *target_dir, const char *arch_name);

int iman_ref_writer_close(struct iman_ref_writer *writer);

int iman_ref_writer_add(struct iman_ref_writer *writer, struct iman_reference_block *block);

#endif