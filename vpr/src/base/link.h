#ifndef LINK_H
#define LINK_H

#include <iostream>
#include "router.h"


class link
{
    private:
        Router* source_router;
        Router* sink_router;

        double bandwidth_usage;

        // represents the number of routed communication paths between routers that use
        // this link. Congestion is proportional to this variable.
        double number_of_connections;

    public:
        link(Router* source_router, Router* sink_router);
        ~link();

        // getters
        Router* get_source_router(void);
        Router* get_sink_router(void);
        double get_bandwidth_usage(void);
        double get_number_of_connections(void);
        
        // setters
        void set_bandwidth_usage(double bandwidth_usage);
        void set_number_of_connections(double number_of_connections);

};








#endif