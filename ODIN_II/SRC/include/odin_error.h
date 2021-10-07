#ifndef ODIN_ERROR_H
#define ODIN_ERROR_H

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>

struct loc_t {
    int file = -1;
    int line = -1;
    int col = -1;
};

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
    /* for BLIF elaboration related error */
    BLIF_ELABORATION,
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
extern const loc_t unknown_location;

// causes an interrupt in GDB
[[noreturn]] void _verbose_abort(const char* condition_str, const char* odin_file_name, int odin_line_number, const char* odin_function_name);

#define oassert(condition) \
    if (!bool(condition)) _verbose_abort(#condition, __FILE__, __LINE__, __func__)

void _log_message(odin_error error_type, loc_t loc, bool soft_error, const char* function_file_name, int function_line, const char* function_name, const char* message, ...);

#define error_message(error_type, loc, message, ...) \
    _log_message(error_type, loc, true, __FILE__, __LINE__, __PRETTY_FUNCTION__, message, __VA_ARGS__)

#define warning_message(error_type, loc, message, ...) \
    _log_message(error_type, loc, false, __FILE__, __LINE__, __PRETTY_FUNCTION__, message, __VA_ARGS__)

#define possible_error_message(error_type, loc, message, ...) \
    _log_message(error_type, loc, !global_args.permissive.value(), __FILE__, __LINE__, __PRETTY_FUNCTION__, message, __VA_ARGS__)

#define delayed_error_message(error_type, loc, message, ...)                                                 \
    {                                                                                                        \
        _log_message(error_type, loc, false, __FILE__, __LINE__, __PRETTY_FUNCTION__, message, __VA_ARGS__); \
        delayed_errors += 1;                                                                                 \
    }

void verify_delayed_error(odin_error error_type);

#endif
