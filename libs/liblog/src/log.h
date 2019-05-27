/**
 * Lightweight logging tool.  Automatically prepend messages with prefixes and store in log file.
 *
 * Init/Change name of log file using log_set_output_file, when done, call log_close
 *
 * Author: Jason Luu
 * Date: Sept 5, 2014
 */

#ifndef LOG_H
#define LOG_H

void log_set_output_file(const char* filename);

void log_print_direct(const char* message, ...);
void log_print_info(const char* message, ...);
void log_print_warning(const char* filename, unsigned int line_num, const char* message, ...);
void log_print_error(const char* filename, unsigned int line_num, const char* message, ...);

void log_close();

#endif
