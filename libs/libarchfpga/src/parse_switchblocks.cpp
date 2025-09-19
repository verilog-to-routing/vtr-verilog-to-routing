/*
 * See vpr/SRC/route/build_switchblocks.c for a detailed description of how the new
 * switch block format works and what files are involved.
 *
 *
 * A large chunk of this file is dedicated to helping parse the initial switchblock
 * specification in the XML arch file, providing error checking, etc.
 *
 * Another large chunk of this file is dedicated to parsing the actual formulas
 * specified by the switch block permutation functions into their numeric counterparts.
 */

#include <cstring>
#include <string>
#include <vector>

#include "vtr_assert.h"
#include "vtr_util.h"

#include "pugixml.hpp"
#include "pugixml_util.hpp"

#include "arch_error.h"

#include "arch_util.h"
#include "switchblock_types.h"
#include "parse_switchblocks.h"

using pugiutil::ReqOpt;

using vtr::FormulaParser;
using vtr::t_formula_data;

/**** Function Declarations ****/
/*---- Functions for Parsing Switchblocks from Architecture ----*/

//Process the desired order of a wireconn
static void parse_switchpoint_order(std::string_view order, e_switch_point_order& switchpoint_order);

/**
 * @brief Parses an inline `<wireconn>` node using its attributes.
 *
 * Extracts wire types, switch points, orders, and optional switch overrides
 * from the attributes of the inline `<wireconn>` XML node and returns a populated
 * `t_wireconn_inf` structure.
 *
 * @param node             XML node containing inline wireconn attributes.
 * @param loc_data         Location data for error reporting.
 * @param switches         List of architecture switch definitions (used for switch overrides).
 * @param can_skip_from_or_to Determines if the from or to attributes are optional or mandatory.
 * <wireconn> tags for switch blocks require both from and to attributes while those for
 * scatter-gather use one or the other.
 * @return                 A `t_wireconn_inf` structure populated with parsed data.
 */
static t_wireconn_inf parse_wireconn_inline(pugi::xml_node node,
                                            const pugiutil::loc_data& loc_data,
                                            const std::vector<t_arch_switch_inf>& switches,
                                            bool can_skip_from_or_to);

/**
 * @brief Parses a multi-node `<wireconn>` definition with `<from>` and `<to>` children.
 *
 * Processes wire connection endpoints described in child nodes and returns a `t_wireconn_inf`
 * structure populated with switchpoint sets and optional metadata like orders and switch overrides.
 *
 * @param node             XML node containing `<from>` and `<to>` children.
 * @param loc_data         Location data for error reporting.
 * @param switches         List of architecture switch definitions (used for switch overrides).
 * @return                 A `t_wireconn_inf` structure populated with parsed data.
 */
static t_wireconn_inf parse_wireconn_multinode(pugi::xml_node node,
                                               const pugiutil::loc_data& loc_data,
                                               const std::vector<t_arch_switch_inf>& switches);

//Process a <from> or <to> sub-node of a multinode wireconn
static t_wire_switchpoints parse_wireconn_from_to_node(pugi::xml_node node, const pugiutil::loc_data& loc_data);

/* parses the wire types specified in the comma-separated 'ch' char array into the vector wire_points_vec.
 * Spaces are trimmed off */
static void parse_comma_separated_wire_types(const char* ch, std::vector<t_wire_switchpoints>& wire_switchpoints);

/* parses the wirepoints specified in ch into the vector wire_points_vec */
static void parse_comma_separated_wire_points(const char* ch, std::vector<t_wire_switchpoints>& wire_switchpoints);

/* Parses the number of connections type */
static void parse_num_conns(std::string num_conns, t_wireconn_inf& wireconn);

/* Set connection from_side and to_side for custom switch block pattern*/
static void set_switch_func_type(SBSideConnection& conn, std::string_view func_type);

/* parse switch_override in wireconn */
static void parse_switch_override(const char* switch_override, t_wireconn_inf& wireconn, const std::vector<t_arch_switch_inf>& switches);

/* checks for correctness of a unidir switchblock. */
static void check_unidir_switchblock(const t_switchblock_inf& sb);

/* checks for correctness of a bidir switchblock. */
static void check_bidir_switchblock(const t_permutation_map* permutation_map);

/* checks for correctness of a wireconn segment specification. */
static void check_wireconn(const t_arch* arch, const t_wireconn_inf& wireconn);

/**** Function Definitions ****/

/*---- Functions for Parsing Switchblocks from Architecture ----*/

void read_sb_wireconns(const std::vector<t_arch_switch_inf>& switches,
                       pugi::xml_node Node,
                       t_switchblock_inf& sb,
                       const pugiutil::loc_data& loc_data) {
    // Make sure that Node is a switchblock
    check_node(Node, "switchblock", loc_data);

    pugi::xml_node SubElem;

    // Count the number of specified wire connections for this SB
    const int num_wireconns = count_children(Node, "wireconn", loc_data, ReqOpt::OPTIONAL);
    sb.wireconns.reserve(num_wireconns);

    if (num_wireconns > 0) {
        SubElem = get_first_child(Node, "wireconn", loc_data);
    }

    for (int i = 0; i < num_wireconns; i++) {
        t_wireconn_inf wc = parse_wireconn(SubElem, loc_data, switches); // need to pass in switch info for switch override
        sb.wireconns.push_back(wc);
        SubElem = SubElem.next_sibling(SubElem.name());
    }
}

t_wireconn_inf parse_wireconn(pugi::xml_node node,
                              const pugiutil::loc_data& loc_data,
                              const std::vector<t_arch_switch_inf>& switches,
                              bool can_skip_from_or_to) {

    size_t num_children = count_children(node, "from", loc_data, ReqOpt::OPTIONAL);
    num_children += count_children(node, "to", loc_data, ReqOpt::OPTIONAL);

    t_wireconn_inf wireconn;
    if (num_children == 0) {
        wireconn = parse_wireconn_inline(node, loc_data, switches, can_skip_from_or_to);
    } else {
        VTR_ASSERT(num_children > 0);
        wireconn = parse_wireconn_multinode(node, loc_data, switches);
    }

    // Parse the optional "side" field of the <wireconn> tag
    std::string sides_string = get_attribute(node, "side", loc_data, pugiutil::OPTIONAL).as_string();

    if (sides_string.find_first_not_of("rtlbRTLB") != std::string::npos) {
        archfpga_throw(loc_data.filename_c_str(), loc_data.line(node), "Unknown side specified: %s\n", sides_string.c_str());
    }
    for (char side_char : sides_string) {
        wireconn.sides.insert(CHAR_SIDE_MAP.at(side_char));
    }

    return wireconn;
}

static t_wireconn_inf parse_wireconn_inline(pugi::xml_node node,
                                            const pugiutil::loc_data& loc_data,
                                            const std::vector<t_arch_switch_inf>& switches,
                                            bool can_skip_from_or_to) {

    // Parse an inline wireconn definition, using attributes
    expect_only_attributes(node,
                           {"num_conns", "from_type", "to_type", "from_switchpoint",
                            "to_switchpoint", "from_order", "to_order", "switch_override", "side"},
                           loc_data);

    t_wireconn_inf wc;

    ReqOpt from_to_required = can_skip_from_or_to ? ReqOpt::OPTIONAL : ReqOpt::REQUIRED;

    // get the connection style
    const char* char_prop = get_attribute(node, "num_conns", loc_data).value();
    parse_num_conns(char_prop, wc);

    // get from type
    char_prop = get_attribute(node, "from_type", loc_data, from_to_required).as_string();
    if (*char_prop) { // if from_to_required is ReqOpt::REQUIRED, char_prop is definitely not null. Otherwise, it's optional and should be null checked.
        parse_comma_separated_wire_types(char_prop, wc.from_switchpoint_set);
    }

    // get to type
    char_prop = get_attribute(node, "to_type", loc_data, from_to_required).as_string();
    if (*char_prop) {
        parse_comma_separated_wire_types(char_prop, wc.to_switchpoint_set);
    }

    // get the source wire point
    char_prop = get_attribute(node, "from_switchpoint", loc_data, from_to_required).as_string();
    if (*char_prop) {
        parse_comma_separated_wire_points(char_prop, wc.from_switchpoint_set);
    }

    // get the destination wire point
    char_prop = get_attribute(node, "to_switchpoint", loc_data, from_to_required).as_string();
    if (*char_prop) {
        parse_comma_separated_wire_points(char_prop, wc.to_switchpoint_set);
    }

    char_prop = get_attribute(node, "from_order", loc_data, ReqOpt::OPTIONAL).value();
    parse_switchpoint_order(char_prop, wc.from_switchpoint_order);

    char_prop = get_attribute(node, "to_order", loc_data, ReqOpt::OPTIONAL).value();
    parse_switchpoint_order(char_prop, wc.to_switchpoint_order);

    // parse switch overrides if they exist:
    char_prop = get_attribute(node, "switch_override", loc_data, ReqOpt::OPTIONAL).value();
    parse_switch_override(char_prop, wc, switches);

    return wc;
}

static t_wireconn_inf parse_wireconn_multinode(pugi::xml_node node,
                                               const pugiutil::loc_data& loc_data,
                                               const std::vector<t_arch_switch_inf>& switches) {
    expect_only_children(node, {"from", "to"}, loc_data);

    t_wireconn_inf wc;

    // get the connection style
    const char* char_prop = get_attribute(node, "num_conns", loc_data).value();
    parse_num_conns(char_prop, wc);

    char_prop = get_attribute(node, "from_order", loc_data, ReqOpt::OPTIONAL).value();
    parse_switchpoint_order(char_prop, wc.from_switchpoint_order);

    char_prop = get_attribute(node, "to_order", loc_data, ReqOpt::OPTIONAL).value();
    parse_switchpoint_order(char_prop, wc.to_switchpoint_order);

    char_prop = get_attribute(node, "switch_override", loc_data, ReqOpt::OPTIONAL).value();
    parse_switch_override(char_prop, wc, switches);

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

    return wc;
}

static t_wire_switchpoints parse_wireconn_from_to_node(pugi::xml_node node, const pugiutil::loc_data& loc_data) {
    expect_only_attributes(node, {"type", "switchpoint"}, loc_data);

    size_t attribute_count = count_attributes(node, loc_data);

    if (attribute_count != 2) {
        archfpga_throw(loc_data.filename_c_str(), loc_data.line(node), "Expected only 2 attributes on node '%s'",
                       node.name());
    }

    t_wire_switchpoints wire_switchpoints;
    wire_switchpoints.segment_name = get_attribute(node, "type", loc_data).value();

    std::string points_str = get_attribute(node, "switchpoint", loc_data).value();
    for (const std::string& point_str : vtr::StringToken(points_str).split(",")) {
        int switchpoint = vtr::atoi(point_str);
        wire_switchpoints.switchpoints.push_back(switchpoint);
    }

    if (wire_switchpoints.switchpoints.empty()) {
        archfpga_throw(loc_data.filename_c_str(), loc_data.line(node), "Empty switchpoint specification",
                       node.name());
    }

    return wire_switchpoints;
}

static void parse_switchpoint_order(std::string_view order, e_switch_point_order& switchpoint_order) {
    if (order == "") {
        switchpoint_order = e_switch_point_order::SHUFFLED; //Default
    } else if (order == "fixed") {
        switchpoint_order = e_switch_point_order::FIXED;
    } else if (order == "shuffled") {
        switchpoint_order = e_switch_point_order::SHUFFLED;
    } else {
        archfpga_throw(__FILE__, __LINE__, "Unrecognized switchpoint order '%s'", order);
    }
}

/* parses the wire types specified in the comma-separated 'ch' char array into the vector wire_points_vec.
 * Spaces are trimmed off */
static void parse_comma_separated_wire_types(const char* ch, std::vector<t_wire_switchpoints>& wire_switchpoints) {
    std::vector<std::string> types = vtr::StringToken(ch).split(",");

    if (types.empty()) {
        archfpga_throw(__FILE__, __LINE__, "parse_comma_separated_wire_types: found empty wireconn wire type entry\n");
    }

    for (const std::string& type : types) {
        t_wire_switchpoints wsp;
        wsp.segment_name = type;

        wire_switchpoints.push_back(wsp);
    }
}

/* parses the wirepoints specified in the comma-separated 'ch' char array into the vector wire_points_vec */
static void parse_comma_separated_wire_points(const char* ch, std::vector<t_wire_switchpoints>& wire_switchpoints) {
    std::vector<std::string> points = vtr::StringToken(ch).split(",");
    if (points.empty()) {
        archfpga_throw(__FILE__, __LINE__, "parse_comma_separated_wire_points: found empty wireconn wire point entry\n");
    }

    for (const std::string& point_str : points) {
        int point = vtr::atoi(point_str);

        for (t_wire_switchpoints& wire_switchpoint : wire_switchpoints) {
            wire_switchpoint.switchpoints.push_back(point);
        }
    }
}

static void parse_num_conns(std::string num_conns, t_wireconn_inf& wireconn) {
    // num_conns is now interpreted as a formula and processed in build_switchblocks
    wireconn.num_conns_formula = num_conns;
}

static void set_switch_func_type(SBSideConnection& conn, std::string_view func_type) {

    if (func_type.length() != 2) {
        archfpga_throw(__FILE__, __LINE__, "Custom switchblock func type must be 2 characters long: %s\n", func_type);
    }

    // Only valid sides are right, top, left, bottom
    if (func_type.find_first_not_of("rtlbRTLB") != std::string::npos) {
        archfpga_throw(__FILE__, __LINE__, "Unknown direction specified: %s\n", func_type);
    }

    e_side from_side = CHAR_SIDE_MAP.at(func_type[0]);
    e_side to_side = CHAR_SIDE_MAP.at(func_type[1]);

    // Can't go from side to same side
    if (to_side == from_side) {
        archfpga_throw(__FILE__, __LINE__, "Unknown permutation function specified, cannot go from side to same side: %s\n", func_type);
    }

    conn.set_sides(from_side, to_side);
}

void read_sb_switchfuncs(pugi::xml_node node, t_switchblock_inf& sb, const pugiutil::loc_data& loc_data) {
    // Make sure the passed-in node is correct
    check_node(node, "switchfuncs", loc_data);

    pugi::xml_node SubElem;

    // Get the number of specified permutation functions
    const int num_funcs = count_children(node, "func", loc_data, ReqOpt::OPTIONAL);

    // now we iterate through all the specified permutation functions, and
    // load them into the switchblock structure as appropriate
    if (num_funcs > 0) {
        SubElem = get_first_child(node, "func", loc_data);
    }

    for (int ifunc = 0; ifunc < num_funcs; ifunc++) {
        // Get function type
        const char* func_type = get_attribute(SubElem, "type", loc_data).as_string(nullptr);

        // Get function formula
        const char* func_formula = get_attribute(SubElem, "formula", loc_data).as_string(nullptr);

        // Used to index into permutation map of switchblock
        SBSideConnection conn;

        // go through all the possible cases of func_type
        set_switch_func_type(conn, func_type);

        // Here we load the specified switch function(s)
        sb.permutation_map[conn].emplace_back(func_formula);

        // get the next switchblock function
        SubElem = SubElem.next_sibling(SubElem.name());
    }
}

static void parse_switch_override(const char* switch_override, t_wireconn_inf& wireconn, const std::vector<t_arch_switch_inf>& switches) {
    // sentinel value to use default driving switch for the receiving wire type
    if (switch_override == std::string("")) {
        wireconn.switch_override_indx = DEFAULT_SWITCH; //Default
        return;
    }

    // iterate through the valid switch names in the arch looking for the requested switch_override
    for (int i = 0; i < (int)switches.size(); i++) {
        if (0 == strcmp(switch_override, switches[i].name.c_str())) {
            wireconn.switch_override_indx = i;
            return;
        }
    }
    // if we haven't found a switch that matched, then throw an error
    archfpga_throw(__FILE__, __LINE__, "Unknown switch_override specified in wireconn of custom switch blocks: \"%s\"\n", switch_override);
}

/* checks for correctness of switch block read-in from the XML architecture file */
void check_switchblock(const t_switchblock_inf& sb, const t_arch* arch) {
    /* get directionality */
    enum e_directionality directionality = sb.directionality;

    /* Check for errors in the switchblock descriptions */
    if (UNI_DIRECTIONAL == directionality) {
        check_unidir_switchblock(sb);
    } else {
        VTR_ASSERT(BI_DIRECTIONAL == directionality);
        check_bidir_switchblock(&(sb.permutation_map));
    }

    /* check that specified wires exist */
    for (const t_wireconn_inf& wireconn : sb.wireconns) {
        check_wireconn(arch, wireconn);
    }

    //TODO:
    /* check that the wire segment directionality matches the specified switch block directionality */
    /* check for duplicate names */
    /* check that specified switches exist */
    /* check that type of switchblock matches type of switch specified */
}

/* checks for correctness of a unidirectional switchblock. hard exit if error found (to be changed to throw later) */
static void check_unidir_switchblock(const t_switchblock_inf& sb) {
    /* Check that the destination wire points are always the starting points (i.e. of wire point 0) */
    for (const t_wireconn_inf& wireconn : sb.wireconns) {
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
    SBSideConnection conn;

    /* iterate over all combinations of from_side -> to side */
    for (e_side from_side : TOTAL_2D_SIDES) {
        for (e_side to_side : TOTAL_2D_SIDES) {
            /* can't connect a switchblock side to itself */
            if (from_side == to_side) {
                continue;
            }

            /* index into permutation map with this variable */
            conn.set_sides(from_side, to_side);

            /* check if a connection between these sides exists */
            auto it = (*permutation_map).find(conn);
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
}

static void check_wireconn(const t_arch* arch, const t_wireconn_inf& wireconn) {
    for (const t_wire_switchpoints& wire_switchpoints : wireconn.from_switchpoint_set) {
        std::string seg_name = wire_switchpoints.segment_name;

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
        std::string seg_name = wire_switchpoints.segment_name;

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
int get_sb_formula_raw_result(FormulaParser& formula_parser, const char* formula, const t_formula_data& mydata) {
    /* the result of the formula will be an integer */
    int result = -1;

    /* check formula */
    if (nullptr == formula) {
        archfpga_throw(__FILE__, __LINE__, "in get_sb_formula_result: SB formula pointer NULL\n");
    } else if ('\0' == formula[0]) {
        archfpga_throw(__FILE__, __LINE__, "in get_sb_formula_result: SB formula empty\n");
    }

    /* parse based on whether formula is piece-wise or not */
    if (formula_parser.is_piecewise_formula(formula)) {
        //EXPERIMENTAL
        result = formula_parser.parse_piecewise_formula(formula, mydata);
    } else {
        result = formula_parser.parse_formula(formula, mydata);
    }

    return result;
}
