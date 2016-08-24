#include "vtr_log.h"

#include "log.h"

namespace vtr {
PrintHandlerInfo printf_info = log_print_info;
PrintHandlerWarning printf_warning = log_print_warning;
PrintHandlerError printf_error = log_print_error;
PrintHandlerDirect printf_direct = log_print_direct;
} //namespace
