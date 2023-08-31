#ifndef VPR_CONSTRAINTS_H
#define VPR_CONSTRAINTS_H

#include "user_place_constraints.h"
#include "user_route_constraints.h"


/**
 * @brief This file defines the VprConstraints class, which encapsulates user-specified placement and routing constraints
 *      
 *
 * Overview
 * ========
 * The VprConstraints class is used to load and manage user-specified constraints from an XML file.
 * It includes instances of the UserRouteConstraints and UserPlaceConstraints classes, which hold routing and placement
 * constraints, respectively.
 *
 * Related Classes
 * ===============
 * 
 * UserRouteConstraints: Stores routing constraints for global nets, specifying routing method and routing network name.
 * See vpr/src/base/user_route_constraints.h for more detail.
 *
 * UserPlaceConstraints: Stores block and region constraints for packing and placement stages.
 * See vpr/src/base/user_place_constraints.h for more detail.
 */


class VprConstraints {
  public:
    /**
     * @brief Store the id of a constrained atom with the id of the partition it belongs to
     *
     *   @param blk_id      The atom being stored
     *   @param part_id     The partition the atom is being constrained to
     */
    void add_constrained_atom(const AtomBlockId blk_id, const PartitionId part_id);

    /**
     * @brief Store a partition
     *
     *   @param part     The partition being stored
     */
    void add_partition(Partition part);

    /**
     * @brief Return a partition
     *
     *   @param part_id    The id of the partition that is wanted
     */
    Partition get_partition(PartitionId part_id);

    /**
     * @brief Return all the atoms that belong to a partition
     *
     *   @param part_id   The id of the partition whose atoms are needed
     */
    std::vector<AtomBlockId> get_part_atoms(PartitionId part_id);

    /**
     * @brief Returns the number of partitions in the object
     */
    int get_num_partitions();

    /**
     * @brief Add a global route constraint for a specific net with its corresponding routing scheme.
     *
     *    @param net_name The name of the net to which the route constraint applies.
     *    @param route_scheme The routing scheme specifying how the net should be routed.
     */
    void add_route_constraint(std::string net_name, RoutingScheme route_scheme);

    /**
     * @brief Get a global route constraint by its index. 
     *
     *    @param index The index of the key-value entry in the unordered map.
     *    @return A pair containing the net name and its corresponding routing scheme.
     */
    const std::pair<std::string, RoutingScheme> get_route_constraint_by_idx(std::size_t idx) const;

    /**
     * @brief Get the routing method for a specific net by its name.
     *
     *    @param net_name The name of the net for which to retrieve the routing method.
     *    @return The routing method associated with the specified net.
     */
    e_clock_modeling get_route_model_by_net_name(std::string net_name) const;

    /**
     * @brief Get the name of the routing network for a specific net by its name.
     *
     *    @param net_name The name of the net for which to retrieve the name of the routing network.
     *    @return The name of the routing network associated with the specified net.
     */
    const std::string get_routing_network_name_by_net_name(std::string net_name) const;

    /**
     * @brief Get the total number of user-specified global route constraints.
     */
    int get_num_route_constraints() const;

    /**
     * @brief Get the UserPlaceConstraints instance.
     */
    const UserPlaceConstraints place_constraints() const;

    /**
     * @brief Get the UserRouteConstraints instance.
     */
    const UserRouteConstraints route_constraints() const;


  private:
   
    UserRouteConstraints route_constraints_;
    UserPlaceConstraints placement_constraints_;

};

#endif /* VPR_CONSTRAINTS_H */
