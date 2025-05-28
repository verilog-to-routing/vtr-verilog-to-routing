#ifndef _resources_hpp_INCLUDED
#define _resources_hpp_INCLUDED

#include "global.h"

#include <cstdint>

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

double absolute_real_time ();
double absolute_process_time ();

uint64_t maximum_resident_set_size ();
uint64_t current_resident_set_size ();

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif // ifndef _resources_hpp_INCLUDED
