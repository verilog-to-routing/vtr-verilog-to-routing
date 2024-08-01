#ifndef USER_ROUTE_CONSTRAINTS_H
#define USER_ROUTE_CONSTRAINTS_H

#include "clock_modeling.h"
#include "vpr_error.h"
#include <unordered_map>
#include <regex>

/**
 * @brief This class specifies a routing scheme for a global net.
 * 
 * Global nets, such as clocks, may require special
 * handling. Clocks are marked as global by default in VPR, but other nets can be specified
 * as global in a user routing constraints file.
 * 
 * The variable `route_model_` can decide between different routing schemes:
 *   - "ideal": The net is not routed.
 *   - "route": The net is routed through the general routing fabric.
 *   - "dedicated_network": The net is routed through a dedicated global clock network using 
 *                          the two-stage router. In the first stage the net source is routed 
 *                          to the clock network root and in the second stage the net is routed
 *                          from the clock network root to the sinks
 * In the third case, the variable `network_name_` specifies the name of the clock network
 * through which the net should be routed.
 */
class RoutingScheme {
  private:
    std::string network_name_ = "INVALID"; // Name of the clock network (if applicable)
    e_clock_modeling route_model_ = e_clock_modeling::ROUTED_CLOCK;

  public:
    // Constructors
    RoutingScheme() = default;
    RoutingScheme(const std::string network_name, const e_clock_modeling route_model)
        : network_name_(network_name)
        , route_model_(route_model) {}

    // Getters
    std::string network_name() const {
        return network_name_;
    }

    e_clock_modeling route_model() const {
        return route_model_;
    }

    // Setters
    void set_network_name(const std::string& network_name) {
        network_name_ = network_name;
    }

    void set_route_model(e_clock_modeling route_model) {
        route_model_ = route_model;
    }

    // Reset network_name_ and route_model_ to their default values
    void reset() {
        network_name_ = "INVALID";
        route_model_ = e_clock_modeling::ROUTED_CLOCK;
    }
};

/**
 * @brief This class is used to store information related to global route constraints from a constraints XML file.
 *
 * In the XML file, you can specify a list of global route constraints where you can specify the name of the net
 * you want to be treated as global. Then, you can define the routing scheme, including the routing method and the
 * name of the global network you want the net to be routed through (if applicable).
 *
 * This class contains an unordered map that stores the name of the global net as the key and the routing scheme
 * for that net as the value. The name can also be a wildcard.
 */
class UserRouteConstraints {
  public:
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
     * The index refers to the position of the key-value entry in the unordered map that stores
     * the route constraints. This function has a linear complexity, and calling it in an outer
     * loop may result in performance issues. Currently, it is primarily used in auto-generated
     * constraint writer code that loops over the number of constraints and fetches them by index.
     *
     *    @param index The index of the key-value entry in the unordered map.
     *    @return A pair containing the net name and its corresponding routing scheme.
     */
    const std::pair<std::string, RoutingScheme> get_route_constraint_by_idx(std::size_t idx) const;

    /**
     * @brief Check if a routing constraint has been specified for a specific net.
     *
     * The net name can include wildcard / regex patterns. The function first searches for an exact match of the net name in the route_constraints collection.
     * If no exact match is found, the method iterates through the entries in the collection,
     * attempting to match the net name using wildcard or regex patterns.
     * 
     * @param net_name The name of the net to check for a routing constraint.
     * @return True if a routing constraint has been specified for the net, false otherwise.
     */
    bool has_routing_constraint(std::string net_name) const;

    /**
     * @brief Get the routing scheme for a specific net by its name.
     *
     * The net name may include wildcard patterns, which will be supported.
     *
     * @param net_name The name of the net for which to retrieve the routing scheme.
     * @return The routing scheme associated with the specified net.
     */
    const RoutingScheme get_route_scheme_by_net_name(std::string net_name) const;

    /**
     * @brief Get the routing model for a specific net by its name.
     *
     * This function retrieves the routing scheme associated with a specific net and 
     * then obtains the routing method from the retrieved routing scheme.
     * 
     *    @param net_name The name of the net for which to retrieve the routing method.
     *    @return The routing method associated with the specified net.
     *  
     * Note: This is a convenience method that just extracts part of the routing scheme.
     * 
     */
    e_clock_modeling get_route_model_by_net_name(std::string net_name) const;

    /**
     * @brief Get the name of the routing network for a specific net by its name.
     *
     * If the net's routing scheme is "dedicated_network", this function will return
     * the name of the dedicated global clock network specified in the routing scheme. 
     * Otherwise, the function will return the default value "INVALID" since the network name
     * is meaningful only when the routing method is "dedicated_network".
     *
     *    @param net_name The name of the net for which to retrieve the name of the routing network.
     *    @return The name of the routing network associated with the specified net.
     */
    const std::string get_routing_network_name_by_net_name(std::string net_name) const;

    /**
     * @brief Get the total number of user-specified global route constraints.
     */
    int get_num_route_constraints(void) const;

  private:
    /**
     * store all route constraints 
     */
    std::unordered_map<std::string, RoutingScheme> route_constraints_;
};
#endif /* USER_ROUTE_CONSTRAINTS_H */