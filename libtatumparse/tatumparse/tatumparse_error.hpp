#ifndef TATUM_ERROR_H
#define TATUM_ERROR_H
#include <functional>
#include "tatumparse.hpp"

namespace tatumparse {

    void tatum_error_wrap(Callback& callback, const int line_no, const std::string& near_text, const char* fmt, ...);

}

#endif
