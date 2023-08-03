#include "noc_link.h"

// constructor
NocLink::NocLink(NocLinkId link_id, NocRouterId source, NocRouterId sink, double bw)
    : id(link_id)
    , source_router(source)
    , sink_router(sink)
    , bandwidth_usage(0.0)
    , bandwidth(bw) {}

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
void NocLink::set_source_router(NocRouterId source) {
    source_router = source;
    return;
}

void NocLink::set_sink_router(NocRouterId sink) {
    sink_router = sink;
    return;
}

void NocLink::set_bandwidth_usage(double new_bandwidth_usage) {
    bandwidth_usage = new_bandwidth_usage;
}

void NocLink::set_bandwidth(double new_bandwidth) {
    bandwidth = new_bandwidth;
    return;
}

double NocLink::get_bandwidth() const {
    return bandwidth;
}

double NocLink::get_congested_bandwidth() const {
    double congested_bandwidth = bandwidth_usage - bandwidth;
    congested_bandwidth = std::max(congested_bandwidth, 0.0);

    VTR_ASSERT(congested_bandwidth >= 0.0);
    return congested_bandwidth;
}

double NocLink::get_congested_bandwidth_ratio() const {
    double congested_bw = get_congested_bandwidth();
    double congested_bw_ratio = congested_bw / get_bandwidth();

    VTR_ASSERT(congested_bw_ratio >= 0.0);
    return congested_bw_ratio;
}

NocLinkId NocLink::get_link_id() const {
    return id;
}

NocLink::operator NocLinkId() const {
    return get_link_id();
}