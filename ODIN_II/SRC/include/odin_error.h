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
    /* for blif output errors */
    OUTPUT_BLIF,
    /* for errors in the netlist simulation */
    SIMULATION,
};

extern const char* odin_error_STR[];
extern std::vector<std::pair<std::string, int>> include_file_names;
extern int delayed_errors;
extern const loc_t unknown_location;

#ifdef ODIN_USE_YOSYS
#    define YOSYS_ELABORATION_ERROR             \
        "Yosys failed to perform elaboration, " \
        "Please look at the log file for the failure cause or pass \'--show_yosys_log\' to Odin-II to see the logs.\n"
#    define YOSYS_FORK_ERROR \
        "Yosys child process failed to be created\n"
#    define YOSYS_PLUGINS_WITH_ODIN_ERROR \
        "Utilizing SystemVerilog/UHDM plugins requires activating the Yosys frontend.\
        Please recompile the VTR project either with the \"WITH_YOSYS\" or \"ODIN_USE_YOSYS\" flag on."
#else
#    define YOSYS_INSTALLATION_ERROR                             \
        "It seems Yosys is not installed in the VTR repository." \
        " Please compile the VTR with (\" -DODIN_USE_YOSYS=ON \") flag.\n"
#endif

#ifndef YOSYS_SV_UHDM_PLUGIN
#    define YOSYS_PLUGINS_NOT_COMPILED \
        "SystemVerilog/UHDM plugins are not compiled.\
        Please recompile the VTR project with the \"YOSYS_SV_UHDM_PLUGIN\" flag on."
#endif

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
