#ifndef SDC_ERROR_H
#define SDC_ERROR_H
#include "sdcparse.hpp"

namespace sdcparse {
    void sdc_error_wrap(Callback& callback, const int line_no, const std::string& near_text, const char* fmt, ...);
}
#endif
