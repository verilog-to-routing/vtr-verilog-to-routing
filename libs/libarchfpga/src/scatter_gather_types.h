#pragma once

#include <string>
#include "switchblock_types.h"

/**
 * @brief Enumeration for the type field of an <sg_pattern> tag. With UNIDIR the gather pattern makes a mux which is connected by a node
 * to the scatter pattern of edges. With BIDIR, the structure is made symmetric with a gather mux and a scatter edge pattern on each end
 * of the node linking them.
 */
enum class e_scatter_gather_type {
    UNIDIR, ///< Unidirectional connection
    BIDIR   ///< Bidirectional connection
};

/**
 * @brief Struct containing information of an <sg_location> tag. An <sg_location> tag instantiates the scatter-gather pattern in some switchblock locations around the device.
 */
struct t_sg_location {
    e_sb_location type;       ///< Type of locations that the pattern is instantiated at.
    int num;                  ///< Number of scatter-gather pattern instantiations per location.
    std::string sg_link_name; ///< Name of scatter-gather link to be used.
};

/**
 * @brief Struct containing information of a <sg_link> tag. This tag describes how and where the scatter (fanout) happens relative to the gather (fanin).
 * 
 */
struct t_sg_link {
    std::string name;     ///< Name of the sg_link.
    std::string mux_name; ///< Name of the multiplexer used to gather connections.
    std::string seg_type; ///< Segment/wire used to move through the device to the scatter location.
    int x_offset;         ///< X offset of where the scatter happens relative to the gather. If set, the y and z offsets must be zero.
    int y_offset;         ///< y offset of where the scatter happens relative to the gather. If set, the x and z offsets must be zero.
    int z_offset;         ///< z offset of where the scatter happens relative to the gather. If set, the x and y offsets must be zero.
};

/**
 * @brief Struct containing information of a <sg_pattern> tag. When instantiated in the device using sg_locations,
 * a scatter-gather pattern defined by this struct gathers connections according to gather_pattern, moves through
 * the device using one of the sg_links and fans out or scatters the connections according to scatter_pattern.
 */
struct t_scatter_gather_pattern {
    std::string name;
    e_scatter_gather_type type;
    t_wireconn_inf gather_pattern;
    t_wireconn_inf scatter_pattern;
    std::vector<t_sg_link> sg_links;
    std::vector<t_sg_location> sg_locations;
};
