#ifndef NOC_STORAGE_H
#define NOC_STORAGE_H

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
 * A link is a component of the NoC ans is defined by the
 * NocLink class. Links are connections between two routers.
 * Links are used by routers to communicate with other routers
 * in the NoC. Thye can be thought of as edges in a graph. Links
 * have a source router where they exit from and sink router where
 * they enter. It is important to note that the links are not
 * unidirectional, the legal way to traverse a link is from the
 * source router of the link to the sink router.
 * 
 */

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
#include "echo_files.h"

// represents the id of a link that does not exist in the NoC
constexpr NocLinkId INVALID_LINK_ID(-1);

class NocStorage {
  private:
    vtr::vector<NocRouterId, NocRouter> router_storage; /*<! Contains all the routers in the NoC*/

    // list of outgoing links for each router
    /**
     * @brief Stores outgoing links for each router in the NoC. These
     * links can be used by the router to communicate to other routers
     * in the NoC.
     * 
     */
    vtr::vector<NocRouterId, std::vector<NocLinkId>> router_link_list;

    vtr::vector<NocLinkId, NocLink> link_storage; /*<! Contains all the links in the NoC*/

    /**
     * @brief The user provides an ID for the router when describing the NoC
     * in the architecture file. This ID system will be different than the
     * NocRouterIds assigned to each router. The user ID system will be 
     * arbritary but the internal ID system used here will start at 0 and
     * are dense since it is used to index the routers. The datastructure
     * below is a conversiont able that maps the user router IDs to the
     * correspondint internal ones.
     * 
     */
    std::unordered_map<int, NocRouterId> router_id_conversion_table;

    /**
     * @brief A flag that indicates whether the NoC has been built. If this
     * flag is true, then the NoC cannot be modified, meaning that routers and
     * links cannot be added or removed. The inteded use of this flag is to
     * set it after you complete building the NoC (adding routers and links).
     * This flag can then acts as a check so that the NoC is not modified
     * later on after building it.
     * 
     */
    bool built_noc;

    /**
     * @brief Represents the maximum allowed bandwidth for the links in the NoC (in Gbps)
     */
    double noc_link_bandwidth;

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

    // prevent "copying" of this object
    NocStorage(const NocStorage&) = delete;
    void operator=(const NocStorage&) = delete;

  public:
    // default contructor (cleare all the elements in the vectors)
    NocStorage();

    // getters for the NoC

    /**
     * @brief Gets a vector of outgoing links for a given router
     * in the NoC. THe link vector cannot be modified.
     * 
     * @param id A unique indetifier that represents a router
     * @return A vector of links. The links are represented by a unique
     * identifier.
     */
    const std::vector<NocLinkId>& get_noc_router_connections(NocRouterId id) const;

    /**
     * @brief Get all the routers in the NoC. The routers themselves cannot
     * be modified. This function should be used to when information on all
     * routers is needed.
     * 
     * @return A vector of routers.
     */
    const vtr::vector<NocRouterId, NocRouter>& get_noc_routers(void) const;

    /**
     * @brief Get all the links in the NoC. The links themselves cannot
     * be modified. This function should be used when information on
     * every link is needed.
     * 
     * @return A vector of links. 
     */
    const vtr::vector<NocLinkId, NocLink>& get_noc_links(void) const;

    /**
     * @brief Get the maximum allowable bandwidth for a link
     * within the NoC.
     * 
     * @return a numeric value that represents the link bandwith in bps
     */

    double get_noc_link_bandwidth(void) const;

    /**
     * @brief Get the latency of traversing through a link in
     * the NoC.
     * 
     * @return a numeric value that represents the link latency in seconds
     */

    double get_noc_link_latency(void) const;

    /**
     * @brief Get the latency of traversing through a router in
     * the NoC.
     * 
     * @return a numeric value that represents the router latency in seconds
     */

    double get_noc_router_latency(void) const;

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
     * @brief Given a unique link identifier, get the corresponding link
     * within the NoC. The link can be modified, so the intended use
     * of this function is tis to retrieve a link to modify it.
     * 
     * @param id A unique link identifier.
     * @return A link (NocLink) that is identified by the given id.
     */
    NocLink& get_single_mutable_noc_link(NocLinkId id);

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
     * @param grid_position_x The horizontal position on the FPGA of the phyical
     * tile that this router represents.
     * @param grid_position_y The vertical position on the FPGA of the phyical
     * tile that this router represents.
     */
    void add_router(int id, int grid_position_x, int grid_position_y);

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
    void add_link(NocRouterId source, NocRouterId sink);

    /**
     * @brief Set the maximum allowable bandwidth for a link
     * within the NoC.
     * 
     */

    void set_noc_link_bandwidth(double link_bandwidth);

    /**
     * @brief Set the latency of traversing through a link in
     * the NoC.
     * 
     */

    void set_noc_link_latency(double link_latency);

    /**
     * @brief Set the latency of traversing through a router in
     * the NoC.
     * 
     */

    void set_noc_router_latency(double router_latency);

    // general utiliy functions
    /**
     * @brief Asserts an internal flag which represents that the NoC
     * has been built. This means that no changes can be made to the
     * NoC (routers and links cannot be added or removed). This function
     * should be called after building the NoC. Guarantees that
     * no future changes can be made.
     * 
     */
    void finished_building_noc(void);

    /**
     * @brief Resets the NoC by clearning all internal datastructures. 
     * This includes deleteing all routers and links. Also all internal
     * IDs are removed (the is conversion table is cleared). It is
     * recommended to run this function before building the NoC.
     * 
     */
    void clear_noc(void);

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
     * @brief The datastructure that stores the outgoing links to each
     * router is an 2-D Vector. When processing the links, they can be
     * outgoing from any router in the NoC. Therefore the column size
     * of the 2-D vector needs to be the size of the number of routers
     * in the NoC. The function below sets the column size to the
     * number of routers in the NoC. 
     * 
     */
    void make_room_for_noc_router_link_list(void);

    /**
     * @brief Two links are considered parallel when the source router of one
     * link is the sink router of the second link and when the sink router
     * of one link is the source router of the other link. Given a link, this
     * functions finds a parallel link, if no link is found then an invalid
     * link is returned.
     * 
     * Example:
     * 
     *  ----------                       ----------
     *  /        /        link 1         /        /
     *  / router / --------------------->/ router /
     *  /   a    / <---------------------/   b    /
     *  /        /        link 2         /        /
     *  /--------/                       /--------/
     * 
     * In the example above, link 1 and link 2 are parallel.
     * 
     * @param current_link A unique identifier that represents a link
     * @return NocLinkId An identifier that represents a link that is parallel
     * to the input link.
     */
    NocLinkId get_parallel_link(NocLinkId current_link) const;

    /**
     * @brief Writes out the NocStirage class infromation to a file. 
     *        This includes the list of routers and their connections
     *        to other routers in the NoC.
     * 
     * @param file_name The name of the file that contains the NoC model info.
     */
    void echo_noc(char* file_name) const;
};

#endif