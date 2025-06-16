#pragma once

#include <ostream>

namespace vtr {

///@brief A RAII guard class to ensure restoration of output stream format
class OsFormatGuard {
  public:
    ///@brief constructor
    explicit OsFormatGuard(std::ostream& os)
        : os_(os)
        , flags_(os_.flags()) //Save formatting flag state
        , width_(os_.width())
        , precision_(os.precision())
        , fill_(os.fill()) {}

    ///@brief destructor
    ~OsFormatGuard() {
        os_.flags(flags_); //Restore
        os_.width(width_);
        os_.precision(precision_);
        os_.fill(fill_);
    }

    OsFormatGuard(const OsFormatGuard&) = delete;
    OsFormatGuard& operator=(const OsFormatGuard&) = delete;
    OsFormatGuard(const OsFormatGuard&&) = delete;
    OsFormatGuard& operator=(const OsFormatGuard&&) = delete;

  private:
    std::ostream& os_;
    std::ios::fmtflags flags_;
    std::streamsize width_;
    std::streamsize precision_;
    char fill_;
};

} // namespace vtr
