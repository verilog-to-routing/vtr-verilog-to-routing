#ifndef VTR_ERROR_H
#define VTR_ERROR_H

#include <stdexcept>
#include <string>
#include <utility>

namespace vtr {

/**
 * @brief Contriner that holds informations related to an error
 *
 * It holds different info related to a VTR error:
 *      - error message
 *      - file name associated with the error
 *      - line number associated with the error
 */
class VtrError : public std::runtime_error {
  public:
    ///@brief VtrError constructor
    VtrError(const std::string& msg = "", std::string new_filename = "", size_t new_linenumber = -1)
        : std::runtime_error(msg)
        , filename_(std::move(new_filename))
        , linenumber_(new_linenumber) {}

    /**
     * @brief gets the filename 
     *
     * Returns the filename associated with this error.
     * Returns an empty string if none is specified.
     */
    std::string filename() const { return filename_; }

    ///@brief same as filename() but returns in c style string
    const char* filename_c_str() const { return filename_.c_str(); }

    /**
     * @brief get the line number
     *
     * Returns the line number associated with this error.
     * Returns zero if none is specified.
     */
    size_t line() const { return linenumber_; }

  private:
    std::string filename_;
    size_t linenumber_;
};

} // namespace vtr

#endif
