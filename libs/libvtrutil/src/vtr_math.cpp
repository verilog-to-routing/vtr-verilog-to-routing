#include <map>

#include "vtr_assert.h"
#include "vtr_error.h"
#include "vtr_math.h"

namespace vtr {

int ipow(int base, int exp) {
    int result = 1;

    VTR_ASSERT(exp >= 0);

    while (exp) {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        base *= base;
    }
    return result;
}

/* Performs linear interpolation or extrapolation on the set of (x,y) values specified by the xy_map.
 * A requested x value is passed in, and we return the interpolated/extrapolated y value at this requested value of x.
 * Meant for maps where both key and element are numbers.
 * This is specifically enforced by the explicit instantiations below this function. i.e. only templates
 * using those types listed in the explicit instantiations below are allowed */
template<typename X, typename Y>
Y linear_interpolate_or_extrapolate(const std::map<X, Y>* xy_map, X requested_x) {
    Y result;

    /* the intention of this function is to interpolate/extrapolate. we can't do so with less than 2 values in the xy_map */
    if (xy_map->size() < 2) {
        throw VtrError("linear_interpolate_or_extrapolate: cannot interpolate/extrapolate based on less than 2 (x,y) pairs", __FILE__, __LINE__);
    }

    auto itr = xy_map->find(requested_x);
    if (itr != xy_map->end()) {
        /* requested x already exists in the x,y map */
        result = itr->second;
    } else {
        /* requested x does not exist in the x,y map. need to interpolate/extrapolate */

        typename std::map<X, Y>::const_iterator it;
        double x_low, x_high, y_low, y_high;
        double slope, reference_y, delta_x;

        /* get first x greater than the one requested */
        it = xy_map->upper_bound(requested_x);

        if (it == xy_map->end()) {
            /* need to extrapolate to higher x. based on the y values at the two largest x values */
            it--;
            x_high = (double)it->first;
            y_high = (double)it->second;
            it--;
            x_low = (double)it->first;
            y_low = (double)it->second;
        } else if (it == xy_map->begin()) {
            /* need to extrapolate to lower x. based on the y values at the two smallest x */
            x_low = (double)it->first;
            y_low = (double)it->second;
            it++;
            x_high = (double)it->first;
            y_high = (double)it->second;
        } else {
            /* need to interpolate. based on y values at x just above/below
             * the one we want */
            x_high = (double)it->first;
            y_high = (double)it->second;
            it--;
            x_low = (double)it->first;
            y_low = (double)it->second;
        }

        slope = (y_high - y_low) / (x_high - x_low);
        reference_y = y_low;
        delta_x = (double)requested_x - x_low;
        result = (Y)(reference_y + (slope * delta_x));
    }

    return result;
}
template double linear_interpolate_or_extrapolate(const std::map<int, double>* xy_map, int requested_x);       /* (int,double) */
template double linear_interpolate_or_extrapolate(const std::map<double, double>* xy_map, double requested_x); /* (double,double) */

} // namespace vtr
