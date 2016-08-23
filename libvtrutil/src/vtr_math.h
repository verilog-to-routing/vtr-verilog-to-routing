#ifndef VTR_MATH_H
#define VTR_MATH_H

namespace vtr {
    /*********************** Math operations *************************************/
    int ipow(int base, int exp);

    template<typename X, typename Y> 
    Y linear_interpolate_or_extrapolate(std::map<X,Y> *xy_map, X requested_x);
}

#endif
