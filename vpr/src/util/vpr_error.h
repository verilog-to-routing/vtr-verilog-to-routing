#ifndef VPR_ERROR_H
#define VPR_ERROR_H

/**
 * @file
 * @brief This header defines useful methods to identify VPR execution errors. The VTRUTIL API provides additional logging and error identification options and these can be found in [VTR Logging-Errors-Assertions](../vtrutil/logging.html). 
 *
 * # VPR Error Macros #
 * 
 * 
 * There are three different way to identify errors in VPR:
 *      -# VPR_FATAL_ERROR: This is used to signal an unconditional fatal error which should stop the program. The error will automatically specify the file and line number of the call site. 
 *         
 *         For Example:
 *              
 *                    // The first argument is an enum which describes the error type
 *                    // The second argument is the error message and allows format specifiers
 *                    VPR_FATAL_ERROR(enum e_vpr_error, "This is an error message %s", some_string);
 *
 *      -# VPR_ERROR: This is used to signal an error (potentially non-fatal) which by default stops the program, but may be suppressed (i.e. converted to a warning). The macro will automatically check whether the function where it was called is in a list of functions names provided with the '\--disable_errors' option and if it is then the error is converted to a 'VTR_LOG_WARN'. The error will automatically specify the file and line number of the call site.
 * 
 *          For Example:
 *              
 *                    // The first argument is an enum which describes the error type
 *                    // The second argument is the error message and allows format specifiers
 *                    VPR_ERROR(enum e_vpr_error, "This is an error message %s", some_string);
 * 
 *      -# vpr_throw: This is used to signal an unconditional fatal error which should stop the program but also allows for the specification of the filename and line number within the file causing the error. The recommened use is for cases where a file is being parsed and there was an error within the file. In this case, the filename and line number can be specified.
 * 
 *          For Example:
 *              
 *                    // The first argument is an enum which describes the error type
 *                    // The last argument is the error message and allows format specifiers 
 *                    vpr_throw(enum e_vpr_error, name_of_file_causing_error, line_number_causing_error, "This is an error message %s", some_string);
 * 
 */


#include <cstdarg>
#include <string>
#include <unordered_set>

#include "vtr_error.h"

/// This describes the error types seen in VPR
enum e_vpr_error {
    VPR_ERROR_UNKNOWN = 0,  /// Unknown Error

    // Flow errors
    VPR_ERROR_ARCH,         /// Error while parsing the architecture description file
    VPR_ERROR_PACK,         /// Error while packing the netlist
    VPR_ERROR_PLACE,        /// Error while placing the netlist
    VPR_ERROR_ROUTE,        /// Error while routing the netlist
    VPR_ERROR_TIMING,
    VPR_ERROR_POWER,
    VPR_ERROR_SDC,

    // File parsing errors
    VPR_ERROR_NET_F,        /// Error while parsing the packed netlist file
    VPR_ERROR_PLACE_F,      /// Error while parsing the placement file
    VPR_ERROR_BLIF_F,       /// Error while parsing the blif file
    VPR_ERROR_IC_NETLIST_F, /// Error while parsing the interchange netlist file

    VPR_ERROR_IMPL_NETLIST_WRITER,
    VPR_ERROR_NETLIST,
    VPR_ERROR_ATOM_NETLIST,
    VPR_ERROR_CLB_NETLIST,
    VPR_ERROR_ANALYSIS,
    VPR_ERROR_INTERRUPTED,
    VPR_ERROR_DRAW,         /// Error while drawing the FPGA device
};
/// \cond DO NOT DOCUMENT
typedef enum e_vpr_error t_vpr_error_type;

/* This structure is thrown back to highest level of VPR flow if an *
 * internal VPR or user input error occurs. */

class VprError : public vtr::VtrError {
  public:
    VprError(t_vpr_error_type err_type,
             std::string msg = "",
             std::string file = "",
             size_t linenum = -1)
        : VtrError(msg, file, linenum)
        , type_(err_type) {}

    t_vpr_error_type type() const { return type_; }

  private:
    t_vpr_error_type type_;
};

// This function is used to save into the functions_to_demote set
// all the function names which contain VPR_THROW errors that are
// going to be demoted to be VTR_LOG_WARN
void map_error_activation_status(std::string function_name);

//VPR error reporting routines
//
//Note that we mark these functions with the C++11 attribute 'noreturn'
//as they will throw exceptions and not return normally. This can help
//reduce false-positive compiler warnings
[[noreturn]] void vpr_throw(enum e_vpr_error type, const char* psz_file_name, unsigned int line_num, const char* psz_message, ...);
[[noreturn]] void vvpr_throw(enum e_vpr_error type, const char* psz_file_name, unsigned int line_num, const char* psz_message, va_list args);
[[noreturn]] void vpr_throw_msg(enum e_vpr_error type, const char* psz_file_name, unsigned int line_num, std::string msg);

void vpr_throw_opt(enum e_vpr_error type, const char* psz_func_pretty_name, const char* psz_func_name, const char* psz_file_name, unsigned int line_num, const char* psz_message, ...);

//Figure out what macro to use to get the name of the current function
// We default to __func__ which is defined in C99
//
// g++ > 2.6 define __PRETTY_FUNC__ which includes class/namespace/overload
// information, so we prefer to use it if possible
#define VPR_THROW_FUNCTION __func__
#ifdef __GNUC__
#    ifdef __GNUC_MINOR__
#        if __GNUC__ >= 2 && __GNUC_MINOR__ > 6
#            undef VPR_THROW_FUNCTION
#            define VPR_THROW_FUNCTION __PRETTY_FUNCTION__
#        endif
#    endif
#endif

/*
 * Unconditionally throws a VprError condition with automatically specified
 * file and line number of the call site.
 *
 * It is preferred to use either VPR_FATAL_ERROR(), or VPR_ERROR() to capture
 * the intention behind the throw.
 *
 * This macro is a wrapper around vpr_throw().
 */
#define VPR_THROW(type, ...)                              \
    do {                                                  \
        vpr_throw(type, __FILE__, __LINE__, __VA_ARGS__); \
    } while (false)

/*
 * VPR_FATAL_ERROR() is used to signal an *unconditional* fatal error which should
 * stop the program.
 *
 * This macro is a wrapper around VPR_THOW()
 */
#define VPR_FATAL_ERROR(...)    \
    do {                        \
        VPR_THROW(__VA_ARGS__); \
    } while (false)

/*
 * VPR_ERROR() is used to signal an error (potentially non-fatal) which by
 * default stops the program, but may be suppressed (i.e. converted to a
 * warning).
 *
 * This macro is a wrapper around vpr_throw_opt() which automatically
 * specifies file and line number of call site.
 *
 */
#define VPR_ERROR(type, ...)                                                                \
    do {                                                                                    \
        vpr_throw_opt(type, VPR_THROW_FUNCTION, __func__, __FILE__, __LINE__, __VA_ARGS__); \
    } while (false)

#endif
/// \endcond