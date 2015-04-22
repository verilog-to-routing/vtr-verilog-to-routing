#include <iostream>
#include <string>

#include "Time.hpp"

inline Time operator+(Time lhs, const Time& rhs) {
    return lhs += rhs;
}

inline Time operator-(Time lhs, const Time& rhs) {
    return lhs -= rhs;
}

inline std::ostream& operator<<(std::ostream& os, const Time& time) {
    os << time.value();
    return os;
}

