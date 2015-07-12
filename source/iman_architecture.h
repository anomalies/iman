/*
 * iman - instruction set manual utility
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#ifndef _IMAN_ARCHITECTURE_H
#define _IMAN_ARCHITECTURE_H

struct iman_architecture_handler {
    const char * name;
    
    int (*handler)(struct iman_options *options);
};

int iman_arch_intel_handler(struct iman_options *options);

int iman_arch_mips32_handler(struct iman_options *options);

#endif