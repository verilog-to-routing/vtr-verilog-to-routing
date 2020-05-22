#ifndef ODIN_ERROR_H
#define ODIN_ERROR_H

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>

enum odin_error {
    NO_ERROR,
    /* for error in utility functions*/
    UTIL,
    /* for error during initialization */
    PARSE_ARGS,
    /* for parser errors */
    PARSER,
    /* for AST related errors */
    AST,
    /* for Netlist related errors */
    NETLIST,
    /* for blif parser errors */
    PARSE_BLIF,
    /* for errors in the netlist simulation */
    SIMULATION,
};

extern const char* odin_error_STR[];
extern std::vector<std::pair<std::string, int>> include_file_names;
extern int delayed_errors;
// causes an interrupt in GDB
void _verbose_assert(bool condition, const char* condition_str, const char* odin_file_name, long odin_line_number, const char* odin_function_name);

#define oassert(condition) _verbose_assert(condition, #condition, __FILE__, __LINE__, __func__)

void _log_message(odin_error error_type, long column, long line_number, long file, bool soft_error, const char* function_file_name, long function_line, const char* function_name, const char* message, ...);

#define error_message(error_type, line_number, file, message, ...) \
    _log_message(error_type, -1, line_number, file, true, __FILE__, __LINE__, __PRETTY_FUNCTION__, message, __VA_ARGS__)

#define warning_message(error_type, line_number, file, message, ...) \
    _log_message(error_type, -1, line_number, file, false, __FILE__, __LINE__, __PRETTY_FUNCTION__, message, __VA_ARGS__)

#define possible_error_message(error_type, line_number, file, message, ...) \
    _log_message(error_type, -1, line_number, file, !global_args.permissive.value(), __FILE__, __LINE__, __PRETTY_FUNCTION__, message, __VA_ARGS__)

#define delayed_error_message(error_type, column, line_number, file, message, ...)                                                 \
    {                                                                                                                              \
        _log_message(error_type, column, line_number, file, false, __FILE__, __LINE__, __PRETTY_FUNCTION__, message, __VA_ARGS__); \
        delayed_errors += 1;                                                                                                       \
    }

void verify_delayed_error(odin_error error_type);

#endif