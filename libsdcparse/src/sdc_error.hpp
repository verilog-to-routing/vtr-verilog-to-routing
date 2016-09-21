#ifndef SDC_ERROR_H
#define SDC_ERROR_H
#include <functional>
namespace sdcparse {


    extern std::function<void(const int, const std::string&, const std::string&)> sdc_error;

    void sdc_error_wrap(const int line_no, const std::string& near_text, const char* fmt, ...);

}
#endif
