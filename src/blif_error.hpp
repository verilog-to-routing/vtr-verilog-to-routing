#ifndef BLIF_ERROR_H
#define BLIF_ERROR_H
#include <functional>
#include "blifparse.hpp"
namespace blifparse {


    extern std::function<void(const int, const std::string&, const std::string&)> blif_error;

    void blif_error_wrap(Callback& callback, const int line_no, const std::string& near_text, const char* fmt, ...);

}
#endif
