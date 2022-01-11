#ifndef NOC_LINK_H
#define NOC_LINK_H

#include <iostream>
#include "noc_router.h"


class NocLink
{
    private:
        // better to change these to ids
        // instead of references
        NocRouter* source_router;
        NocRouter* sink_router;

        double bandwidth_usage;

        // represents the number of routed communication paths between routers that use
        // this link. Congestion is proportional to this variable.
        double number_of_connections;

    public:
        NocLink(NocRouter* source_router, NocRouter* sink_router);
        ~NocLink();

        // getters
        NocRouter* get_source_router(void);
        NocRouter* get_sink_router(void);
        double get_bandwidth_usage(void);
        double get_number_of_connections(void);
        
        // setters
        void set_bandwidth_usage(double bandwidth_usage);
        void set_number_of_connections(double number_of_connections);

};








#endif