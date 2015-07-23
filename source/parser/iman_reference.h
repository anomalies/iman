/*
 * iman - instruction set manual utility
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#ifndef _IMAN_REFERENCE_H
#define _IMAN_REFERENCE_H

#define IMAN_REFERENCE_TERM_MAX_NAMES 8
#define IMAN_REFERENCE_TITLE_MAX_SIZE 256
#define IMAN_REFERENCE_MNEMONIC_SIZE 32
#define IMAN_REFERENCE_MAX_OPERANDS 4
#define IMAN_REFERENCE_MAX_CLOBBERS 4
#define IMAN_REFERENCE_OPERAND_LENGTH 8
#define IMAN_REFERENCE_MAX_FEATURES 4
#define IMAN_REFERENCE_FEATURE_LENGTH 8
#define IMAN_REFERENCE_MAX_OPCODE_SIZE 32
#define IMAN_REFERENCE_MAX_DESCRIPTION 256

struct iman_reference_term_definition {
    struct iman_reference_term_definition *next;
    
    unsigned int name_count;
    char *names[IMAN_REFERENCE_TERM_MAX_NAMES];
    
    char title[IMAN_REFERENCE_TITLE_MAX_SIZE];
};

struct iman_reference_form_definition {
    struct iman_reference_form_definition *next_form;
    
    char mnemonic[IMAN_REFERENCE_MNEMONIC_SIZE];
    
    /* Instruction width, -1 for variable/unknown; used for GAS-style suffix */
    int width;
    
    struct {
        unsigned int mode64:1, mode32:1, mode16:1;
        
        unsigned int count;
        
        /* e.g. AVX, AES etc */
        char name[IMAN_REFERENCE_MAX_FEATURES][IMAN_REFERENCE_FEATURE_LENGTH];
    } feature;
    
    struct {
        unsigned int count;
        char type[IMAN_REFERENCE_MAX_OPERANDS][IMAN_REFERENCE_OPERAND_LENGTH];
    } operand;
    
    struct {
        unsigned int count;
        char type[IMAN_REFERENCE_MAX_CLOBBERS][IMAN_REFERENCE_OPERAND_LENGTH];
    } clobber;
    
    char opcode[IMAN_REFERENCE_MAX_OPCODE_SIZE];
    
    char description[IMAN_REFERENCE_MAX_DESCRIPTION];
};

struct iman_reference_block {
    struct iman_reference_block * next_block;
    struct iman_reference_term_definition *terms;
    
    struct {
        char *buffer;
        unsigned int size;
        unsigned int offset;
    } desc;
    
    struct iman_reference_form_definition *forms;
};

void iman_reference_block_release(struct iman_reference_block *block);

#endif