#ifndef VTR_ERROR_H
#define VTR_ERROR_H

#include <stdexcept>
#include <string>

namespace vtr {

class VtrError : public std::runtime_error {
  public:
    VtrError(std::string msg = "", std::string new_filename = "", size_t new_linenumber = -1)
        : std::runtime_error(msg)
        , filename_(new_filename)
        , linenumber_(new_linenumber) {}

    //Returns the filename associated with this error
    //returns an empty string if none is specified
    std::string filename() const { return filename_; }
    const char* filename_c_str() const { return filename_.c_str(); }

    //Returns the line number associated with this error
    //returns zero if none is specified
    size_t line() const { return linenumber_; }

  private:
    std::string filename_;
    size_t linenumber_;
};

} // namespace vtr

#endif
