#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <memory>

#include "util.h"
#include "vtr_util.h"
#include "log.h"

/* This file contains utility functions widely used in *
 * my programs.  Many are simply versions of file and  *
 * memory grabbing routines that take the same         *
 * arguments as the standard library ones, but exit    *
 * the program if they find an error condition.        */

vpr_PrintHandlerInfo vpr_printf_info = log_print_info;
vpr_PrintHandlerWarning vpr_printf_warning = log_print_warning;
vpr_PrintHandlerError vpr_printf_error = log_print_error;
vpr_PrintHandlerDirect vpr_printf_direct = log_print_direct;


/* Returns the min of cur and max. If cur > max, a warning
 * is emitted. */
int limit_value(int cur, int max, const char *name) {
	if (cur > max) {
		vpr_printf_warning(__FILE__, __LINE__,
				"%s is being limited from [%d] to [%d]\n", name, cur, max);
		return max;
	}
	return cur;
}




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

