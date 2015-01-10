/**
 * Lightweight logging tool.  Automatically prepend messages with prefixes and store in log file.
 *
 * Author: Jason Luu
 * Date: Sept 5, 2014
 */


#include <stdio.h>
#include <stdarg.h> /* Allows for variable arguments, necessary for wrapping printf */
#include "log.h"

#define LOG_DEFAULT_FILE_NAME "output.log"

static int log_warning = 0;
static int log_error = 0;
FILE *log_stream = NULL;


static void check_init();

/* Set the output file of logger.
   If different than current log file, close current log file and reopen to new log file
*/
void log_set_output_file(const char *filename) {
	if(log_stream != NULL) {
		fclose(log_stream);
	}
	log_stream = fopen(filename, "w");
	if(log_stream == NULL) {
		printf("Error writing to file %s\n\n", filename);
	}
	fprintf(log_stream, "filename\n");
}

void log_print_direct(const char* message, ...) {
	va_list args;
	va_start(args, message);
	vprintf(message, args);
	va_end(args);
}

void log_print_info(const char* message, ...) {
	check_init(); /* Check if output log file setup, if not, then this function also sets it up */

	va_list args;
	va_start(args, message);
	vprintf(message, args);
	va_end(args);

	va_start(args, message); /* Must reset variable arguments so that they can be read again */
	vfprintf(log_stream, message, args);
	va_end(args);

	fflush(log_stream);
}

void log_print_warning(const char* filename, unsigned int line_num, const char* message, ...) {
	check_init(); /* Check if output log file setup, if not, then this function also sets it up */

	va_list args;
	va_start(args, message);
	log_warning++;

	printf("Warning %d: ", log_warning);
	vprintf(message, args);
	va_end(args);

	va_start(args, message); /* Must reset variable arguments so that they can be read again */
	fprintf(log_stream, "Warning %d: ", log_warning);
	vfprintf(log_stream, message, args);

	va_end(args);
	fflush(log_stream);
}

void log_print_error(const char* filename, unsigned int line_num, const char* message, ...) {
	check_init(); /* Check if output log file setup, if not, then this function also sets it up */

	va_list args;
	va_start(args, message);
	log_error++;

	check_init();
	fprintf(stderr, "Error %d: ", log_error);
	vfprintf(stderr, message, args);
	va_end(args);

	va_start(args, message); /* Must reset variable arguments so that they can be read again */
	fprintf(log_stream, "Error %d: ", log_error);
	vfprintf(log_stream, message, args);

	va_end(args);

	fflush(log_stream);
}

/**
 * Check if output log file setup, if not, then this function also sets it up 
 */
static void check_init() {
	if(log_stream == NULL) {
		log_stream = fopen(LOG_DEFAULT_FILE_NAME, "w");
		if(log_stream == NULL) {
			printf("Error writing to file %s\n", LOG_DEFAULT_FILE_NAME);
		}
		fprintf(log_stream, "%s\n\n", LOG_DEFAULT_FILE_NAME);
	}
}


void log_close() {
	fclose(log_stream);
}
