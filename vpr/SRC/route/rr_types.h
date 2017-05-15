#ifndef RR_TYPES_H
#define RR_TYPES_H
#include <vector>
#include "vtr_ndmatrix.h"

typedef std::vector<vtr::NdMatrix<int,5>> t_pin_to_track_lookup;
typedef std::vector<vtr::NdMatrix<vtr::t_ivec,4>> t_track_to_pin_lookup;

#endif
