#include <cstdarg>

#include "vtr_util.h"
#include "vpr_error.h"

/* Date:June 15th, 2013
 * Author: Daniel Chen
 * Purpose: Used to throw any internal VPR error or architecture
 *			file error and output the appropriate file name,
 *			line number, and the error message. Does not return
 *			anything but throw an exception which will be caught
 *			main.c.
 */
void vpr_throw(enum e_vpr_error type,
               const char* psz_file_name,
               unsigned int line_num,
               const char* psz_message,
               ...) {
    // Make a variable argument list
    va_list va_args;

    // Initialize variable argument list
    va_start(va_args, psz_message);

    //Format the message
    std::string msg = vtr::vstring_fmt(psz_message, va_args);

    // Reset variable argument list
    va_end(va_args);

    throw VprError(type, msg, psz_file_name, line_num);
}

void vvpr_throw(enum e_vpr_error type,
                const char* psz_file_name,
                unsigned int line_num,
                const char* psz_message,
                va_list va_args) {
    //Format the message
    std::string msg = vtr::vstring_fmt(psz_message, va_args);

    throw VprError(type, msg, psz_file_name, line_num);
}
