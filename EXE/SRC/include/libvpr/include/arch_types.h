/*
Data types describing the FPGA architecture.

Date: February 19, 2009
Authors: Jason Luu and Kenneth Kent
*/

#ifndef ARCH_TYPES_H
#define ARCH_TYPES_H

#include "logic_types.h"
#include "physical_types.h"

/* Constant describing architecture library version number */
#define VPR_VERSION "6.0 BETA"

/* Input file parsing. */
#define TOKENS " \t\n"

/* UNDEFINED user inputs */
#define UNDEFINED -1

/* Want to avoid overflows of shorts.  OPINs can have edges to 4 * width if  *
 * they are on all 4 sides, so set MAX_CHANNEL_WIDTH to 8000.                */
#define MAX_CHANNEL_WIDTH 8000

#endif

