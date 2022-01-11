#include "noc_link.h"


// this represents a link in the NoC
NocLink::NocLink(NocRouter* source, NocRouter* sink):source_router(source), sink_router(sink)
{

}

NocLink::~NocLink()
{

}

// getters
NocRouter* NocLink::get_source_router(void)
{
    return source_router;
}

NocRouter* NocLink::get_sink_router(void)
{
    return sink_router;
}

double NocLink::get_bandwidth_usage(void)
{
    return bandwidth_usage;
}

double NocLink::get_number_of_connections(void)
{
    return number_of_connections;
}

//setters
void NocLink::set_bandwidth_usage(double bandwidth_usage)
{
    this->bandwidth_usage = bandwidth_usage;
}

void NocLink::set_number_of_connections(double number_of_connections)
{
    this->number_of_connections = number_of_connections;
}