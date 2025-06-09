#pragma once

#include <map>
#include <vector>
#include <cmath>

/**
 * @file
 *
 * @brief This file defines some math operations
 */

namespace vtr {
/*********************** Math operations *************************************/

///@brief Calculates the value pow(base, exp)
int ipow(int base, int exp);

///@brief Returns the median of an input vector.
float median(std::vector<float> vector);

///@brief Linear interpolation/Extrapolation
template<typename X, typename Y>
Y linear_interpolate_or_extrapolate(const std::map<X, Y>* xy_map, X requested_x);

///@brief Integer rounding conversion for floats
constexpr int nint(float val) { return static_cast<int>(val + 0.5); }

///@brief Returns a 'safe' ratio which evaluates to zero if the denominator is zero
template<typename T>
T safe_ratio(T numerator, T denominator) {
    if (denominator == T(0)) {
        return 0;
    }
    return numerator / denominator;
}

///@brief Returns the median of the elements in range [first, last]
template<typename InputIterator>
double median(InputIterator first, InputIterator last) {
    auto len = std::distance(first, last);
    auto iter = first + len / 2;

    if (len % 2 == 0) {
        return (*iter + *(iter + 1)) / 2;
    } else {
        return *iter;
    }
}

///@brief Returns the median of a whole container
template<typename Container>
double median(Container c) {
    return median(std::begin(c), std::end(c));
}

/**
 * @brief Returns the geometric mean of the elments in range [first, last)
 *
 * To avoid potential round-off issues we transform the standard formula:
 *
 *      geomean = ( v_1 * v_2 * ... * v_n) ^ (1/n)
 *
 * by taking the log:
 *
 *      geomean = exp( (1 / n) * (log(v_1) + log(v_2) + ... + log(v_n)))
 */
template<typename InputIterator>
double geomean(InputIterator first, InputIterator last, double init = 1.) {
    double log_sum = std::log(init);
    size_t n = 0;
    for (auto iter = first; iter != last; ++iter) {
        log_sum += std::log(*iter);
        n += 1;
    }

    if (n == 0) {
        return init;
    } else {
        return std::exp((1. / n) * log_sum);
    }
}

///@brief Returns the geometric mean of a whole container
template<typename Container>
double geomean(Container c) {
    return geomean(std::begin(c), std::end(c));
}

///@brief Returns the arithmatic mean of the elements in range [first, last]
template<typename InputIterator>
double arithmean(InputIterator first, InputIterator last, double init = 0.) {
    double sum = init;
    size_t n = 0;
    for (auto iter = first; iter != last; ++iter) {
        sum += *iter;
        n += 1;
    }

    if (n == 0) {
        return init;
    } else {
        return sum / n;
    }
}

///@brief Returns the aritmatic mean of a whole container
template<typename Container>
double arithmean(Container c) {
    return arithmean(std::begin(c), std::end(c));
}

/**
 * @brief Returns the greatest common divisor of x and y
 *
 * Note that T should be an integral type
 */
template<typename T>
static T gcd(T x, T y) {
    static_assert(std::is_integral<T>::value, "T must be integral");
    // Euclidean algorithm
    if (y == 0) {
        return x;
    }
    return gcd(y, x % y);
}

/**
 * @brief Return the least common multiple of x and y
 *
 * Note that T should be an integral type
 */
template<typename T>
T lcm(T x, T y) {
    static_assert(std::is_integral<T>::value, "T must be integral");

    if (x == 0 && y == 0) {
        return 0;
    } else {
        return (x / gcd(x, y)) * y;
    }
}

constexpr double DEFAULT_REL_TOL = 1e-9;
constexpr double DEFAULT_ABS_TOL = 0;

///@brief Return true if a and b values are close to each other
template<class T>
bool isclose(T a, T b, T rel_tol, T abs_tol) {
    if (std::isinf(a) && std::isinf(b)) return (std::signbit(a) == std::signbit(b));
    if (std::isnan(a) && std::isnan(b)) return false;

    T abs_largest = std::max(std::abs(a), std::abs(b));
    return std::abs(a - b) <= std::max(rel_tol * abs_largest, abs_tol);
}

///@brief Return true if a and b values are close to each other (using the default tolerances)
template<class T>
bool isclose(T a, T b) {
    return isclose<T>(a, b, DEFAULT_REL_TOL, DEFAULT_ABS_TOL);
}

} // namespace vtr
