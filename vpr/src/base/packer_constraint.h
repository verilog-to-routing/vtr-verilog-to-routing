#ifndef PACKER_CONSTRAINT_H
#define PACKER_CONSTRAINT_H

#include "vpr_types.h"

/**
 * @file
 * @brief This file defines the PackerConstraint class.
 */

class PackerConstraint {
  public:
    /**
     * @brief Constructor for the PackerConstraint class, sets member variables to invalid values
     */
    PackerConstraint();

    /**
     * @brief get net name
     */
    std::string net_name() const;

    /**
     * @brief set net name
     */
    void set_net_name(std::string);

    /**
     * @brief get pin name
     */
    std::string pin_name() const;

    /**
     * @brief set pin name
     */
    void set_pin_name(std::string);

    /**
     * @brief set is valid 
     */
    void set_is_valid(bool);

    /**
     * @brief get is valid 
     */
    bool is_valid() const;

  private:
    std::string net_name_;
    std::string pin_name_;
    bool is_valid_;
};

#endif /* PACKER_CONSTRAINT_H */
