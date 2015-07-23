/*
 * iman - instruction set manual utility
 * Andrew Watts - 2015 <andrew@andrewwatts.info>
 */

#ifndef _IMAN_H
#define _IMAN_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define IMAN_UNUSED(arg) (void)arg

#define IMAN_FALSE 0
#define IMAN_TRUE 1

#define IMAN_FOURCC(a, b, c, d) (((d) << 24 & 0xFF000000) | ((c) << 16 & 0xFF0000) | ((b) << 8 & 0xFF00) | ((a) & 0xFF))

#include "iman_config.h"

#endif