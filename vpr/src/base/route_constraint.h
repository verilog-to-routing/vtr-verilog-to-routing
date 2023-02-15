#ifndef ROUTE_CONSTRAINT_H
#define ROUTE_CONSTRAINT_H

#include "vpr_types.h"

/**
 * @file
 * @brief This file defines the RouteConstraint class.
 */

class RouteConstraint {
  public:
    /**
     * @brief Constructor for the RouteConstraint class, sets member variables to invalid values
     */
    RouteConstraint();

    /**
     * @brief get net name
     */
    std::string get_net_name() const;

    /**
     * @brief set net name
     */
    void set_net_name(std::string);

    /**
     * @brief get net type
     */
    std::string get_net_type() const;

    /**
     * @brief set net type
     */
    void set_net_type(std::string);

    /**
     * @brief get route model
     */
    std::string get_route_model() const;

    /**
     * @brief set route model
     */
    void set_route_model(std::string);

    /**
     * @brief set is valid 
     */
    void set_is_valid(bool);

    /**
     * @brief get is valid 
     */
    bool get_is_valid() const;

  private:
    std::string net_name_;
    std::string net_type_;
    std::string route_method_;
    bool is_valid_;
};

#endif /* ROUTE_CONSTRAINT_H */
