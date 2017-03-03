#ifndef VTR_MATH_H
#define VTR_MATH_H

#include <map>
#include <cmath>

namespace vtr {
    /*********************** Math operations *************************************/
    int ipow(int base, int exp);

    template<typename X, typename Y> 
    Y linear_interpolate_or_extrapolate(std::map<X,Y> *xy_map, X requested_x);

    constexpr int nint(float val) { return static_cast<int>(val + 0.5); }

    template<typename InputIterator>
    double geomean(InputIterator first, InputIterator last, double init=1.) {
        //Compute the geometric mean of the elments in range [first, last)
        //
        //To avoid potential round-off issues we transform the standard formula:
        //
        //      geomean = ( v_1 * v_2 * ... * v_n) ^ (1/n)
        //
        //by taking the log:
        //
        //      geomean = exp( (1 / n) * (log(v_1) + log(v_2) + ... + log(v_n)))

        double log_sum = std::log(init);
        size_t n = 0;
        for(auto iter = first; iter != last; ++iter) {
            log_sum += std::log(*iter);
            n += 1;
        }

        return std::exp( (1. / n) * log_sum );
    }

    //Return the lowest common multiple of m and n
    // Note that T should be an integral type
    template<typename T>
    T lcm(T m, T n) {
        static_assert(std::is_integral<T>::value, "T must be integral");

        T lcm_val;
        for (lcm_val = 1; lcm_val % m != 0 || lcm_val % n != 0; ++lcm_val)
            ;

        return lcm_val;
    }
}

#endif
