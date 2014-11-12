#include <iostream>
#include <string>

#include "Time.hpp"

std::ostream& operator<<(std::ostream& os, const Time& time) {
    os << time.value();
    return os;
}
