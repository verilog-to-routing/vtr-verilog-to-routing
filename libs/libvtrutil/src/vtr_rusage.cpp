#include "vtr_rusage.h"

#ifdef __unix__
#    include <sys/time.h>
#    include <sys/resource.h>
#endif

namespace vtr {

///@brief Returns the maximum resident set size in bytes, or zero if unable to determine.
size_t get_max_rss() {
    size_t max_rss = 0;

#ifdef __unix__
    rusage usage;
    int result = getrusage(RUSAGE_SELF, &usage);

    if (result == 0) { //Success
        //ru_maxrss is in kilobytes, convert to bytes
        max_rss = usage.ru_maxrss * 1024;
    }
#else
    //Do nothing, other platform specific code could be added here
    //with appropriate defines
#endif

    return max_rss;
}

} // namespace vtr
