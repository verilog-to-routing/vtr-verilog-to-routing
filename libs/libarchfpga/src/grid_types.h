#pragma once

/**
 * @file grid_types.h
 * @brief FPGA grid layout data types 
 */

#include <string>
#include <memory>

// Forward declerations

struct t_metadata_dict;

/**
 * @brief Grid location specification
 * Each member is a formula evaluated in terms of 'W' (device width),
 * and 'H' (device height). Formulas can be evaluated using parse_formula()
 * from expr_eval.h.
 */
struct t_grid_loc_spec {
    std::string start_expr; ///<Starting position (inclusive)
    std::string end_expr;   ///<Ending position (inclusive)

    std::string repeat_expr; ///<Distance between repeated region instances

    std::string incr_expr; ///<Distance between block instantiations with the region
};

/**
 * @brief Definition of how to place physical logic block in the grid.
 * @details This defines a region of the grid to be set to a specific type
 * (provided its priority is high enough to override other blocks).
 *
 * The diagram below illustrates the layout specification.
 *
 *                      +----+                +----+           +----+
 *                      |    |                |    |           |    |
 *                      |    |                |    |    ...    |    |
 *                      |    |                |    |           |    |
 *                      +----+                +----+           +----+
 *
 *                        .                     .                 .
 *                        .                     .                 .
 *                        .                     .                 .
 *
 *                      +----+                +----+           +----+
 *                      |    |                |    |           |    |
 *                      |    |                |    |    ...    |    |
 *                      |    |                |    |           |    |
 *                      +----+                +----+           +----+
 *                   ^
 *                   |
 *           repeaty |
 *                   |
 *                   v        (endx,endy)
 *                      +----+                +----+           +----+
 *                      |    |                |    |           |    |
 *                      |    |                |    |    ...    |    |
 *                      |    |                |    |           |    |
 *                      +----+                +----+           +----+
 *       (startx,starty)
 *                            <-------------->
 *                                 repeatx
 *
 * startx/endx and endx/endy define a rectangular region instance's dimensions.
 * The region instance is then repeated every repeatx/repeaty (if specified).
 *
 * Within a particular region instance a block of block_type is laid down every
 * incrx/incry units (if not specified defaults to block width/height):
 *
 *
 *    * = an instance of block_type within the region
 *
 *                    +------------------------------+
 *                    |*         *         *        *|
 *                    |                              |
 *                    |                              |
 *                    |                              |
 *                    |                              |
 *                    |                              |
 *                    |*         *         *        *|
 *                ^   |                              |
 *                |   |                              |
 *          incry |   |                              |
 *                |   |                              |
 *                v   |                              |
 *                    |*         *         *        *|
 *                    +------------------------------+
 *
 *                      <------->
 *                        incrx
 *
 * In the above diagram incrx = 10, and incry = 6
 */
struct t_grid_loc_def {
    t_grid_loc_def(std::string block_type_val, int priority_val)
        : block_type(std::move(block_type_val))
        , priority(priority_val)
        , x{"0", "W-1", "max(w+1,W)", "w"} // Fill in x direction, no repeat, incr by block width
        , y{"0", "H-1", "max(h+1,H)", "h"} // Fill in y direction, no repeat, incr by block height
    {}

    std::string block_type; ///< The block type name

    int priority = 0; ///< Priority of the specification. In case of conflicting specifications the largest priority wins.

    t_grid_loc_spec x; ///< Horizontal location specification
    t_grid_loc_spec y; ///< Vertical location specification

    /**
     * @brief When 1 metadata tag is split among multiple t_grid_loc_def, one
     * t_grid_loc_def is arbitrarily chosen to own the metadata, and the other
     * t_grid_loc_def point to the owned version.
     * 
     */
    std::unique_ptr<t_metadata_dict> owned_meta;

    /**
     * @brief Metadata for this location definition. This
     * metadata may be shared with multiple grid_locs
     * that come from a common definition.
     */
    t_metadata_dict* meta = nullptr;
};

/**
 * @brief Enum for specfying if the architecture grid specification is for an auto sized device (variable size)
 * or a fixed size device.
 */
enum class e_grid_def_type {
    AUTO,
    FIXED
};
