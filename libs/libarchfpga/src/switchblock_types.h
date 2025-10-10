#pragma once

#include <array>
#include <string>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

constexpr int NO_SWITCH = -1;
constexpr int DEFAULT_SWITCH = -2;

enum e_directionality {
    UNI_DIRECTIONAL,
    BI_DIRECTIONAL
};

/// Defines the type of switch block used in FPGA routing.
enum class e_switch_block_type {
    /// If the type is SUBSET, I use a Xilinx-like switch block where track i in one channel always
    /// connects to track i in other channels.
    SUBSET,

    /// If type is WILTON, I use a switch block where track i
    /// does not always connect to track i in other channels.
    /// See Steve Wilton, PhD Thesis, University of Toronto, 1996.
    WILTON,

    /// The UNIVERSAL switch block is from Y. W. Chang et al, TODAES, Jan. 1996, pp. 80 - 101.
    UNIVERSAL,

    /// The FULL switch block type allows for complete connectivity between tracks.
    FULL,

    /// A CUSTOM switch block has also been added which allows a user to describe custom permutation functions and connection patterns.
    /// See comment at top of SRC/route/build_switchblocks.c
    CUSTOM
};

/**
 * @brief At the intersection of routing channels, left, right, top and bottom specify the x- and y-directed channels
 * while above and under specify the switch block wires one a layer above or below the current one. above and below
 * are only used for multi-layer FPGAs. Note that the order of 2D sides is important, as it corresponds to the bit 
 * order in t_rr_node_data::dir_side_.sides.
 */
enum e_side : unsigned char {
    TOP = 0,
    RIGHT = 1,
    BOTTOM = 2,
    LEFT = 3,
    NUM_2D_SIDES = 4
};

inline const std::unordered_map<char, e_side> CHAR_SIDE_MAP = {
    {'T', TOP},
    {'t', TOP},
    {'R', RIGHT},
    {'r', RIGHT},
    {'B', BOTTOM},
    {'b', BOTTOM},
    {'L', LEFT},
    {'l', LEFT}};

constexpr std::array<e_side, NUM_2D_SIDES> TOTAL_2D_SIDES = {{TOP, RIGHT, BOTTOM, LEFT}};                     // Set of all side orientations
constexpr std::array<const char*, NUM_2D_SIDES> TOTAL_2D_SIDE_STRINGS = {{"TOP", "RIGHT", "BOTTOM", "LEFT"}}; // String versions of side orientations

/// @brief Specifies what part of the FPGA a custom switchblock should be built in (i.e. perimeter, core, everywhere)
enum class e_sb_location {
    E_PERIMETER = 0,
    E_CORNER,
    E_FRINGE, /* perimeter minus corners */
    E_CORE,
    E_EVERYWHERE,
    E_XY_SPECIFIED
};

inline const std::unordered_map<std::string, e_sb_location> SB_LOCATION_STRING_MAP = {{"EVERYWHERE", e_sb_location::E_EVERYWHERE},
                                                                                      {"PERIMETER", e_sb_location::E_PERIMETER},
                                                                                      {"CORE", e_sb_location::E_CORE},
                                                                                      {"CORNER", e_sb_location::E_CORNER},
                                                                                      {"FRINGE", e_sb_location::E_FRINGE},
                                                                                      {"XY_SPECIFIED", e_sb_location::E_XY_SPECIFIED}};

/**
 * @brief Describes regions that a specific switch block specifications should be applied to
 */
struct t_sb_loc_spec {
    int start = -1;
    int repeat = -1;
    int incr = -1;
    int end = -1;
};

/**
 * @brief represents a connection between two sides of a switchblock
 */
class SBSideConnection {
  public:
    // Specify the two SB sides that form a connection
    enum e_side from_side = TOP;
    enum e_side to_side = TOP;

    void set_sides(enum e_side from, enum e_side to) {
        from_side = from;
        to_side = to;
    }

    SBSideConnection() = default;

    SBSideConnection(enum e_side from, enum e_side to)
        : from_side(from)
        , to_side(to) {
    }

    // Overload < operator which will be used by std::map
    bool operator<(const SBSideConnection& obj) const {
        bool result;

        if (from_side < obj.from_side) {
            result = true;
        } else {
            if (from_side == obj.from_side) {
                result = (to_side < obj.to_side) ? true : false;
            } else {
                result = false;
            }
        }

        return result;
    }
};

enum class e_switch_point_order {
    FIXED,   ///< Switchpoints are ordered as specified in architecture
    SHUFFLED ///< Switchpoints are shuffled (more diversity)
};

/**
 * @brief A collection of switchpoints associated with a segment
 */
struct t_wire_switchpoints {
    std::string segment_name;      ///< The type of segment
    std::vector<int> switchpoints; ///< The indices of wire points along the segment
};

/**
 * @brief Used to list information about a set of track segments that should connect through a switchblock
 */
struct t_wireconn_inf {
    std::vector<t_wire_switchpoints> from_switchpoint_set;                     ///< The set of segment/wirepoints representing the 'from' set (union of all t_wire_switchpoints in vector)
    std::vector<t_wire_switchpoints> to_switchpoint_set;                       ///< The set of segment/wirepoints representing the 'to' set (union of all t_wire_switchpoints in vector)
    e_switch_point_order from_switchpoint_order = e_switch_point_order::FIXED; ///< The desired from_switchpoint_set ordering
    e_switch_point_order to_switchpoint_order = e_switch_point_order::FIXED;   ///< The desired to_switchpoint_set ordering
    int switch_override_indx = DEFAULT_SWITCH;                                 ///< index in switch array of the switch used to override wire_switch of the 'to' set.
                                                                               ///< DEFAULT_SWITCH is a sentinel value (i.e. the usual driving switch from a wire for the receiving wire will be used)

    std::string num_conns_formula; /* Specifies how many connections should be made for this wireconn.
                                    *
                                    * '<int>': A specific number of connections
                                    * 'from':  The number of generated connections between the 'from' and 'to' sets equals the
                                    *          size of the 'from' set. This ensures every element in the from set is connected
                                    *          to an element of the 'to' set.
                                    *          Note: this it may result in 'to' elements being driven by multiple 'from'
                                    *          elements (if 'from' is larger than 'to'), or in some elements of 'to' having
                                    *          no driving connections (if 'to' is larger than 'from').
                                    * 'to':    The number of generated connections is set equal to the size of the 'to' set.
                                    *          This ensures that each element of the 'to' set has precisely one incoming connection.
                                    *          Note: this may result in 'from' elements driving multiple 'to' elements (if 'to' is
                                    *          larger than 'from'), or some 'from' elements driving to 'to' elements (if 'from' is
                                    *          larger than 'to')
                                    */

    std::unordered_set<e_side> sides; ///< Used for scatter-gather wireconns determining which sides to gather from / scatter to, ignored in other usages.
};

/* Use a map to index into the string permutation functions used to connect from one side to another */
typedef std::map<SBSideConnection, std::vector<std::string>> t_permutation_map;

struct t_specified_loc {
    int x = -1; ///< The exact x-axis location that this SB is used, meaningful when type is set to E_XY_specified
    int y = -1; ///< The exact y-axis location that this SB is used, meaningful when type is set to E_XY_specified

    // We can also define a region to apply this SB to all locations falls into this region using regular expression in the architecture file
    t_sb_loc_spec reg_x;
    t_sb_loc_spec reg_y;
};

/**
 * @brief Lists all information about a particular switch block specified in the architecture file
 */
struct t_switchblock_inf {
    std::string name;                ///< the name of this switchblock
    e_sb_location location;          ///< where on the FPGA this switchblock should be built (i.e. perimeter, core, everywhere)
    e_directionality directionality; ///< the directionality of this switchblock (unidir/bidir)

    t_permutation_map permutation_map; ///< map holding the permutation functions attributed to this switchblock

    t_specified_loc specified_loc;

    std::vector<t_wireconn_inf> wireconns; ///< list of wire types/groups this SB will connect
};
