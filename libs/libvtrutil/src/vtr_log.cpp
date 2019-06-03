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
