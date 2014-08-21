#include "version.hpp"

//Build version tracking. Makefile should define the 
//BUILD_DATE and BUILD_VERSION macros.
#ifdef BUILD_DATE
const char* build_date = BUILD_DATE;
#else
const char* build_date = __DATE__ __TIME__;
#endif

#ifdef BUILD_VERSION
const char* build_version = BUILD_VERSION;
#else
const char* build_version = "unkown-build-version";
#endif
