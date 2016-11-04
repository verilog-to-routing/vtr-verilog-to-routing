#include <cstdarg>
#include <cassert>
#include "blif_error.hpp"
#include "blifparse.hpp"

namespace blifparse {

std::string escape_string(const std::string& near_text);

//We wrap the actual blif_error to issolate custom handlers from vaargs
void blif_error_wrap(Callback& callback, const int line_no, const std::string& near_text, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    //We need to copy the args so we don't change them before the true formating
    va_list args_copy;
    va_copy(args_copy, args);

    //Determine the formatted length using a copy of the args
    int len = std::vsnprintf(nullptr, 0, fmt, args_copy); 

    va_end(args_copy); //Clean-up

    //Negative if there is a problem with the format string
    assert(len >= 0 && "Problem decoding format string");

    size_t buf_size = len + 1; //For terminator

    //Allocate a buffer
    //  unique_ptr will free buffer automatically
    std::unique_ptr<char[]> buf(new char[buf_size]);

    //Format into the buffer using the original args
    len = std::vsnprintf(buf.get(), buf_size, fmt, args);

    va_end(args); //Clean-up

    assert(len >= 0 && "Problem decoding format string");
    assert(static_cast<size_t>(len) == buf_size - 1);

    //Build the string from the buffer
    std::string msg(buf.get(), len);

    //TODO: escape near_text
    std::string escaped_near_text = escape_string(near_text);

    //Call the error handler
    callback.parse_error(line_no, escaped_near_text, msg);
}

std::string escape_string(const std::string& near_text) {
    std::string escaped_text;

    for(char c : near_text) {

        if(c == '\n') {
            escaped_text += "\\n";
        } else if(c == '\r') {
            escaped_text += "\\r";
        } else {
            escaped_text += c;
        }
    }

    return escaped_text;
}


}
