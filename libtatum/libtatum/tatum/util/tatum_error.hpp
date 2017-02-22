#ifndef TATUM_ERROR_HPP
#define TATUM_ERROR_HPP
#include <stdexcept>

namespace tatum {
class Error : public std::runtime_error {

    public:
        template<typename ...Args>
        explicit Error(Args&&... args)
            : std::runtime_error(std::forward<Args>(args)...) {}

};

}
#endif
