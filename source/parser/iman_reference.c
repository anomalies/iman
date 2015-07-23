/*
 * iman - instruction set manual utility
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#include "../iman.h"
#include "iman_reference.h"

void iman_reference_block_release(struct iman_reference_block *block) {
    struct iman_reference_term_definition *term = block->terms;
    struct iman_reference_form_definition *form = block->forms;
    
    if (block->desc.buffer != NULL) {
        free(block->desc.buffer);
        block->desc.buffer = NULL;
    }
    
    while(term != NULL) {
        struct iman_reference_term_definition *next = term->next;
        unsigned int x;
        
        for (x = 0; x < term->name_count; ++x) {
            free(term->names[x]);
        }
        
        free(term);
        term = next;
    }
    
    while(form != NULL) {
        struct iman_reference_form_definition *next = form->next_form;
        
        free(form);
        form = next;
    }
    
    block->terms = NULL;
    block->forms = NULL;
}