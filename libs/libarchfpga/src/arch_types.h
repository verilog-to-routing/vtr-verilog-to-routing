#pragma once
/*
 * Data types describing the FPGA architecture.
 *
 * Date: February 19, 2009
 * Authors: Jason Luu and Kenneth Kent
 */

/* Input file parsing. */
#define TOKENS " \t\n"

/* Value for UNDEFINED data */
constexpr int ARCH_FPGA_UNDEFINED_VAL = -1;

/* Maximum value for minimum channel width to avoid overflows of short data type.               */
constexpr int MAX_CHANNEL_WIDTH = 8000;

enum class e_arch_format {
    VTR,            ///<VTR-specific device XML format
    FPGAInterchange ///<FPGA Interchange device format
};
