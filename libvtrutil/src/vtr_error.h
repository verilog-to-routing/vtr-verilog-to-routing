#ifndef VTR_ERROR_H
#define VTR_ERROR_H

#include <stdexcept>

class VtrError : public std::runtime_error {
    public:
        VtrError(std::string msg="", std::string new_filename="", size_t new_linenumber=-1)
            : std::runtime_error(msg)
            , filename_(new_filename)
            , linenumber_(new_linenumber) {}

        //Returns the filename associated with this error
        //returns an empty string if none is specified
        std::string filename() { return filename_; }

        //Returns the line number associated with this error
        //returns zero if none is specified
        size_t line() { return linenumber_; }

    private:
        std::string filename_;
        size_t linenumber_;
};

#endif
