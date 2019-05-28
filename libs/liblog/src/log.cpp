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
FILE* log_stream = nullptr;

static void check_init();

/* Set the output file of logger.
 * If different than current log file, close current log file and reopen to new log file
 */
void log_set_output_file(const char* filename) {
    if (log_stream != nullptr) {
        fclose(log_stream);
    }

    if (filename == nullptr) {
        log_stream = nullptr;
    } else {
        log_stream = fopen(filename, "w");
        if (log_stream == nullptr) {
            printf("Error writing to file %s\n\n", filename);
        }
    }
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

    if (log_stream) {
        va_start(args, message); /* Must reset variable arguments so that they can be read again */
        vfprintf(log_stream, message, args);
        va_end(args);

        fflush(log_stream);
    }
}

void log_print_warning(const char* /*filename*/, unsigned int /*line_num*/, const char* message, ...) {
    check_init(); /* Check if output log file setup, if not, then this function also sets it up */

    va_list args;
    va_start(args, message);
    log_warning++;

    printf("Warning %d: ", log_warning);
    vprintf(message, args);
    va_end(args);

    if (log_stream) {
        va_start(args, message); /* Must reset variable arguments so that they can be read again */
        fprintf(log_stream, "Warning %d: ", log_warning);
        vfprintf(log_stream, message, args);

        va_end(args);
        fflush(log_stream);
    }
}

void log_print_error(const char* /*filename*/, unsigned int /*line_num*/, const char* message, ...) {
    check_init(); /* Check if output log file setup, if not, then this function also sets it up */

    va_list args;
    va_start(args, message);
    log_error++;

    check_init();
    fprintf(stderr, "Error %d: ", log_error);
    vfprintf(stderr, message, args);
    va_end(args);

    if (log_stream) {
        va_start(args, message); /* Must reset variable arguments so that they can be read again */
        fprintf(log_stream, "Error %d: ", log_error);
        vfprintf(log_stream, message, args);

        va_end(args);

        fflush(log_stream);
    }
}

/**
 * Check if output log file setup, if not, then this function also sets it up
 */
static void check_init() {
    //We now allow a nullptr log_stream (i.e. no log file) so nothing to do here
}

void log_close() {
    if (log_stream) {
        fclose(log_stream);
    }
}
