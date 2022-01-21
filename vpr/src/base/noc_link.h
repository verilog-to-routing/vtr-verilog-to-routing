#ifndef NOC_LINK_H
#define NOC_LINK_H

#include <iostream>
#include "noc_router.h"
#include "noc_data_types.h"


class NocLink
{
    private:
        // the two routers that are connected by this link
        NocRouterId source_router;
        NocRouterId sink_router;

        double bandwidth_usage;

        // represents the number of routed communication paths between routers that use
        // this link. Congestion is proportional to this variable.
        int number_of_connections;

    public:
        NocLink(NocRouterId source_router, NocRouterId sink_router);
        ~NocLink();

        // getters
        NocRouterId get_source_router(void) const;
        NocRouterId get_sink_router(void) const;
        double get_bandwidth_usage(void) const;
        int get_number_of_connections(void) const;
        
        // setters
        void set_bandwidth_usage(double bandwidth_usage);
        void set_number_of_connections(int number_of_connections);

};








#endif