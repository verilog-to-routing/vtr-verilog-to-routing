#include "noc_link.h"

// constructor
NocLink::NocLink(NocLinkId link_id, NocRouterId source, NocRouterId sink,
                 double bw, double lat)
    : id(link_id)
    , source_router(source)
    , sink_router(sink)
    , bandwidth(bw)
    , latency(lat) { }

// getters
NocRouterId NocLink::get_source_router() const {
    return source_router;
}

NocRouterId NocLink::get_sink_router() const {
    return sink_router;
}

//setters
void NocLink::set_source_router(NocRouterId source) {
    source_router = source;
}

void NocLink::set_sink_router(NocRouterId sink) {
    sink_router = sink;
}

void NocLink::set_bandwidth(double new_bandwidth) {
    bandwidth = new_bandwidth;
}

double NocLink::get_bandwidth() const {
    return bandwidth;
}

double NocLink::get_latency() const {
    return latency;
}

NocLinkId NocLink::get_link_id() const {
    return id;
}

NocLink::operator NocLinkId() const {
    return get_link_id();
}

