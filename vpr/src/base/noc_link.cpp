#include "noc_link.h"

// constructor
NocLink::NocLink(NocRouterId source, NocRouterId sink)
    : source_router(source)
    , sink_router(sink) {
    // initialize variables
    bandwidth_usage = 0.0;
}

// getters
NocRouterId NocLink::get_source_router(void) const {
    return source_router;
}

NocRouterId NocLink::get_sink_router(void) const {
    return sink_router;
}

double NocLink::get_bandwidth_usage(void) const {
    return bandwidth_usage;
}

//setters
void NocLink::set_bandwidth_usage(double new_bandwidth_usage) {
    bandwidth_usage = new_bandwidth_usage;
}