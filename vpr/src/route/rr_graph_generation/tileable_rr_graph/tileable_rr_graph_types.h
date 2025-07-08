#pragma once

#include "vtr_array.h"

/**
 * @file tileable_rr_graph_types.h
 * @brief Data types required by routing resource graph (RRGraph) definition
 */

/**
 * @brief Directionality of a routing track (node type CHANX and CHANY) in
 * a routing resource graph
 */
enum class e_direction {
    INC_DIRECTION = 0,
    DEC_DIRECTION = 1,
    BI_DIRECTION = 2,
    NO_DIRECTION = 3,
    NUM_DIRECTIONS
};

/**
 * @brief String used in describe_rr_node() and write_xml_rr_graph_obj()
 */
constexpr vtr::array<e_direction, const char*, static_cast<size_t>(e_direction::NUM_DIRECTIONS)> DIRECTION_STRING_WRITE_XML = {
    "INC_DIR",
    "DEC_DIR",
    "BI_DIR",
    "NO_DIR"
};
