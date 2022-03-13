#ifndef NOC_STORAGE_H
#define NOC_STORAGE_H


#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include "noc_data_types.h"
#include "vtr_vector.h"
#include "noc_router.h"
#include "noc_link.h"
#include "vtr_assert.h"
#include "vpr_error.h"


class NocStorage
{
    private:
        
        // list of routers in the noc
        vtr::vector<NocRouterId, NocRouter> router_storage;

        // list of outgoing links for each router
        vtr::vector<NocRouterId, std::vector<NocLinkId>> router_link_list;

        // list of links in the noc
        vtr::vector<NocLinkId, NocLink> link_storage;

        // The user provides an id for routers when describing the noc in the architecture file.
        // This id system will be different than than the NocRouterIds assigned to each router
        // when creating the NoC datastructre.
        // A conversion table is created below that maps the user provided router ids to the corresponding
        // NocRouterId.
        std::unordered_map<int, NocRouterId> router_id_conversion_table;

        // flags to keep track of the status
        bool built_noc;

        // prevent "copying" of this object
        NocStorage(const NocStorage&) = delete;
        void operator=(const NocStorage&) = delete;
    
    public:

        // default contructor (cleare all the elements in the vectors)
        NocStorage();

        // getters for the NoC
        const std::vector<NocLinkId>& get_noc_router_connections(NocRouterId id) const;
        const vtr::vector<NocRouterId, NocRouter>& get_noc_routers(void) const;
        const vtr::vector<NocLinkId, NocLink>& get_noc_links(void) const;

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

        // setters for the noc router
        void set_noc_router_design_module_ref(NocRouterId id, std::string design_module_ref);

        // setters for the noc link
        void set_noc_link_bandwidth_usage(NocLinkId id, double bandwidth_usage);
        void set_noc_link_number_of_connections(NocLinkId id, int number_of_connections);

        // general utiliy functions
        void finished_building_noc();
        void clear_noc();
        NocRouterId convert_router_id(int id) const;
        void make_room_for_noc_router_link_list();

        


};



#endif