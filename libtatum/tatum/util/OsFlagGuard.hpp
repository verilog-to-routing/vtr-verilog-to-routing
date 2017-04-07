#ifndef TATUM_OS_FLAG_GUARD_HPP
#define TATUM_OS_FLAG_GUARD_HPP

#include <ostream>

namespace tatum {

//A RAII guard class to ensure restoration of output stream state
class OsFlagGuard {
    public:
        explicit OsFlagGuard(std::ostream& os)
            : os_(os)
            , flags_(os_.flags()) //Save formatting flag state
        {}

        ~OsFlagGuard() {
            os_.flags(flags_); //Restore
        }

        OsFlagGuard(const OsFlagGuard&) = delete;
        OsFlagGuard& operator=(const OsFlagGuard&) = delete;
        OsFlagGuard(const OsFlagGuard&&) = delete;
        OsFlagGuard& operator=(const OsFlagGuard&&) = delete;
    private:
        std::ostream& os_;
        std::ios::fmtflags flags_;
};


} //namespace

#endif
