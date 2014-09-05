/**
 * Lightweight logging tool.  Automatically prepend messages with prefixes and store in log file.
 *
 * Author: Jason Luu
 * Date: Sept 5, 2014
 */

#ifndef LOG_H
#define LOG_H

void log_set_output_file(char *filename);

void log_print_info(const char* message, ...);
void log_print_warning(const char* message, ...);
void log_print_error(const char* message, ...);

#endif
