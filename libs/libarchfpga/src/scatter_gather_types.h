#pragma once

#include <string>
#include "switchblock_types.h"

enum class e_scatter_gather_type {
    UNIDIR,
    BIDIR
};

struct t_sg_location {
    e_sb_location type;
    int num;
    std::string sg_link_name;
};

enum class e_sg_link_offset_dim {
    X,
    Y,
    Z
};

struct t_sg_link {
    std::string name;
    e_sg_link_offset_dim offset_dim;
    int offset_length;
    std::string mux_name;
    std::string seg_type;
};

struct t_scatter_gather_pattern {
    std::string name;
    e_scatter_gather_type type;
    t_wireconn_inf gather_pattern;  //TODO: Add side
    t_wireconn_inf scatter_pattern; //TODO: Add side
    std::vector<t_sg_link> sg_links;
    std::vector<t_sg_location> sg_locations;
};
