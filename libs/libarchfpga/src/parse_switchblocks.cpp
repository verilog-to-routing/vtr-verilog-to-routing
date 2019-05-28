/*
 * See vpr/SRC/route/build_switchblocks.c for a detailed description of how the new
 * switch block format works and what files are involved.
 *
 *
 * A large chunk of this file is dedicated to helping parse the initial switchblock
 * specificaiton in the XML arch file, providing error checking, etc.
 *
 * Another large chunk of this file is dedicated to parsing the actual formulas
 * specified by the switch block permutation functions into their numeric counterparts.
 */

#include <string.h>
#include <string>
#include <sstream>
#include <vector>
#include <stack>
#include <utility>
#include <algorithm>

#include "vtr_assert.h"
#include "vtr_util.h"

#include "pugixml.hpp"
#include "pugixml_util.hpp"

#include "arch_error.h"

#include "read_xml_util.h"
#include "arch_util.h"
#include "arch_types.h"
#include "physical_types.h"
#include "parse_switchblocks.h"

using namespace std;
using namespace pugiutil;

/**** Function Declarations ****/
/*---- Functions for Parsing Switchblocks from Architecture ----*/

//Load an XML wireconn specification into a t_wireconn_inf
t_wireconn_inf parse_wireconn(pugi::xml_node node, const pugiutil::loc_data& loc_data);

//Process the desired order of a wireconn
static void parse_switchpoint_order(const char* order, SwitchPointOrder& switchpoint_order);

//Process a wireconn defined in the inline style (using attributes)
void parse_wireconn_inline(pugi::xml_node node, const pugiutil::loc_data& loc_data, t_wireconn_inf& wc);

//Process a wireconn defined in the multinode style (more advanced specification)
void parse_wireconn_multinode(pugi::xml_node node, const pugiutil::loc_data& loc_data, t_wireconn_inf& wc);

//Process a <from> or <to> sub-node of a multinode wireconn
t_wire_switchpoints parse_wireconn_from_to_node(pugi::xml_node node, const pugiutil::loc_data& loc_data);

/* parses the wire types specified in the comma-separated 'ch' char array into the vector wire_points_vec.
 * Spaces are trimmed off */
static void parse_comma_separated_wire_types(const char* ch, std::vector<t_wire_switchpoints>& wire_switchpoints);

/* parses the wirepoints specified in ch into the vector wire_points_vec */
static void parse_comma_separated_wire_points(const char* ch, std::vector<t_wire_switchpoints>& wire_switchpoints);

/* Parses the number of connections type */
static void parse_num_conns(std::string num_conns, t_wireconn_inf& wireconn);

/* checks for correctness of a unidir switchblock. */
static void check_unidir_switchblock(const t_switchblock_inf* sb);

/* checks for correctness of a bidir switchblock. */
static void check_bidir_switchblock(const t_permutation_map* permutation_map);

/* checks for correctness of a wireconn segment specification. */
static void check_wireconn(const t_arch* arch, const t_wireconn_inf& wireconn);

/**** Function Definitions ****/

/*---- Functions for Parsing Switchblocks from Architecture ----*/

/* Reads-in the wire connections specified for the switchblock in the xml arch file */
void read_sb_wireconns(const t_arch_switch_inf* /*switches*/, int /*num_switches*/, pugi::xml_node Node, t_switchblock_inf* sb, const pugiutil::loc_data& loc_data) {
    /* Make sure that Node is a switchblock */
    check_node(Node, "switchblock", loc_data);

    int num_wireconns;
    pugi::xml_node SubElem;

    /* count the number of specified wire connections for this SB */
    num_wireconns = count_children(Node, "wireconn", loc_data, OPTIONAL);
    sb->wireconns.reserve(num_wireconns);

    if (num_wireconns > 0) {
        SubElem = get_first_child(Node, "wireconn", loc_data);
    }
    for (int i = 0; i < num_wireconns; i++) {
        t_wireconn_inf wc = parse_wireconn(SubElem, loc_data);
        sb->wireconns.push_back(wc);
        SubElem = SubElem.next_sibling(SubElem.name());
    }

    return;
}

t_wireconn_inf parse_wireconn(pugi::xml_node node, const pugiutil::loc_data& loc_data) {
    t_wireconn_inf wc;

    size_t num_children = count_children(node, "from", loc_data, ReqOpt::OPTIONAL);
    num_children += count_children(node, "to", loc_data, ReqOpt::OPTIONAL);

    if (num_children == 0) {
        parse_wireconn_inline(node, loc_data, wc);
    } else {
        VTR_ASSERT(num_children > 0);
        parse_wireconn_multinode(node, loc_data, wc);
    }

    return wc;
}

void parse_wireconn_inline(pugi::xml_node node, const pugiutil::loc_data& loc_data, t_wireconn_inf& wc) {
    //Parse an inline wireconn definition, using attributes
    expect_only_attributes(node, {"num_conns", "from_type", "to_type", "from_switchpoint", "to_switchpoint", "from_order", "to_order"}, loc_data);

    /* get the connection style */
    const char* char_prop = get_attribute(node, "num_conns", loc_data).value();
    parse_num_conns(char_prop, wc);

    /* get from type */
    char_prop = get_attribute(node, "from_type", loc_data).value();
    parse_comma_separated_wire_types(char_prop, wc.from_switchpoint_set);

    /* get to type */
    char_prop = get_attribute(node, "to_type", loc_data).value();
    parse_comma_separated_wire_types(char_prop, wc.to_switchpoint_set);

    /* get the source wire point */
    char_prop = get_attribute(node, "from_switchpoint", loc_data).value();
    parse_comma_separated_wire_points(char_prop, wc.from_switchpoint_set);

    /* get the destination wire point */
    char_prop = get_attribute(node, "to_switchpoint", loc_data).value();
    parse_comma_separated_wire_points(char_prop, wc.to_switchpoint_set);

    char_prop = get_attribute(node, "from_order", loc_data, ReqOpt::OPTIONAL).value();
    parse_switchpoint_order(char_prop, wc.from_switchpoint_order);

    char_prop = get_attribute(node, "to_order", loc_data, ReqOpt::OPTIONAL).value();
    parse_switchpoint_order(char_prop, wc.to_switchpoint_order);
}

void parse_wireconn_multinode(pugi::xml_node node, const pugiutil::loc_data& loc_data, t_wireconn_inf& wc) {
    expect_only_children(node, {"from", "to"}, loc_data);

    /* get the connection style */
    const char* char_prop = get_attribute(node, "num_conns", loc_data).value();
    parse_num_conns(char_prop, wc);

    char_prop = get_attribute(node, "from_order", loc_data, ReqOpt::OPTIONAL).value();
    parse_switchpoint_order(char_prop, wc.from_switchpoint_order);

    char_prop = get_attribute(node, "to_order", loc_data, ReqOpt::OPTIONAL).value();
    parse_switchpoint_order(char_prop, wc.to_switchpoint_order);

    size_t num_from_children = count_children(node, "from", loc_data);
    size_t num_to_children = count_children(node, "to", loc_data);

    VTR_ASSERT(num_from_children > 0);
    VTR_ASSERT(num_to_children > 0);

    for (pugi::xml_node child : node.children()) {
        if (child.name() == std::string("from")) {
            t_wire_switchpoints from_switchpoints = parse_wireconn_from_to_node(child, loc_data);
            wc.from_switchpoint_set.push_back(from_switchpoints);
        } else if (child.name() == std::string("to")) {
            t_wire_switchpoints to_switchpoints = parse_wireconn_from_to_node(child, loc_data);
            wc.to_switchpoint_set.push_back(to_switchpoints);
        } else {
            archfpga_throw(loc_data.filename_c_str(), loc_data.line(node), "Unrecognized child node '%s' of parent node '%s'",
                           node.name(), child.name());
        }
    }
}

t_wire_switchpoints parse_wireconn_from_to_node(pugi::xml_node node, const pugiutil::loc_data& loc_data) {
    expect_only_attributes(node, {"type", "switchpoint"}, loc_data);

    size_t attribute_count = count_attributes(node, loc_data);

    if (attribute_count != 2) {
        archfpga_throw(loc_data.filename_c_str(), loc_data.line(node), "Expected only 2 attributes on node '%s'",
                       node.name());
    }

    t_wire_switchpoints wire_switchpoints;
    wire_switchpoints.segment_name = get_attribute(node, "type", loc_data).value();

    auto points_str = get_attribute(node, "switchpoint", loc_data).value();
    for (const auto& point_str : vtr::split(points_str, ",")) {
        int switchpoint = vtr::atoi(point_str);
        wire_switchpoints.switchpoints.push_back(switchpoint);
    }

    if (wire_switchpoints.switchpoints.empty()) {
        archfpga_throw(loc_data.filename_c_str(), loc_data.line(node), "Empty switchpoint specification",
                       node.name());
    }

    return wire_switchpoints;
}

static void parse_switchpoint_order(const char* order, SwitchPointOrder& switchpoint_order) {
    if (order == std::string("")) {
        switchpoint_order = SwitchPointOrder::SHUFFLED; //Default
    } else if (order == std::string("fixed")) {
        switchpoint_order = SwitchPointOrder::FIXED;
    } else if (order == std::string("shuffled")) {
        switchpoint_order = SwitchPointOrder::SHUFFLED;
    } else {
        archfpga_throw(__FILE__, __LINE__, "Unrecognized switchpoint order '%s'", order);
    }
}

/* parses the wire types specified in the comma-separated 'ch' char array into the vector wire_points_vec.
 * Spaces are trimmed off */
static void parse_comma_separated_wire_types(const char* ch, std::vector<t_wire_switchpoints>& wire_switchpoints) {
    auto types = vtr::split(ch, ",");

    if (types.empty()) {
        archfpga_throw(__FILE__, __LINE__, "parse_comma_separated_wire_types: found empty wireconn wire type entry\n");
    }

    for (const auto& type : types) {
        t_wire_switchpoints wsp;
        wsp.segment_name = type;

        wire_switchpoints.push_back(wsp);
    }
}

/* parses the wirepoints specified in the comma-separated 'ch' char array into the vector wire_points_vec */
static void parse_comma_separated_wire_points(const char* ch, std::vector<t_wire_switchpoints>& wire_switchpoints) {
    auto points = vtr::split(ch, ",");
    if (points.empty()) {
        archfpga_throw(__FILE__, __LINE__, "parse_comma_separated_wire_points: found empty wireconn wire point entry\n");
    }

    for (const auto& point_str : points) {
        int point = vtr::atoi(point_str);

        for (auto& wire_switchpoint : wire_switchpoints) {
            wire_switchpoint.switchpoints.push_back(point);
        }
    }
}

static void parse_num_conns(std::string num_conns, t_wireconn_inf& wireconn) {
    //num_conns is now interpretted as a formula and processed in build_switchblocks
    wireconn.num_conns_formula = num_conns;
}

/* Loads permutation funcs specified under Node into t_switchblock_inf. Node should be
 * <switchfuncs> */
void read_sb_switchfuncs(pugi::xml_node Node, t_switchblock_inf* sb, const pugiutil::loc_data& loc_data) {
    /* Make sure the passed-in is correct */
    check_node(Node, "switchfuncs", loc_data);

    pugi::xml_node SubElem;

    /* get the number of specified permutation functions */
    int num_funcs = count_children(Node, "func", loc_data, OPTIONAL);

    const char* func_type;
    const char* func_formula;
    vector<string>* func_ptr;

    /* used to index into permutation map of switchblock */
    SB_Side_Connection conn;

    /* now we iterate through all the specified permutation functions, and
     * load them into the switchblock structure as appropriate */
    if (num_funcs > 0) {
        SubElem = get_first_child(Node, "func", loc_data);
    }
    for (int ifunc = 0; ifunc < num_funcs; ifunc++) {
        /* get function type */
        func_type = get_attribute(SubElem, "type", loc_data).as_string(nullptr);

        /* get function formula */
        func_formula = get_attribute(SubElem, "formula", loc_data).as_string(nullptr);

        /* go through all the possible cases of func_type */
        if (0 == strcmp(func_type, "lt")) {
            conn.set_sides(LEFT, TOP);
        } else if (0 == strcmp(func_type, "lr")) {
            conn.set_sides(LEFT, RIGHT);
        } else if (0 == strcmp(func_type, "lb")) {
            conn.set_sides(LEFT, BOTTOM);
        } else if (0 == strcmp(func_type, "tl")) {
            conn.set_sides(TOP, LEFT);
        } else if (0 == strcmp(func_type, "tb")) {
            conn.set_sides(TOP, BOTTOM);
        } else if (0 == strcmp(func_type, "tr")) {
            conn.set_sides(TOP, RIGHT);
        } else if (0 == strcmp(func_type, "rt")) {
            conn.set_sides(RIGHT, TOP);
        } else if (0 == strcmp(func_type, "rl")) {
            conn.set_sides(RIGHT, LEFT);
        } else if (0 == strcmp(func_type, "rb")) {
            conn.set_sides(RIGHT, BOTTOM);
        } else if (0 == strcmp(func_type, "bl")) {
            conn.set_sides(BOTTOM, LEFT);
        } else if (0 == strcmp(func_type, "bt")) {
            conn.set_sides(BOTTOM, TOP);
        } else if (0 == strcmp(func_type, "br")) {
            conn.set_sides(BOTTOM, RIGHT);
        } else {
            /* unknown permutation function */
            archfpga_throw(__FILE__, __LINE__, "Unknown permutation function specified: %s\n", func_type);
        }
        func_ptr = &(sb->permutation_map[conn]);

        /* Here we load the specified switch function(s) */
        func_ptr->push_back(string(func_formula));

        func_ptr = nullptr;
        /* get the next switchblock function */
        SubElem = SubElem.next_sibling(SubElem.name());
    }

    return;
}

/* checks for correctness of switch block read-in from the XML architecture file */
void check_switchblock(const t_switchblock_inf* sb, const t_arch* arch) {
    /* get directionality */
    enum e_directionality directionality = sb->directionality;

    /* Check for errors in the switchblock descriptions */
    if (UNI_DIRECTIONAL == directionality) {
        check_unidir_switchblock(sb);
    } else {
        VTR_ASSERT(BI_DIRECTIONAL == directionality);
        check_bidir_switchblock(&(sb->permutation_map));
    }

    /* check that specified wires exist */
    for (const auto& wireconn : sb->wireconns) {
        check_wireconn(arch, wireconn);
    }

    //TODO:
    /* check that the wire segment directionality matches the specified switch block directionality */
    /* check for duplicate names */
    /* check that specified switches exist */
    /* check that type of switchblock matches type of switch specified */
}

/* checks for correctness of a unidirectional switchblock. hard exit if error found (to be changed to throw later) */
static void check_unidir_switchblock(const t_switchblock_inf* sb) {
    /* Check that the destination wire points are always the starting points (i.e. of wire point 0) */
    for (const t_wireconn_inf& wireconn : sb->wireconns) {
        for (const t_wire_switchpoints& wire_to_points : wireconn.to_switchpoint_set) {
            if (wire_to_points.switchpoints.size() > 1 || wire_to_points.switchpoints[0] != 0) {
                archfpga_throw(__FILE__, __LINE__, "Unidirectional switch blocks are currently only allowed to drive the start points of wire segments\n");
            }
        }
    }
}

/* checks for correctness of a bidirectional switchblock */
static void check_bidir_switchblock(const t_permutation_map* permutation_map) {
    /**** check that if side1->side2 is specified, then side2->side1 is not, as it is implicit ****/

    /* variable used to index into the permutation map */
    SB_Side_Connection conn;

    /* iterate over all combinations of from_side -> to side */
    for (e_side from_side : {TOP, RIGHT, BOTTOM, LEFT}) {
        for (e_side to_side : {TOP, RIGHT, BOTTOM, LEFT}) {
            /* can't connect a switchblock side to itself */
            if (from_side == to_side) {
                continue;
            }

            /* index into permutation map with this variable */
            conn.set_sides(from_side, to_side);

            /* check if a connection between these sides exists */
            t_permutation_map::const_iterator it = (*permutation_map).find(conn);
            if (it != (*permutation_map).end()) {
                /* the two sides are connected */
                /* check if the opposite connection has been specified */
                conn.set_sides(to_side, from_side);
                it = (*permutation_map).find(conn);
                if (it != (*permutation_map).end()) {
                    archfpga_throw(__FILE__, __LINE__, "If a bidirectional switch block specifies a connection from side1->side2, no connection should be specified from side2->side1 as it is implicit.\n");
                }
            }
        }
    }

    return;
}

static void check_wireconn(const t_arch* arch, const t_wireconn_inf& wireconn) {
    for (const t_wire_switchpoints& wire_switchpoints : wireconn.from_switchpoint_set) {
        auto seg_name = wire_switchpoints.segment_name;

        //Make sure the segment exists
        const t_segment_inf* seg_info = find_segment(arch, seg_name);
        if (!seg_info) {
            archfpga_throw(__FILE__, __LINE__, "Failed to find segment '%s' for <wireconn> from type specification\n", seg_name.c_str());
        }

        //Check that the specified switch points are valid
        for (int switchpoint : wire_switchpoints.switchpoints) {
            if (switchpoint < 0) {
                archfpga_throw(__FILE__, __LINE__, "Invalid <wireconn> from_switchpoint '%d' (must be >= 0)\n", switchpoint, seg_name.c_str());
            }
            if (switchpoint >= seg_info->length) {
                archfpga_throw(__FILE__, __LINE__, "Invalid <wireconn> from_switchpoints '%d' (must be < %d)\n", switchpoint, seg_info->length);
            }
            //TODO: check that points correspond to valid sb locations
        }
    }

    for (const t_wire_switchpoints& wire_switchpoints : wireconn.to_switchpoint_set) {
        auto seg_name = wire_switchpoints.segment_name;

        //Make sure the segment exists
        const t_segment_inf* seg_info = find_segment(arch, seg_name);
        if (!seg_info) {
            archfpga_throw(__FILE__, __LINE__, "Failed to find segment '%s' for <wireconn> to type specification\n", seg_name.c_str());
        }

        //Check that the specified switch points are valid
        for (int switchpoint : wire_switchpoints.switchpoints) {
            if (switchpoint < 0) {
                archfpga_throw(__FILE__, __LINE__, "Invalid <wireconn> to_switchpoint '%d' (must be >= 0)\n", switchpoint, seg_name.c_str());
            }
            if (switchpoint >= seg_info->length) {
                archfpga_throw(__FILE__, __LINE__, "Invalid <wireconn> to_switchpoints '%d' (must be < %d)\n", switchpoint, seg_info->length);
            }
            //TODO: check that points correspond to valid sb locations
        }
    }
}

/*---- Functions for Parsing the Symbolic Switchblock Formulas ----*/

/* returns integer result according to the specified switchblock formula and data. formula may be piece-wise */
int get_sb_formula_raw_result(const char* formula, const t_formula_data& mydata) {
    /* the result of the formula will be an integer */
    int result = -1;

    /* check formula */
    if (nullptr == formula) {
        archfpga_throw(__FILE__, __LINE__, "in get_sb_formula_result: SB formula pointer NULL\n");
    } else if ('\0' == formula[0]) {
        archfpga_throw(__FILE__, __LINE__, "in get_sb_formula_result: SB formula empty\n");
    }

    /* parse based on whether formula is piece-wise or not */
    if (is_piecewise_formula(formula)) {
        //EXPERIMENTAL
        result = parse_piecewise_formula(formula, mydata);
    } else {
        result = parse_formula(formula, mydata);
    }

    return result;
}
