#ifndef VTR_ERROR_H
#define VTR_ERROR_H

#include <stdexcept>
#include <string>

/**
 * @file
 * @brief A utility container that can be used to identify VTR execution errors.
 * 
 * The recommended usage is to store information in this container about the error during an error event and and then throwing an exception with the container. If the exception is not handled (exception is not caught), this will result in the termination of the program.
 * 
 * Error information can be displayed using the information stored within this container.
 * 
 */

namespace vtr {

/**
 * @brief Container that holds information related to an error
 *
 * It holds different info related to a VTR error:
 *      - error message
 *      - file name associated with the error
 *      - line number associated with the error
 * 
 * Example Usage:
 * 
 *      // creating and throwing an exception with a VtrError container that has an error occuring in file "error_file.txt" at line number 1
 *       
 *      throw vtr::VtrError("This is a program terminating error!", "error_file.txt", 1);
 * 
 */
class VtrError : public std::runtime_error {
  public:
    ///@brief VtrError constructor
    VtrError(std::string msg = "", std::string new_filename = "", size_t new_linenumber = -1)
        : std::runtime_error(msg)
        , filename_(new_filename)
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
