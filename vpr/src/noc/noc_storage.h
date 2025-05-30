#pragma once
/**
 * @file
 * @brief This file defines the NocStorage class.
 * 
 * Overview
 * ======== 
 * The NocStorage class represents the model of the embedded NoC
 * in the FPGA device. The model describes the topology of the NoC,
 * its placement on the FPGA and properties of the NoC components. 
 * The NocStorage consists of two main
 * components, which are routers and links. The routers and
 * links can be accessed to retrieve information about them
 * using unique identifier (NocRouterId, NocLinkId). Each router
 * and link modelled in the NoC has a unique ID.
 * 
 * Router
 * ------
 * A router is component of the NoC and is defined by the
 * NocRouter class. Routers are represent physical FPGA tiles
 * and represent entry and exit points to and from the NoC. 
 * 
 * Link
 * ----
 * A link is a component of the NoC and is defined by the
 * NocLink class. Links are connections between two routers.
 * Links are used by routers to communicate with other routers
 * in the NoC. They can be thought of as edges in a graph. Links
 * have a source router where they exit from and sink router where
 * they enter. It is important to note that the links are not
 * bi-directional; the legal way to traverse a link is from the
 * source router of the link to the sink router.
 * 
 */

#include <vector>
#include <unordered_map>
#include "noc_data_types.h"
#include "vpr_types.h"
#include "vtr_vector.h"
#include "noc_router.h"
#include "noc_link.h"

class NocStorage {
  private:
    /** Contains all the routers in the NoC*/
    vtr::vector<NocRouterId, NocRouter> router_storage;

    // list of outgoing links for each router
    /**
     * @brief Stores outgoing links for each router in the NoC. These
     * links can be used by the router to communicate to other routers
     * in the NoC.
     */
    vtr::vector<NocRouterId, std::vector<NocLinkId>> router_outgoing_links_list;

    /**
     * @brief Stores incoming links for each router in the NoC. These
     * links can be used by the router to communicate to other routers
     * in the NoC.
     */
    vtr::vector<NocRouterId, std::vector<NocLinkId>> router_incoming_links_list;

    /** Contains all the links in the NoC*/
    vtr::vector<NocLinkId, NocLink> link_storage;

    /**
     * @brief The user provides an ID for the router when describing the NoC
     * in the architecture file. This ID system will be different than the
     * NocRouterIds assigned to each router. The user ID system will be 
     * arbitrary but the internal ID system used here will start at 0 and
     * are dense since it is used to index the routers. The data structure
     * below is a conversiont able that maps the user router IDs to the
     * corresponding internal ones.
     */
    std::unordered_map<int, NocRouterId> router_id_conversion_table;

    /**
     * @brief Associates the hard (physical) routers on the device to their grid
     * location. During placement, when logical routers are moved to
     * different hard routers, only the grid location of where the
     * logical router was moved is known.
     * Using this data structure, the grid location can be used to
     * identify the corresponding hard router block positioned at that grid 
     * location. The NocROuterId uniquely identifies hard router blocks and
     * can be used to retrieve the hard router block information using
     * the router_storage data structure above. This can also be used to
     * access the connectivity graph data structure above.
     * 
     * It is important to know the specific hard router block because 
     * without it we cannot determine the starting/end points of the traffic
     * flows associated to the moved logical router. We need this
     * so that we can re-route all traffic flows and evaluate the
     * the placement cost of the moved logical router block.
     * 
     * The intended use is when trying to re-route a traffic flow. The current
     * location of a logical router block can be used in conjunction with this
     * data structure to identify the corresponding hard router block.
     * 
     */
    std::unordered_map<int, NocRouterId> grid_location_to_router_id;

    /**
     * @brief A flag that indicates whether the NoC has been built. If this
     * flag is true, then the NoC cannot be modified, meaning that routers and
     * links cannot be added or removed. The intended use of this flag is to
     * set it after you complete building the NoC (adding routers and links).
     * This flag can then acts as a check so that the NoC is not modified
     * later on after building it.
     * 
     */
    bool built_noc;

    /**
     * @brief Represents the delay expected when going through a link (in
     * seconds)
     */
    double noc_link_latency;

    /**
     * @brief Represents the expected delay when going through a router (in
     * seconds))
     */
    double noc_router_latency;

    /**
     * @brief When set true, specifies that some NoC routers have different
     * latencies than others. When set false, all the NoC routers have the same
     * latency.
     */
    bool detailed_router_latency_;

    /**
     * @brief When set true, specifies that some NoC links have different
     * latencies than others. When set false, all the NoC link have the same
     * latency.
     */
    bool detailed_link_latency_;

    /**
     * @brief Indicates whether the NoC is 3D or 2D.
     * A 3D NoC has routers in at least two different layers.
     * In a 3D FPGA architecture, the NoC is not necessarily 3D.
     */
    bool multi_layer_noc_;

    /**
     * @brief A constant reference to this vector is returned by get_noc_links(...).
     * This is used to avoid memory allocation whenever get_noc_links(...) is called.
     * The vector is mutable so that get_noc_links(...), which is a const method, can
     * modify it.
     */
    mutable std::vector<std::reference_wrapper<const NocLink>> returnable_noc_link_const_refs_;

    /**
     * @brief Internal reference to the device grid width. This is necessary
     * to compute a unique key for a given grid location which we can then use
     * to get the corresponding physical (hard) router at the given grid
     * location using 'grid_location_to_router_id'. 
     * 
     */
    int device_grid_width;
    /**
     * @brief Internal reference to the number of blocks at each layer (width * height). This is necessary
     * to compute a unique key for a given grid location which we can then use
     * to get the corresponding physical (hard) router at the given grid
     * location using 'grid_location_to_router_id'.
     */
    int layer_num_grid_locs;

  public:
    // default constructor (clear all the elements in the vectors)
    NocStorage();

    // prevent "copying" of this object
    NocStorage(const NocStorage&) = delete;
    void operator=(const NocStorage&) = delete;

    // getters for the NoC

    /**
     * @brief Gets a vector of outgoing links for a given router
     * in the NoC. The link vector cannot be modified.
     * 
     * @param id A unique identifier that represents a router
     * @return A vector of links. The links are represented by a unique
     * identifier.
     */
    const std::vector<NocLinkId>& get_noc_router_outgoing_links(NocRouterId id) const;

    /**
     * @brief Gets a vector of incoming links for a given router
     * in the NoC.
     *
     * @param id A unique identifier that represents a router
     * @return A vector of links. The links are represented by a unique
     * identifier.
     */
    const std::vector<NocLinkId>& get_noc_router_incoming_links(NocRouterId id) const;

    /**
     * @brief Get all the routers in the NoC. The routers themselves cannot
     * be modified. This function should be used to when information on all
     * routers is needed.
     * 
     * @return A vector of routers.
     */
    const vtr::vector<NocRouterId, NocRouter>& get_noc_routers() const;

    /**
     * @return An integer representing the total number of routers within the
     * NoC.
     */
    int get_number_of_noc_routers() const;

    /**
     * @brief Get all the links in the NoC. The links themselves cannot
     * be modified. This function should be used when information on
     * every link is needed.
     * 
     * @return A vector of links. 
     */
    const vtr::vector<NocLinkId, NocLink>& get_noc_links() const;

    /**
     * @brief Get all the links in the NoC. The links themselves can
     * be modified. This function should be used when information on
     * every link needs to be modified.
     *
     * @return A vector of links.
     */
    vtr::vector<NocLinkId, NocLink>& get_mutable_noc_links();

    /**
     * @return An integer representing the total number of links within the
     * NoC.
     */
    int get_number_of_noc_links() const;

    /**
     * @brief Get the latency of traversing through a link in
     * the NoC.
     * 
     * @return a numeric value that represents the link latency in seconds
     */
    double get_noc_link_latency() const;

    /**
     * @brief Get the latency of traversing through a router in
     * the NoC.
     * 
     * @return a numeric value that represents the router latency in seconds
     */
    double get_noc_router_latency() const;

    /**
     * @return True if some NoC routers have different latencies than others.
     * False if all NoC routers have the same latency.
     */
    bool get_detailed_router_latency() const;

    /**
     * @return True if some NoC links have different latencies than others.
     * False if all NoC links have the same latency.
     */
    bool get_detailed_link_latency() const;

    /**
     * @return True if some NoC links have different bandwidths than others.
     * False if all NoC links have the same bandwidth.
     */
    bool get_detailed_link_bandwidth() const;

    // getters for  routers

    /**
     * @brief Given a unique router identifier, get the corresponding router
     * within the NoC. The router cannot be modified, so the intended use
     * of this function is to retrieve information about a specific router.
     * 
     * @param id A unique router identifier.
     * @return A router (NocRouter) that is identified by the given id.
     */
    const NocRouter& get_single_noc_router(NocRouterId id) const;

    /**
     * @brief Given a unique router identifier, get the corresponding router
     * within the NoC. The router can be modified, so the intended use
     * of this function is to retrieve a router to modify it.
     * 
     * @param id A unique router identifier.
     * @return A router (NocRouter) that is identified by the given id.
     */
    NocRouter& get_single_mutable_noc_router(NocRouterId id);

    // getters for links

    /**
     * @brief Given a unique link identifier, get the corresponding link
     * within the NoC. The link cannot be modified, so the intended use
     * of this function is to retrieve information about a specific link.
     * 
     * @param id A unique link identifier.
     * @return A link (NocLink) that is identified by the given id.
     */
    const NocLink& get_single_noc_link(NocLinkId id) const;

    /**
     *
     * @tparam Container The type of standard library container used to carry
     * NoCLinkIds. This container type must be iterable in a range-based loop.
     * @tparam Ts Used to help clang infer correct template types. GCC can compile
     * without this extra template argument.
     * @param noc_link_ids A standard container that contains NoCLinkIds of the
     * requested NoC links
     * @return A const
     */
    template<template<typename...> class Container, typename... Ts>
    const std::vector<std::reference_wrapper<const NocLink>>& get_noc_links(const Container<NocLinkId, Ts...>& noc_link_ids) const;

    /**
     * @brief Given source and sink router identifiers, this function
     * finds a link connecting these routers and returns its identifier.
     * If such a link does not exist, an invalid id is returned.
     * The function is not optimized for performance as it has a complexity
     * of O(N_links).
     *
     * @param src_router The unique router identifier for the source router.
     * @param dst_router The unique router identifier for the destination router.
     * @return A link identifier (NocLinkId) that connects the source router
     * to the destination router. NocLinkId::INVALID() is such a link is not
     * found.
     */
    NocLinkId get_single_noc_link_id(NocRouterId src_router, NocRouterId dst_router) const;

    /**
     * @brief Given a unique link identifier, get the corresponding link
     * within the NoC. The link can be modified, so the intended use
     * of this function is tis to retrieve a link to modify it.
     * 
     * @param id A unique link identifier.
     * @return A link (NocLink) that is identified by the given id.
     */
    NocLink& get_single_mutable_noc_link(NocLinkId id);

    /**
     * @brief Given a grid location of a hard router block on
     * the FPGA device this function determines the id of the hard
     * router block positioned on that grid location.
     * 
     * @param hard_router_location A struct that contains the grid location
     * of an arbitrary hard router block on the FPGA.
     * @return NocRouterId The hard router block "id"  
     * located at the given grid location. 
     */
    NocRouterId get_router_at_grid_location(const t_pl_loc& hard_router_location) const;

    /**
     * @brief Indicates whether the NoC is 3D.
     * @return True if there are NoC routers in different layers.
     */
    bool is_noc_3d() const;

    // setters for the NoC

    /**
     * @brief Creates a new router and adds it to the NoC. When the router
     * is created, its corresponding internal id (NocRouterId) is also created
     * and a conversion between the user supplied id to the internal id is
     * setup. If "finish_building_noc()" was called then calling this function
     * after will throw an error as the NoC cannot be modified after building
     * the NoC.
     * 
     * @param id The user supplied identification for the router.
     * @param grid_position_x The horizontal position on the FPGA of the physical
     * tile that this router represents.
     * @param grid_position_y The vertical position on the FPGA of the physical
     * tile that this router represents.
     * @param layer_position The layer where the physical tile that this router
     * represents is located.
     * @param latency The zero-load latency that a traffic flow will experience
     * when it is routed through this router.
     */
    void add_router(int id,
                    int grid_position_x,
                    int grid_position_y,
                    int layer_position,
                    double latency);

    /**
     * @brief Creates a new link and adds it to the NoC. The newly created
     * links internal id (NocLinkId) is then added to the vector of outgoing
     * links of its source router. If "finish_building_noc()" was called then 
     * calling this function after will throw an error as the NoC cannot be
     * modified after building the NoC.
     * 
     * @param source A unique identifier for the router that the new link exits
     * from (outgoing from the router)
     * @param sink A unique identifier for the router that the new link enters
     * into (incoming to the router) 
     */
    void add_link(NocRouterId source, NocRouterId sink, double bandwidth, double latency);

    /**
     * @brief Set the maximum allowable bandwidth for all links
     * within the NoC
     */
    void set_noc_link_bandwidth(double link_bandwidth);

    /**
     * @brief Set the latency of traversing through a link in
     * the NoC.
     */
    void set_noc_link_latency(double link_latency);

    /**
     * @brief Set the latency of traversing through a router in
     * the NoC.
     */
    void set_noc_router_latency(double router_latency);

    /**
     * @brief Set the internal reference to the device
     * grid width.
     */
    void set_device_grid_width(int grid_width);

    void set_device_grid_spec(int grid_width, int grid_height);

    // general utility functions
    /**
     * @brief The link is removed from the outgoing vector of links for
     * the source router. The link is not removed from the vector of all
     * links as this will require a re-indexing of all link ids. Instead,
     * the link is set to being invalid by. The link
     * is still removed since it will be considered invalid when used 
     * externally. THe link is identified by going through the vector 
     * outgoing links of the supplied source router, for each outgoing link
     * the sink router is compared the supplied sink router and the link to
     * remove is identified if there is a match.
     * If the link doesn't exist in the
     * NoC then a warning message is printed and a boolean status is updated
     * indicating that the link does not exist in the NoC.
     * 
     * @param src_router_id The source router of the traffic flow to delete
     * @param sink_router_id The sink router of the traffic flow to delete
     * @return true The link was successfully removed
     * @return false The link was not removed
     */
    bool remove_link(NocRouterId src_router_id, NocRouterId sink_router_id);

    /**
     * @brief Asserts an internal flag which represents that the NoC
     * has been built. This means that no changes can be made to the
     * NoC (routers and links cannot be added or removed). This function
     * should be called after building the NoC. Guarantees that
     * no future changes can be made.
     *
     * When the NoC building is finished, this function checks whether
     * all links and routers have the same bandwidth and latency.
     * If some NoC elements have different latencies or bandwidths than
     * others, a flag is set to indicate that the detailed NoC model should be
     * used. In the detailed model, instead of associating a single latency or
     * bandwidth value with all NoC routers or links, each NoC router or link
     * has its specific value.
     */
    void finished_building_noc();

    /**
     * @brief Resets the NoC by clearing all internal data structures.
     * This includes deleting all routers and links. Also all internal
     * IDs are removed (the is conversion table is cleared). It is
     * recommended to run this function before building the NoC.
     */
    void clear_noc();

    /**
     * @brief Given a user id of a router, this function converts
     * the id to the equivalent internal NocRouterId. If there were
     * no routers in the NoC with the given id an error is thrown.
     * 
     * @param id The user supplied identification for the router.
     * @return The equivalent internal NocRouterId.
     */
    NocRouterId convert_router_id(int id) const;

    /**
     * @brief Converts a NoCRouterID to the user router id.
     * @param id The internal NoCRouterId
     * @return The user provided router id;
     */
    int convert_router_id(NocRouterId id) const;

    /**
     * @brief The data structure that stores the outgoing links to each
     * router is an 2-D Vector. When processing the links, they can be
     * outgoing from any router in the NoC. Therefore the column size
     * of the 2-D vector needs to be the size of the number of routers
     * in the NoC. The function below sets the column size to the
     * number of routers in the NoC. 
     * 
     */
    void make_room_for_noc_router_link_list();

    /**
     * @brief Two links are considered parallel when the source router of one
     * link is the sink router of the second link and when the sink router
     * of one link is the source router of the other link. Given a link, this
     * functions finds a parallel link, if no link is found then an invalid
     * link is returned.
     * 
     * Example:
     * ```
     *  ----------                       ----------
     *  /        /        link 1         /        /
     *  / router / --------------------->/ router /
     *  /   a    / <---------------------/   b    /
     *  /        /        link 2         /        /
     *  /--------/                       /--------/
     * ```
     * In the example above, link 1 and link 2 are parallel.
     * 
     * @param current_link A unique identifier that represents a link
     * @return NocLinkId An identifier that represents a link that is parallel
     * to the input link.
     */
    NocLinkId get_parallel_link(NocLinkId current_link) const;

    /**
     * @brief Generates a unique integer using the x and y coordinates of a 
     * hard router block that can be used to identify it. This should be
     * used to generate the keys for the 'grid_location_to_router_id'
     * data structure.
     * 
     * The key will be generated as follows:
     * key = y * device_grid.width() + x
     * 
     * @param grid_position_x The horizontal position on the FPGA of the physical
     * tile that this router represents.
     * 
     * @param grid_position_y The vertical position on the FPGA of the physical
     * tile that this router represents.
     * 
     * @param layer_position The layer number of the physical
     * tile that this router represents.
     *  
     * @return int Represents a unique key that can be used to identify a
     * hard router block.
     */
    int generate_router_key_from_grid_location(int grid_position_x, int grid_position_y, int layer_position) const;

    /**
     * @brief Writes out the NocStorage class information to a file.
     *        This includes the list of routers and their connections
     *        to other routers in the NoC.
     * 
     * @param file_name The name of the file that contains the NoC model info.
     */
    void echo_noc(char* file_name) const;
};

template<template<typename...> class Container, typename... Ts>
const std::vector<std::reference_wrapper<const NocLink>>& NocStorage::get_noc_links(const Container<NocLinkId, Ts...>& noc_link_ids) const {
    returnable_noc_link_const_refs_.clear();

    std::transform(noc_link_ids.begin(), noc_link_ids.end(), std::back_inserter(returnable_noc_link_const_refs_),
                   [this](const NocLinkId lid) {
                       return std::reference_wrapper<const NocLink>(this->get_single_noc_link(lid));
                   });

    return returnable_noc_link_const_refs_;
}
