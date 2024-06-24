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
     * @brief Get a mutable reference to the UserPlaceConstraints instance.
     */
    UserPlaceConstraints& mutable_place_constraints();

    /**
     * @brief Get a mutable reference to the UserRouteConstraints instance.
     */
    UserRouteConstraints& mutable_route_constraints();

    /**
     * @brief Get a const reference to the UserPlaceConstraints instance.
     */
    const UserPlaceConstraints& place_constraints() const;

    /**
     * @brief Get a const reference to the UserRouteConstraints instance.
     */
    const UserRouteConstraints& route_constraints() const;

    /**
     * @brief add route constraint
     *
     *   @param net_name the route constraint 
     */
    void add_route_constraint(UserRouteConstraints rc);

    /**
     * @brief returns route constraint by index
     *
     *   @param index the constraint index 
     */
    const UserRouteConstraints get_route_constraint_by_idx(std::size_t index) const;

    /**
     * @brief returns route constraint of a specific net
     *
     *   @param net_name the net name
     */
    const UserRouteConstraints get_route_constraint_by_net_name(std::string net_name);

    /**
     * @brief returns number of route constraints
     *
     *   @param void
     */
    int get_route_constraint_num(void) const;

  private:
   
    UserRouteConstraints route_constraints_;
    UserPlaceConstraints placement_constraints_;

};

#endif /* VPR_CONSTRAINTS_H */
