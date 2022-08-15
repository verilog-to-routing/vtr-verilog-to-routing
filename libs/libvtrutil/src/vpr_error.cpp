#include <cstdarg>
#include <string>

#include "vtr_util.h"
#include "vtr_log.h"
#include "vpr_error.h"

// Set of function names for which the VPR_THROW errors are treated
// as VTR_LOG_WARN
static std::unordered_set<std::string> functions_to_demote;

/* Date:June 15th, 2013
 * Author: Daniel Chen
 * Purpose: Used to throw any internal VPR error or architecture
 *			file error and output the appropriate file name,
 *			line number, and the error message. Does not return
 *			anything but throw an exception which will be caught
 *			main.c.
 */
void map_error_activation_status(std::string function_name) {
    functions_to_demote.insert(function_name);
}

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

    vpr_throw_msg(type, psz_file_name, line_num, msg);
}

void vvpr_throw(enum e_vpr_error type,
                const char* psz_file_name,
                unsigned int line_num,
                const char* psz_message,
                va_list va_args) {
    //Format the message
    std::string msg = vtr::vstring_fmt(psz_message, va_args);

    vpr_throw_msg(type, psz_file_name, line_num, msg);
}

void vpr_throw_msg(enum e_vpr_error type,
                   const char* psz_file_name,
                   unsigned int line_num,
                   std::string msg) {
    throw VprError(type, msg, psz_file_name, line_num);
}

void vpr_throw_opt(enum e_vpr_error type,
                   const char* psz_func_pretty_name,
                   const char* psz_func_name,
                   const char* psz_file_name,
                   unsigned int line_num,
                   const char* psz_message,
                   ...) {
    std::string func_name(psz_func_name);

    // Make a variable argument list
    va_list va_args;

    // Initialize variable argument list
    va_start(va_args, psz_message);

    //Format the message
    std::string msg = vtr::vstring_fmt(psz_message, va_args);

    // Reset variable argument list
    va_end(va_args);

    auto result = functions_to_demote.find(func_name);
    if (result != functions_to_demote.end()) {
        VTR_LOGFF_WARN(psz_file_name, line_num, psz_func_pretty_name, msg.data());
    } else {
        vpr_throw_msg(type, psz_file_name, line_num, msg);
    }
}
