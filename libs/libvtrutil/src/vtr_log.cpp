#include <string>
#include <fstream>
#include <cstdarg>

#include "vtr_util.h"
#include "vtr_log.h"
#include "log.h"

namespace vtr {
PrintHandlerInfo printf = log_print_info;
PrintHandlerInfo printf_info = log_print_info;
PrintHandlerWarning printf_warning = log_print_warning;
PrintHandlerError printf_error = log_print_error;
PrintHandlerDirect printf_direct = log_print_direct;

void set_log_file(const char* filename) {
    log_set_output_file(filename);
}

} // namespace vtr

void add_warnings_to_suppress(std::string function_name) {
    warnings_to_suppress.insert(function_name);
}

void set_noisy_warn_log_file(std::string log_file_name) {
    std::ofstream log;
    log.open(log_file_name, std::ifstream::out | std::ifstream::trunc);
    log.close();
    noisy_warn_log_file = log_file_name;
}

void print_or_suppress_warning(const char* pszFileName, unsigned int lineNum, const char* pszFuncName, const char* pszMessage, ...) {
    std::string function_name(pszFuncName);

    va_list va_args;
    va_start(va_args, pszMessage);
    std::string msg = vtr::vstring_fmt(pszMessage, va_args);
    va_end(va_args);

    auto result = warnings_to_suppress.find(function_name);
    if (result == warnings_to_suppress.end()) {
        vtr::printf_warning(pszFileName, lineNum, msg.data());
    } else if (!noisy_warn_log_file.empty()) {
        std::ofstream log;
        log.open(noisy_warn_log_file.data(), std::ios_base::app);
        log << "Warning:\n\tfile: " << pszFileName << "\n\tline: " << lineNum << "\n\tmessage: " << msg << std::endl;
        log.close();
    }
}
