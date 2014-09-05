/**
 * Lightweight logging tool.  Automatically prepend messages with prefixes and store in log file.
 *
 * Author: Jason Luu
 * Date: Sept 5, 2014
 */


#include <stdio.h>
#include <stdarg.h> /* Allows for variable arguments, necessary for wrapping printf */
#include "log.h"

static int log_warning = 0;
static int log_error = 0;

void log_set_output_file(char *filename) {
}

void log_print_info(const char* message, ...) {
	va_list args;
	va_start(args, message);
	vprintf(message, args);
	va_end(args);
}

void log_print_warning(const char* message, ...) {
	va_list args;
	va_start(args, message);
	log_warning++;
	printf("Warning %d: ", log_warning);
	vprintf(message, args);
	va_end(args);
}

void log_print_error(const char* message, ...) {
	va_list args;
	va_start(args, message);
	log_error++;
	printf("Error %d: ", log_error);
	vprintf(message, args);
	va_end(args);
}



