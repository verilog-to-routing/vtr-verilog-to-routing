#ifndef TATUM_MATH_HPP
#define TATUM_MATH_HPP
#include <cmath>

namespace tatum { namespace util {

inline float absolute_error(float a, float b) {
    return std::fabs(a - b);
}

inline float relative_error(float a, float b) {
    if (a == b) {
        return 0.;
    }

    if (std::fabs(b) > std::fabs(a)) {
        return std::fabs((a - b) / b);
    } else {
        return std::fabs((a - b) / a);
    }
}

inline bool nearly_equal(const float lhs, const float rhs, const float abs_err_tol, const float rel_err_tol) {
    float abs_err = absolute_error(lhs, rhs);
    float rel_err = relative_error(lhs, rhs);

    return (abs_err <= abs_err_tol) || (rel_err <= rel_err_tol);
}

}} //namspace

#endif
