#pragma once

/**
 * @file tileable_rr_graph_types.h
 * @brief Data types required by routing resource graph (RRGraph) definition
 */

/**
 * @brief Directionality of a routing track (node type CHANX and CHANY) in
 * a routing resource graph
 */
enum e_direction : unsigned char {
    INC_DIRECTION = 0,
    DEC_DIRECTION = 1,
    BI_DIRECTION = 2,
    NO_DIRECTION = 3,
    NUM_DIRECTIONS
};

/* Xifan Tang - string used in describe_rr_node() and write_xml_rr_graph_obj() */
constexpr std::array<const char*, NUM_DIRECTIONS> DIRECTION_STRING_WRITE_XML = {{"INC_DIR", "DEC_DIR", "BI_DIR", "NO_DIR"}};
