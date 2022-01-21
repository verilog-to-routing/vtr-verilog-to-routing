#ifndef NOC_STORAGE_H
#define NOC_STORAGE_H


#include <iostream>
#include <vector>
#include <string>
#include "noc_data_types.h"
#include "vtr_vector.h"
#include "noc_router.h"
#include "noc_link.h"
#include "vtr_assert.h"

class NocStorage
{
    private:
        
        // list of routers in the noc
        vtr::vector<NocRouterId, NocRouter> router_storage;

        // list of outgoing links for each router
        vtr::vector<NocRouterId, std::vector<NocLinkId>> router_link_list;

        // list of links in the noc
        vtr::vector<NocLinkId, NocLink> link_storage;

        // flags to keep track of the status
        bool built_noc;
    public:

        // default contructor (cleare all the elements in the vectors)
        NocStorage();
        // default destructor (dont have to do anything here)
        ~NocStorage();

        // getters for the NoC
        const std::vector<NocLinkId>& get_noc_router_connections(NocRouterId id) const;

        // getters for  routers
        int get_noc_router_grid_position_x(NocRouterId id) const;
        int get_noc_router_grid_position_y(NocRouterId id) const;
        int get_noc_router_id(NocRouterId id) const;
        std::string get_noc_router_design_module_ref(NocRouterId id) const;


        // getters for links
        NocRouterId get_noc_link_source_router(NocLinkId id)const;
        NocRouterId get_noc_link_sink_router(NocLinkId id) const;
        double get_noc_link_bandwidth_usage(NocLinkId id) const;
        int get_noc_link_number_of_connections(NocLinkId id) const;

        // setters for the NoC
        void add_router(int id, int grid_position_x, int grid_position_y);
        void add_link(NocRouterId source, NocRouterId sink);
        void add_noc_router_link(NocRouterId router_id, NocLinkId link_id);

        // setters for the noc router
        void set_noc_router_design_module_ref(NocRouterId id, std::string design_module_ref);

        // setters for the noc link
        void set_noc_link_bandwidth_usage(NocLinkId id, double bandwidth_usage);
        void set_noc_link_number_of_connections(NocLinkId id, int number_of_connections);

        // general utiliy functions
        void finished_building_noc();
        void clear_noc();

};



#endif