#include <kj/std/iostream.h>
#include <zlib.h>
#include <algorithm>
#include <map>
#include <queue>

#include "rr_graph_fpga_interchange.h"

#include "arch_util.h"
#include "vtr_time.h"
#include "vtr_error.h"
#include "globals.h"
#include "check_rr_graph.h"
#include "rr_metadata.h"
#include "rr_graph_indexed_data.h"
#include "fpga_interchange_arch_utils.h"

//#define DEBUG

using namespace DeviceResources;
using namespace LogicalNetlist;
using namespace capnp;

// Helper types definitions

// t_location in the grid: <x, y>
typedef std::pair<int, int> t_location;

// edge between two wires: <switch_id, tile_id, <name, wire0, wire1, forward>>
typedef std::tuple<int, int, std::tuple<int, int, int, bool>> t_pip;

// Helper enumerations

enum constant {
    GND = 0,
    VCC,
};

enum node_sides {
    LEFT_EDGE = 0,
    TOP_EDGE,
    RIGHT_EDGE,
    BOTTOM_EDGE,
};

const std::vector<node_sides> NODE_SIDES = {LEFT_EDGE, TOP_EDGE, RIGHT_EDGE, BOTTOM_EDGE};
const std::vector<node_sides> OPPOSITE_NODE_SIDES = {RIGHT_EDGE, BOTTOM_EDGE, LEFT_EDGE, TOP_EDGE};

/** @brief Intermediate data type used to build and explore FPGA Interchange
 *         nodes graph and later convert them to VTR RR Nodes.
 */
struct intermediate_node {
  public:
    t_location loc;
    int segment_id = -1;
    std::vector<bool> links{false, false, false, false}; // left, up, right, down
    intermediate_node* visited = nullptr;
    bool back_bone = false; // Indicates whether the node is in the connecting line between two ends
    bool on_route = false;
    bool has_pins = false;

    intermediate_node() = delete;

    intermediate_node(int x, int y)
        : loc{x, y} {}

    intermediate_node(t_location loc_)
        : loc(loc_) {}
};

/** @brief Interchange RR Graph builder class
 *
 *  This class contains methods and attributes to translate from the Interchange
 *  RR graph collection of nodes and wires to the VTR RR Graph representation.
 *
 *  First it builds an intermediate representation, which is necessary to store all the
 *  information in a convenient way to use the VTR RR Graph builder API.
 */
struct InterchangeRRGraphBuilder {
  public:
    InterchangeRRGraphBuilder(Device::Reader& arch_reader,
                              const DeviceGrid& grid,
                              DeviceContext& device_ctx,
                              t_rr_graph_storage& rr_nodes,
                              const vtr::vector<RRSegmentId, t_segment_inf>& segment_inf,
                              const std::vector<t_physical_tile_type>& physical_tile_types,
                              const enum e_base_cost_type base_cost_type)
        : ar_(arch_reader)
        , device_ctx_(device_ctx)
        , rr_nodes_(rr_nodes)
        , segment_inf_(segment_inf)
        , grid_(grid)
        , base_cost_type_(base_cost_type) {
        for (auto& type : physical_tile_types) {
            tile_type_to_pb_type_[std::string(type.name)] = &type;
        }

        auto wire_types = ar_.getWireTypes();
        for (auto wire : ar_.getWires())
            if (wire_types[wire.getType()].getCategory() != Device::WireCategory::GENERAL)
                wire_name_to_seg_idx_[str(wire.getWire())] = 0;

        // segment at index 0 is the generic one
        for (size_t i = 1; i < segment_inf.size(); ++i)
            wire_name_to_seg_idx_[segment_inf_[RRSegmentId(i)].name] = i;

        std::unordered_map<uint32_t, std::set<t_bel_cell_mapping>> temp_;
        auto func = std::bind(&InterchangeRRGraphBuilder::str, this, std::placeholders::_1);
        process_cell_bel_mappings(ar_, temp_, func);

        for (auto i : temp_)
            bel_cell_mappings_.insert(i.first);
    }

    void build_rr_graph() {
        create_switch_inf();
        create_tile_id_to_loc();
        create_uniq_node_ids();
        create_sink_source_nodes();
        create_pips();
        process_nodes();
#ifdef DEBUG
        print_virtual_rr_graph();
#endif
        virtual_to_real();
        pack_to_rr_graph();
#ifdef DEBUG
        print_real_rr_graph();
#endif
        finish_rr_generation();
    }

  private:
    Device::Reader& ar_;
    DeviceContext& device_ctx_;
    t_rr_graph_storage& rr_nodes_;
    const vtr::vector<RRSegmentId, t_segment_inf>& segment_inf_;
    const DeviceGrid& grid_;
    const enum e_base_cost_type base_cost_type_;

    std::unordered_map<std::string, const t_physical_tile_type*> tile_type_to_pb_type_;

    std::unordered_map<std::string, int> wire_name_to_seg_idx_;

    std::set<uint32_t> bel_cell_mappings_;

    // Tile location maps
    vtr::bimap<int /*tile_id*/, t_location> tile_loc_bimap_;

    // PIP maps
    std::unordered_map<std::tuple<int /*timing_model*/, bool /*buffered*/>, int /*idx*/, hash_tuple::hash<std::tuple<int, bool>>> pips_models_;
    std::unordered_map<std::tuple<int /*node_id*/, int /*node_id*/>, t_pip, hash_tuple::hash<std::tuple<int, int>>> pips_;

    // Node maps
    std::unordered_map<std::tuple<int, int>, int, hash_tuple::hash<std::tuple<int, int>>> wire_to_node_;
    std::unordered_map<int, std::set<t_location>> node_to_locs_;
    std::unordered_map<std::tuple<int, int> /*<node, tile_id>*/, int /*segment type*/, hash_tuple::hash<std::tuple<int, int>>> node_tile_to_segment_;
    std::unordered_map<int, std::pair<float, float>> node_to_RC_;
    int total_node_count_;

    /*
     * Offsets for FPGA Interchange node processing
     */
    t_location offsets[4] = {t_location(-1, 0), t_location(0, 1), t_location(1, 0), t_location(0, -1)};

    /*
     * Intermediate data storage.
     * - virtual_shorts_ represents connections between rr_nodes in a single FPGA Interchange node.
     * - virtual_redirect_ is map from node_id at t_location to t_location channel and index of virtual_track in that channel.
     * It's useful for ends of the nodes (FPGA Interchange) that don't have representation in CHANS.
     * - virtual_chan_loc_map maps from t_location and CHAN to vector containing virtual_tracks description.
     * - virtual_beg_to_real_ maps from virtual track to physical one
     * - chan_loc_map_ maps from t_location and CHAN to vector containing tracks description.
     * - node_id_count_ maps from node_id to its tile count
     */
    std::vector<std::tuple<t_location, e_rr_type, int /*idx*/, t_location, e_rr_type, int /*idx*/>> virtual_shorts_;
    std::unordered_map<std::tuple<int /*node_id*/, t_location>,
                       std::tuple<t_location, e_rr_type /*CHANX/CHANY*/, int /*idx*/>,
                       hash_tuple::hash<std::tuple<int, t_location>>>
        virtual_redirect_;

    std::unordered_map<std::tuple<t_location, e_rr_type, int>,
                       int,
                       hash_tuple::hash<std::tuple<t_location, e_rr_type, int>>>
        virtual_beg_to_real_;

    std::unordered_map<std::tuple<t_location, e_rr_type>,
                       std::vector<std::tuple<int, float, float, t_location>>,
                       hash_tuple::hash<std::tuple<t_location, e_rr_type>>>
        virtual_chan_loc_map_;

    std::unordered_map<std::tuple<t_location, e_rr_type>,
                       std::unordered_map<int, std::tuple<int, float, float, t_location>>,
                       hash_tuple::hash<std::tuple<t_location, e_rr_type>>>
        chan_loc_map_;

    std::unordered_map<int, int> node_id_count_;

    /*
     * Sets contain tuples of node ids and t_location.
     * Each value <n,l> correspondence to node id n being used by either pip or pin at t_location l.
     */
    std::unordered_set<std::tuple<int /*node_id*/, t_location>,
                       hash_tuple::hash<std::tuple<int, t_location>>>
        used_by_pip_;

    std::unordered_set<std::tuple<int /*node_id*/, t_location>,
                       hash_tuple::hash<std::tuple<int, t_location>>>
        used_by_pin_;

    std::unordered_set<int /* node_id*/> useful_node_;

    /* Sink_source_loc_map is used to create ink/source and ipin/opin rr_nodes,
     * rr_edges from sink/source to ipin/opin and from ipin/opin to their coresponding segments
     * uniq_id is used to create edge from segment to ipin/opin, edges from ipin/opin to sink/source are created simply as one to one
     */
    std::unordered_map<int /*tile_id*/, std::vector<std::tuple<bool, float, int>> /*idx = ptc,<type, R/C, node_id>*/> sink_source_loc_map_;

    /*
     * RR generation stuff
     */
    vtr::vector<RRIndexedDataId, short> seg_index_;
    std::unordered_map<std::tuple<t_location, e_rr_type, int>,
                       int,
                       hash_tuple::hash<std::tuple<t_location, e_rr_type, int>>>
        loc_type_idx_to_rr_idx_;
    int rr_idx = 0; // Do not decrement!

    /// @brief Returns the sum of a location and a vector, resulting in a new location
    t_location add_vec(t_location x, t_location dx) {
        return t_location(x.first + dx.first, x.second + dx.second);
    }

    /// @brief Returns the multiplication of a location and a scalar
    t_location mul_vec_scal(t_location x, int s) {
        return t_location(x.first * s, x.second * s);
    }

    /// @brief Returns the string corresponding to an entry in the string list of the interchange database
    std::string str(int idx) {
        if (idx == -1)
            return std::string("constant_block");
        return std::string(ar_.getStrList()[idx].cStr());
    }

    /*
     * The following code iterates over each node independently when transforming
     * an FPGA Interchange device resource into a VTR RR Graph.
     *
     * During the transformation, segments are created, which require
     * starting and ending coordinates, as well as their track location.
     * At any given time only one segment can occupy a single physical track.
     * Solving the track assignment with an online algorithm would greatly impact
     * on runtime, achieving a suboptimal maximal number of tracks in a single channel.
     *
     * This is solved by only storing a starting and ending location and
     * the track number in the starting position, thus it is named "virtual".
     * Then, when all FPGA Interchange nodes are taken care of,
     * the physical track placement is computed.
     *
     * Much of the code runs on virtual assignment, as it's fast to create,
     * use and is only translated when creating the final RR Graph.
     */

    /** @brief Debug print function to output the virtual RR graph information
     */
    void print_virtual_rr_graph() {
        VTR_LOG("Virtual\n");
        for (const auto& entry : sink_source_loc_map_) {
            const auto& key = entry.first;
            const auto& value = entry.second;
            int x, y;
            std::tie(x, y) = tile_loc_bimap_[key];
            VTR_LOG("t_location x:%d y:%d Name:%s\n", x, y, str(key).c_str());
            int it = 0;
            for (const auto& pin : value) {
                bool dir;
                float RC;
                int node_id;
                std::tie(dir, RC, node_id) = pin;
                VTR_LOG("\tpin:%d input:%d R/C:%e\n", it, dir, RC);
                VTR_LOG("\ttile_name:%s node_id:%d\n", str(key).c_str(), node_id);
                it++;
            }
        }

        int it = 0;

        for (auto const& switch_id : device_ctx_.rr_graph.rr_switch()) {
            VTR_LOG("Switch: %d Name:%s\n", it++, switch_id.name);
        }

        for (auto& entry : virtual_chan_loc_map_) {
            t_location loc, loc2;
            e_rr_type type;
            std::tie(loc, type) = entry.first;
            VTR_LOG("CHAN%c X:%d Y:%d\n", type == e_rr_type::CHANX ? 'X' : 'Y', loc.first, loc.second);
            for (auto& seg : entry.second) {
                loc2 = std::get<3>(seg);
                VTR_LOG("\tSegment id:%d name:%s -> X:%d Y:%d\n", std::get<0>(seg), segment_inf_[RRSegmentId(std::get<0>(seg))].name.c_str(), loc2.first, loc2.second);
            }
        }

        VTR_LOG("Redirects:\n");
        for (auto& entry : virtual_redirect_) {
            int node;
            t_location loc;
            std::tie(node, loc) = entry.first;

            t_location new_loc;
            e_rr_type new_type;
            int new_idx;
            std::tie(new_loc, new_type, new_idx) = entry.second;
            VTR_LOG("\t Node:%d X:%d Y:%d -> X:%d Y:%d CHAN%c idx:%d\n", node, loc.first, loc.second, new_loc.first,
                    new_loc.second, new_type == e_rr_type::CHANX ? 'X' : 'Y', new_idx);
        }

        VTR_LOG("Shorts:\n");
        for (auto& entry : virtual_shorts_) {
            t_location loc1, loc2;
            e_rr_type type1, type2;
            int id1, id2;
            std::tie(loc1, type1, id1, loc2, type2, id2) = entry;
            VTR_LOG("\tCHAN%c X:%d Y%d", type1 == e_rr_type::CHANX ? 'X' : 'Y', loc1.first, loc1.second);
            VTR_LOG(" Segment id:%d name:%s ->", id1,
                    segment_inf_[RRSegmentId(std::get<0>(virtual_chan_loc_map_[std::make_tuple(loc1, type1)][id1]))].name.c_str());
            VTR_LOG(" CHAN%c X:%d Y:%d", type2 == e_rr_type::CHANX ? 'X' : 'Y', loc2.first, loc2.second);
            VTR_LOG(" Segment id:%d name:%s\n", id2,
                    segment_inf_[RRSegmentId(std::get<0>(virtual_chan_loc_map_[std::make_tuple(loc2, type2)][id2]))].name.c_str());
        }
    }

    void print_real_rr_graph() {
        VTR_LOG("Printing real RR graph\n");
        VTR_LOG("Virtual to real mapping:\n");
        for (auto i : virtual_beg_to_real_) {
            e_rr_type type;
            t_location loc;
            int virt_idx;
            std::tie(loc, type, virt_idx) = i.first;
            VTR_LOG("CHAN%c X:%d Y:%d virt_idx:%d -> idx:%d\n", type == CHANX ? 'X' : 'Y', loc.first, loc.second, virt_idx, i.second);
        }

        VTR_LOG("RR Nodes\n");
        for (auto i : loc_type_idx_to_rr_idx_) {
            t_location loc;
            e_rr_type type;
            int idx;
            std::tie(loc, type, idx) = i.first;
            VTR_LOG("\t X:%d Y:%d type:%d idx:%d -> rr_node:%d\n", loc.first, loc.second, type, idx, i.second);
        }
    }

    /** @brief Fill device_ctx rr_switch_inf structure and store id of each PIP type for future use
     */
    void create_switch_inf() {
        std::set<std::tuple<int /*timing*/, bool /*buffered*/>> seen;
        pip_types(seen, ar_);

        std::vector<t_rr_switch_inf> switches;
        switches.resize(seen.size() + 2);

        std::vector<std::tuple<std::tuple<int, bool>, int>> temp_;
        process_switches_array<std::vector<t_rr_switch_inf>&, int>(ar_, seen, switches, temp_);

        auto& rr_switches = device_ctx_.rr_graph_builder.rr_switch();
        rr_switches.resize(seen.size() + 2);

        for (int i = 0; i < (int)switches.size(); i++)
            rr_switches[RRSwitchId(i)] = switches[i];

        for (auto i : temp_)
            pips_models_[std::get<0>(i)] = std::get<1>(i);
    }

    /** @brief Creates mapping from tile_id to its location
     */
    void create_tile_id_to_loc() {
        int max_height = 0;
        for (const auto& tile : ar_.getTileList()) {
            tile_loc_bimap_.insert(tile.getName(), t_location(tile.getCol() + 1, tile.getRow() + 1));
            max_height = std::max(max_height, (int)(tile.getRow() + 1));
        }
        /* tile with id name -1 is assosiated with constant source tile */
        tile_loc_bimap_.insert(-1, t_location(1, max_height + 1));
    }

    void fill_node_mappings(int tile_id, int wire_id, int node_id) {
        wire_to_node_[std::make_tuple(tile_id, wire_id)] = node_id;
        node_to_locs_[node_id].insert(tile_loc_bimap_[tile_id]);

        if (wire_name_to_seg_idx_.find(str(wire_id)) == wire_name_to_seg_idx_.end())
            wire_name_to_seg_idx_[str(wire_id)] = -1;

        node_tile_to_segment_[std::make_tuple(node_id, tile_id)] = wire_name_to_seg_idx_[str(wire_id)];
    }

    /** @brief Helper function for create_uniq_node_ids. It process constant sources into single node.
     */
    void process_const_nodes() {
        for (const auto& tile : ar_.getTileList()) {
            int tile_id = tile.getName();
            auto tile_type = ar_.getTileTypeList()[tile.getType()];
            auto wires = tile_type.getWires();
            for (const auto& constant : tile_type.getConstants()) {
                int const_id = constant.getConstant() == Device::ConstantType::VCC ? VCC : GND;

                for (const auto wire_id : constant.getWires())
                    fill_node_mappings(tile_id, wires[wire_id], const_id);
            }
        }

        for (const auto& node_source : ar_.getConstants().getNodeSources()) {
            int tile_id = node_source.getTile();
            int wire_id = node_source.getWire();
            int const_id = node_source.getConstant() == Device::ConstantType::VCC ? VCC : GND;
            fill_node_mappings(tile_id, wire_id, const_id);
        }

        for (auto const const_node : {GND, VCC}) {
            wire_to_node_[std::make_tuple(-1, const_node)] = const_node;
            node_to_locs_[const_node].insert(tile_loc_bimap_[-1]);
            node_tile_to_segment_[std::make_tuple(const_node, -1)] = -1;
        }
    }

    /** @brief Create unique IDs for each FPGA Interchange node.
     *
     * In addition it creates mappings from wire to node ID and from node ID to its locations.
     * These ids are used later in the RR graph generation for site pins and pip conections (rr_edges)
     */
    void create_uniq_node_ids() {
        process_const_nodes();

        // Process nodes
        int id = 2; // ids 0 and 1 are reserved respectively for GND and VCC constants
        for (const auto& node : ar_.getNodes()) {
            bool is_constant_node = false;
            int node_id = OPEN;

            // Nodes connected to constant sources should be combined into single constant node
            for (const auto& wire_id_ : node.getWires()) {
                const auto& wire = ar_.getWires()[wire_id_];
                int tile_id = wire.getTile();
                int wire_id = wire.getWire();
                if (wire_to_node_.find(std::make_tuple(tile_id, wire_id)) != wire_to_node_.end() && wire_to_node_[std::make_tuple(tile_id, wire_id)] < 2) {
                    // At least one of node wires is marked as constant source, that make whole node constant source.
                    // Give this node id equal to const source type.
                    is_constant_node = true;
                    node_id = wire_to_node_[std::make_tuple(tile_id, wire_id)];
                }
            }

            // If the node is not part of constant network give it unique ID
            if (!is_constant_node)
                node_id = id++;

            VTR_ASSERT(node_id != OPEN);

            float capacitance = 0.0, resistance = 0.0;
            if (node_to_RC_.find(node_id) == node_to_RC_.end()) {
                node_to_RC_[node_id] = std::pair<float, float>(0, 0);
                // TODO: these values are only temporarily hard-coded in case there is no timing information.
                //       Need to handle this better.
                capacitance = 0.000000001;
                resistance = 5.7;
            }

            // Use timing model if present
            if (ar_.hasNodeTimings()) {
                auto model = ar_.getNodeTimings()[node.getNodeTiming()];
                capacitance = get_corner_value(model.getCapacitance(), "slow", "typ");
                resistance = get_corner_value(model.getResistance(), "slow", "typ");
            }

            node_to_RC_[node_id].first += resistance;
            node_to_RC_[node_id].second += capacitance;

            for (const auto& wire_id_ : node.getWires()) {
                const auto& wire = ar_.getWires()[wire_id_];
                int tile_id = wire.getTile();
                int wire_id = wire.getWire();

                fill_node_mappings(tile_id, wire_id, node_id);
            }
        }
        total_node_count_ = id;
    }

    /** @brief Creates an entry for each pip in the FPGA device architecture,
     *
     *  Additionally, it stores the following metadata for later physical netlist writing:
     *      - tile name
     *      - wire names
     *      - direction
     */
    void create_pips() {
        int pip_count = 0;

        for (const auto& tile : ar_.getTileList()) {
            const auto& tile_type = ar_.getTileTypeList()[tile.getType()];
            pip_count += tile_type.getPips().size();
        }

        pips_.reserve((int)(pip_count * 1.05));

        for (const auto& tile : ar_.getTileList()) {
            int tile_id = tile.getName();
            t_location loc = tile_loc_bimap_[tile_id];
            const auto& tile_type = ar_.getTileTypeList()[tile.getType()];

            for (const auto& pip : tile_type.getPips()) {
                int wire0_name, wire1_name;
                int node0, node1;
                int switch_id;

                wire0_name = tile_type.getWires()[pip.getWire0()];
                wire1_name = tile_type.getWires()[pip.getWire1()];

                if (wire_to_node_.find(std::make_tuple(tile_id, wire0_name)) == wire_to_node_.end())
                    continue;
                if (wire_to_node_.find(std::make_tuple(tile_id, wire1_name)) == wire_to_node_.end())
                    continue;

                node0 = wire_to_node_[std::make_tuple(tile_id, wire0_name)];
                node1 = wire_to_node_[std::make_tuple(tile_id, wire1_name)];

                // FIXME: Right now we allow for pseudo pips even tho we don't check if they are avaiable
                //if (pip.isPseudoCells() && (node0 > 1 && node1 > 1))
                //    continue;

                used_by_pip_.emplace(node0, loc);
                used_by_pip_.emplace(node1, loc);

                useful_node_.insert(node0);
                useful_node_.insert(node1);

                int name = tile_id;
                if (tile.hasSubTilesPrefices())
                    name = tile.getSubTilesPrefices()[pip.getSubTile()];

                VTR_ASSERT(pips_models_.find(std::make_tuple(pip.getTiming(), pip.getBuffered21())) != pips_models_.end());

                switch_id = pips_models_[std::make_tuple(pip.getTiming(), pip.getBuffered21())];

                auto source_sink = std::make_tuple(node0, node1);
                auto edge = t_pip(switch_id, tile_id, std::make_tuple(name, wire0_name, wire1_name, true));
                pips_.emplace(source_sink, edge);

                if (!pip.getBuffered21() && (pip.getDirectional() || pip.getBuffered20())) {
                    source_sink = std::make_tuple(node1, node0);
                    pips_.emplace(source_sink, t_pip(switch_id, tile_id, std::make_tuple(name, wire0_name, wire1_name, true)));
                }

                if (!pip.getDirectional()) {
                    switch_id = pips_models_[std::make_tuple(pip.getTiming(), pip.getBuffered20())];
                    source_sink = std::make_tuple(node1, node0);
                    pips_.emplace(source_sink, t_pip(switch_id, tile_id, std::make_tuple(name, wire0_name, wire1_name, true)));
                    if (!pip.getBuffered20() && pip.getBuffered21()) {
                        source_sink = std::make_tuple(node0, node1);
                        pips_.emplace(source_sink, t_pip(switch_id, tile_id, std::make_tuple(name, wire0_name, wire1_name, true)));
                    }
                }
            }
        }
    }

    bool pip_uses_node_loc(int node_id, t_location loc) {
        return used_by_pip_.find(std::make_tuple(node_id, loc)) != used_by_pip_.end();
    }

    bool pin_uses_node_loc(int node_id, t_location loc) {
        return used_by_pin_.find(std::make_tuple(node_id, loc)) != used_by_pin_.end();
    }

    /** @brief Builds graph of FPGA Interchange node for further computations
     *
     * An FPGA Interchange node graph can be interpreted as a set of wires
     * that are electrically connected by non-configurable shorts.
     *
     * A direct translation of nodes from the Interchange Format to the VTR RR graph
     * format is not possible, and some extra computation is required to build a node's
     * representation suitable fro the RR graph.
     *
     * The node graph may be split in multiple unconnected sub-graphs, each with its own root nodes,
     * and each sub-graph will need to be connected to each other, forming a single final node graph.
     *
     * The reason why a node graph is a forest, rather than a single tree (e.g. has multiple roots hence
     * multiple unconnected sub-graphs), is that with absent information on the location of the various wires
     * in the interchange format, the exact structure of the graph is not available and needs to be
     * reconstructed to suite the VTR RR graph representation.
     *
     * All this procedure is to allow having nodes that represent complex-shaped wires, namely L-wires or U-wires.
     */
    std::set<intermediate_node*> build_node_graph(int node_id,
                                                  std::set<t_location> nodes,
                                                  std::map<t_location, intermediate_node*>& existing_nodes,
                                                  int& seg_id) {
        std::set<intermediate_node*> roots;
        do {
            intermediate_node* root_node = nullptr;
            t_location key;
            for (const auto& it : nodes) {
                if (pip_uses_node_loc(node_id, it) || pin_uses_node_loc(node_id, it)) {
                    root_node = new intermediate_node(it);
                    key = it;
                    break;
                }
            }

            if (root_node == nullptr) {
                nodes.clear();
                break;
            }

            if (node_tile_to_segment_[std::make_tuple(node_id, tile_loc_bimap_[key])] != -1)
                seg_id = node_tile_to_segment_[std::make_tuple(node_id, tile_loc_bimap_[key])];

            nodes.erase(key);

            existing_nodes.emplace(key, root_node);

            std::queue<intermediate_node*> builder;
            builder.push(root_node);

            while (!builder.empty()) {
                intermediate_node* vertex;
                vertex = builder.front();
                builder.pop();
                t_location loc = vertex->loc;
                for (auto i : NODE_SIDES) {
                    t_location other_loc = add_vec(loc, offsets[i]);
                    bool temp = false;
                    if (existing_nodes.find(other_loc) != existing_nodes.end()) {
                        temp = true;
                    } else if (nodes.find(other_loc) != nodes.end()) {
                        intermediate_node* new_node = new intermediate_node(other_loc);
                        temp = true;

                        if (node_tile_to_segment_[std::make_tuple(node_id, tile_loc_bimap_[other_loc])] != -1)
                            seg_id = node_tile_to_segment_[std::make_tuple(node_id, tile_loc_bimap_[other_loc])];

                        nodes.erase(other_loc);
                        existing_nodes.emplace(other_loc, new_node);
                        builder.push(new_node);
                    }
                    vertex->links[i] = temp;
                }
            }

            roots.insert(root_node);
        } while (!nodes.empty());

        for (auto& node : existing_nodes) {
            if (pip_uses_node_loc(node_id, node.second->loc))
                node.second->on_route = true;

            if (pin_uses_node_loc(node_id, node.second->loc)) {
                node.second->has_pins = true;
                node.second->on_route = true;
            }
        }

        return roots;
    }

    /** @brief This function is used to update the bounding box of a node graph.
     *
     * parameters:
     *      - end: node at a side's frontier
     *      - node: node that might replace the end node
     *      - side: side where to check the bounding box expansion
     */
    intermediate_node* update_end(intermediate_node* end, intermediate_node* node, enum node_sides side) {
        intermediate_node* res = end;
        int x, y;
        x = end->loc.first > node->loc.first ? -1 : end->loc.first < node->loc.first ? 1 : 0;
        y = end->loc.second > node->loc.second ? -1 : end->loc.second < node->loc.second ? 1 : 0;
        switch (side) {
            case LEFT_EDGE:
                if (x < 0 || (x == 0 && y < 0))
                    res = node;
                break;
            case TOP_EDGE:
                if (y > 0 || (y == 0 && x > 0))
                    res = node;
                break;
            case RIGHT_EDGE:
                if (x > 0 || (x == 0 && y > 0))
                    res = node;
                break;
            case BOTTOM_EDGE:
                if (y < 0 || (y == 0 && x < 0))
                    res = node;
                break;
            default:
                VTR_ASSERT(false);
                break;
        }

        return res;
    }

    /** @brief Creates a line of nodes connecting two points.
     *
     * parameters:
     *      - existing_nodes: mapping of nodes at locations
     *      - start: starting location
     *      - end: ending location
     *      - connections: sides that are used for this line
     *
     *  If intermediate nodes are missing, they will be created.
     *
     *  TODO: consider using segments of length > 1 and create one node only
     *        instead of multiple nodes on a straight line.
     */
    void add_line(std::map<t_location, intermediate_node*>& existing_nodes,
                  t_location start,
                  t_location end,
                  std::initializer_list<enum node_sides> connections) {
        bool horizontal = start.second == end.second;
        VTR_ASSERT(horizontal || start.first == end.first);

        int range_start = horizontal ? start.first + 1 : start.second + 1;
        int range_end = horizontal ? end.first : end.second;

        for (int i = range_start; i < range_end; i++) {
            t_location loc = horizontal ? t_location(i, end.second) : t_location(end.first, i);

            if (existing_nodes.find(loc) == existing_nodes.end())
                existing_nodes[loc] = new intermediate_node(loc);

            intermediate_node* loc_ = existing_nodes[loc];
            loc_->back_bone = true;

            for (auto j : connections)
                loc_->links[j] = true;
        }
    }

    /** @brief Adds a middle node to allow connecting end nodes that would form a non-straight
     *         line of nodes.
     *
     *  +----------+
     *  |          |          +--------+
     *  | End Node +----------+ Middle |
     *  |          |          |  Node  |
     *  +----------+          +---+----+
     *                            |
     *                            |
     *                            |
     *                       +----+-----+
     *                       |          |
     *                       | End Node |
     *                       |          |
     *                       +----------+
     */
    void add_middle_node(std::map<t_location, intermediate_node*>& existing_nodes,
                         intermediate_node* ends[],
                         t_location node_loc,
                         bool is_up,
                         enum node_sides node_side,
                         enum node_sides end_side) {
        if (existing_nodes.find(node_loc) == existing_nodes.end())
            existing_nodes[node_loc] = new intermediate_node(node_loc);

        intermediate_node* node = existing_nodes[node_loc];
        node->back_bone = true;

        if (node != ends[node_side]) {
            node->links[node_side] = true;
            ends[node_side]->links[end_side] = true;

            if (node->loc.first < ends[node_side]->loc.first)
                add_line(existing_nodes, node->loc, ends[node_side]->loc, {RIGHT_EDGE, LEFT_EDGE});
            else
                add_line(existing_nodes, ends[node_side]->loc, node->loc, {RIGHT_EDGE, LEFT_EDGE});
        }

        if (is_up && node->loc.second != ends[TOP_EDGE]->loc.second) {
            node->links[TOP_EDGE] = true;
            ends[TOP_EDGE]->links[BOTTOM_EDGE] = true;
            add_line(existing_nodes, node->loc, ends[TOP_EDGE]->loc, {TOP_EDGE, BOTTOM_EDGE});
        } else if (!is_up && node->loc.second != ends[BOTTOM_EDGE]->loc.second) {
            node->links[BOTTOM_EDGE] = true;
            ends[BOTTOM_EDGE]->links[TOP_EDGE] = true;
            add_line(existing_nodes, ends[BOTTOM_EDGE]->loc, node->loc, {TOP_EDGE, BOTTOM_EDGE});
        }
    }

    /** @brief Creates the final root node that will be used to represent an Interchange node graph
     *
     * Given that there may be different root nodes for the same node graph, we need to elect one to
     * be the new final root, so that we can than build the RR graph connections between all the
     * generated nodes reliably.
     */
    intermediate_node* create_final_root_node(std::map<t_location, intermediate_node*>& existing_nodes,
                                              bool left_node,
                                              t_location root_node,
                                              t_location up_node,
                                              t_location middle_node,
                                              enum node_sides side) {
        if (existing_nodes.find(root_node) == existing_nodes.end())
            existing_nodes[root_node] = new intermediate_node(root_node);

        intermediate_node* temp = existing_nodes[root_node];

        temp->back_bone = true;
        temp->links[side] = true;
        temp->links[BOTTOM_EDGE] = true;

        add_line(existing_nodes, up_node, root_node, {TOP_EDGE, BOTTOM_EDGE});

        existing_nodes[up_node]->links[TOP_EDGE] = true;
        existing_nodes[middle_node]->links[OPPOSITE_NODE_SIDES[side]] = true;

        if (left_node)
            add_line(existing_nodes, root_node, middle_node, {RIGHT_EDGE, LEFT_EDGE});
        else
            add_line(existing_nodes, middle_node, root_node, {RIGHT_EDGE, LEFT_EDGE});

        return temp;
    }

    /** @brief Connect all the remaining root nodes that are not yet connected to the corresponding
     *         node graph.
     *
     *  This is to connect all the nodes that may be in the current situation:
     *
     *                          +---------+
     *                          |         |
     *                          | Node    |
     *                          |         |
     *                          +----+----+
     *                               |
     *                               |          +-------------+
     *                               |          |  Dangling   |
     *                               +----------+             |
     *                               |          | Root Node   |
     *                               |          +-------------+
     *                               |
     *   +---------+            +----+----+
     *   |         |            |  Final  |
     *   |  Node   +------------+         |
     *   |         |            |  Root   |
     *   +---------+            +----+----+
     *                               |
     *                               |
     *                               |
     *                               |
     *                               |
     *                          +----+----+
     *                          |         |
     *                          |  Node   |
     *                          |         |
     *                          +---------+
     */
    void connect_dangling_roots(std::set<intermediate_node*>& roots,
                                std::map<t_location, intermediate_node*>& existing_nodes,
                                intermediate_node* ends[]) {
        /* connect not yet connected roots */
        std::vector<intermediate_node*> del_list;

        int x_, y_;
        x_ = ends[RIGHT_EDGE]->loc.first - ends[LEFT_EDGE]->loc.first;
        y_ = ends[TOP_EDGE]->loc.second - ends[BOTTOM_EDGE]->loc.second;

        int max_length = std::max(x_, y_);

        for (const auto& root : roots) {
            bool done = root->back_bone;
            for (int j = 1; j < max_length; j++) {
                if (done) {
                    del_list.push_back(root);
                    break;
                }

                for (const auto& k : NODE_SIDES) {
                    t_location offset = mul_vec_scal(offsets[k], j);
                    t_location other_node_loc = add_vec(root->loc, offset);
                    if (existing_nodes.find(other_node_loc) == existing_nodes.end())
                        continue;
                    intermediate_node* other_node;
                    other_node = existing_nodes[other_node_loc];
                    if (!other_node->back_bone)
                        continue;
                    root->links[k] = true;
                    root->back_bone = true;
                    other_node->links[OPPOSITE_NODE_SIDES[k]] = true;

                    if (0 < k && k < 3)
                        add_line(existing_nodes, root->loc, other_node_loc, {k, OPPOSITE_NODE_SIDES[k]});
                    else
                        add_line(existing_nodes, other_node_loc, root->loc, {k, OPPOSITE_NODE_SIDES[k]});

                    done = true;
                    break;
                }
            }
        }

        // Remove roots that have already been processed
        for (const auto& i : del_list) {
            if (roots.find(i) != roots.end())
                roots.erase(i);
        }

        del_list.clear();
    }

    /** @brief Connects to unconnected graphs into a single node graph.
     *
     * If the node graph is composed of multiple unconnected trees, this procedure will connect all
     * of the various roots of the sub-trees together, so that a single node graph is present.
     */
    intermediate_node* connect_roots(std::set<intermediate_node*>& roots,
                                     std::map<t_location, intermediate_node*>& existing_nodes) {
        // The node graph is fully connected, therefore no extra computation is required
        if (roots.size() == 1) {
            intermediate_node* root = *roots.begin();
            root->back_bone = true;
            roots.clear();
            return root;
        }

        intermediate_node* ends[4];
        ends[0] = ends[1] = ends[2] = ends[3] = *roots.begin();

        for (auto root : roots)
            for (auto const& side : NODE_SIDES)
                ends[side] = update_end(ends[side], root, side);

        for (auto const& side : NODE_SIDES) {
            ends[side]->back_bone = true;
            if (roots.find(ends[side]) != roots.end())
                roots.erase(ends[side]);
        }

        bool right_to_top = ends[TOP_EDGE]->loc.first >= ends[BOTTOM_EDGE]->loc.first;
        bool left_to_top = ends[TOP_EDGE]->loc.first < ends[BOTTOM_EDGE]->loc.first;

        t_location right_middle_node(right_to_top ? ends[TOP_EDGE]->loc.first : ends[BOTTOM_EDGE]->loc.first, ends[RIGHT_EDGE]->loc.second);
        t_location left_middle_node(left_to_top ? ends[TOP_EDGE]->loc.first : ends[BOTTOM_EDGE]->loc.first, ends[LEFT_EDGE]->loc.second);

        add_middle_node(existing_nodes, ends, right_middle_node, right_to_top, RIGHT_EDGE, LEFT_EDGE);
        add_middle_node(existing_nodes, ends, left_middle_node, left_to_top, LEFT_EDGE, RIGHT_EDGE);

        int y = std::max(right_middle_node.second, left_middle_node.second);
        t_location loc1(left_middle_node.first, y);
        t_location loc2(right_middle_node.first, y);

        intermediate_node* true_root = nullptr;

        // The final root node is chosen among the possible different ones, depending on the various intermediate nodes
        // locations.
        if (loc1 != left_middle_node && loc1 != loc2) {
            true_root = create_final_root_node(existing_nodes, true, loc1, left_middle_node, right_middle_node, RIGHT_EDGE);
        } else if (loc2 != right_middle_node && loc1 != loc2) {
            true_root = create_final_root_node(existing_nodes, false, loc2, right_middle_node, left_middle_node, LEFT_EDGE);
        } else if (loc1 != loc2) {
            add_line(existing_nodes, loc1, loc2, {RIGHT_EDGE, LEFT_EDGE});
            existing_nodes[loc1]->links[RIGHT_EDGE] = true;
            existing_nodes[loc2]->links[LEFT_EDGE] = true;
            true_root = existing_nodes[loc1];
        } else {
            loc1 = right_middle_node;
            loc2 = left_middle_node;

            if (right_middle_node.second > left_middle_node.second)
                std::swap(loc1, loc2);

            add_line(existing_nodes, loc1, loc2, {TOP_EDGE, BOTTOM_EDGE});
            existing_nodes[loc1]->links[TOP_EDGE] = true;
            existing_nodes[loc2]->links[BOTTOM_EDGE] = true;
            true_root = existing_nodes[loc1];
        }

        connect_dangling_roots(roots, existing_nodes, ends);

        VTR_ASSERT(true_root != nullptr);
        return true_root;
    }

    /** @brief Removes dangling nodes from a graph represented by the root node.
     *
     *  Dangling nodes are nodes that do not connect to a pin, a pip or other non-dangling node.
     */
    int reduce_graph_and_count_nodes_left(intermediate_node* root,
                                          std::map<t_location, intermediate_node*>& existing_nodes,
                                          int seg_id) {
        int cnt = 0;
        std::queue<intermediate_node*> walker;
        std::stack<intermediate_node*> back_walker;

        walker.push(root);
        root->visited = root;

        while (!walker.empty()) {
            intermediate_node* vertex = walker.front();
            walker.pop();
            back_walker.push(vertex);
            vertex->segment_id = seg_id;

            bool single_node = true;
            bool has_chanx = false;

            for (auto i : NODE_SIDES) {
                if (vertex->links[i]) {
                    single_node = false;
                    intermediate_node* other_node = existing_nodes[add_vec(vertex->loc, offsets[i])];
                    if (other_node->visited == nullptr || vertex->visited == other_node) {
                        if (i == node_sides::LEFT_EDGE) {
                            has_chanx = true;
                            cnt++;
                        } else if (i == node_sides::TOP_EDGE) {
                            cnt++;
                        }

                        if (other_node->visited == nullptr) {
                            other_node->visited = vertex;
                            walker.push(other_node);
                        }
                    } else {
                        vertex->links[i] = false;
                    }
                }
            }

            if ((vertex->has_pins && !has_chanx) || single_node)
                cnt++;
        }

        while (!back_walker.empty()) {
            intermediate_node* vertex = back_walker.top();
            back_walker.pop();

            bool single_node = true;

            if (!vertex->has_pins && !vertex->on_route) {
                for (auto i : NODE_SIDES)
                    if (vertex->links[i])
                        if (i == node_sides::LEFT_EDGE || i == node_sides::TOP_EDGE)
                            cnt--;

                existing_nodes.erase(vertex->loc);
                delete vertex;
            } else {
                vertex->visited->on_route = true;
                for (auto i : NODE_SIDES) {
                    if (vertex->links[i]) {
                        if (existing_nodes.find(add_vec(vertex->loc, offsets[i])) == existing_nodes.end()) {
                            vertex->links[i] = false;
                            if ((i == node_sides::LEFT_EDGE && !vertex->has_pins) || i == node_sides::TOP_EDGE)
                                cnt--;
                        }
                    }
                }

                for (auto i : NODE_SIDES)
                    single_node &= !vertex->links[i];

                if (!vertex->has_pins && single_node)
                    cnt++;
            }
        }

        return cnt;
    }

    /** @brief Processes the set of nodes to populate the virtual CHAN loation maps
     */
    void process_set(std::unordered_set<intermediate_node*>& set,
                     std::unordered_set<t_location, hash_tuple::hash<t_location>>& nodes_used,
                     std::map<t_location, intermediate_node*>& existing_nodes,
                     std::map<t_location, std::tuple<t_location, e_rr_type, int>>& local_redirect,
                     float R,
                     float C,
                     t_location offset,
                     node_sides side,
                     e_rr_type chan_type) {
        for (auto const inode : set) {
            int len = 0;
            intermediate_node* start = inode;
            intermediate_node* end = inode;
            auto key = std::make_tuple(add_vec(start->loc, offset), chan_type);
            int idx = virtual_chan_loc_map_[key].size();
            do {
                nodes_used.insert(end->loc);
                len++;
                local_redirect.emplace(end->loc, std::make_tuple(add_vec(start->loc, offset), chan_type, idx));

                if (!end->links[side])
                    // Reached the end of this set of nodes exiting the loop
                    break;

                intermediate_node* neighbour = existing_nodes[add_vec(end->loc, offsets[side])];

                if (set.find(neighbour) != set.end())
                    break;

                end = neighbour;
            } while (true);

            virtual_chan_loc_map_[key].emplace_back(start->segment_id, R * len, C * len, add_vec(end->loc, offset));
        }
    }

    /** @brief Adds a short between two node locations between two nodes existing in the
     *         same Interchange node graph. This populates the virtual shorts map.
     */
    void add_short(t_location n1,
                   t_location n2,
                   std::map<t_location, std::tuple<t_location, e_rr_type, int>>& r1,
                   std::map<t_location, std::tuple<t_location, e_rr_type, int>>& r2) {
        auto red1 = r1[n1];
        auto red2 = r2[n2];
        virtual_shorts_.emplace_back(std::get<0>(red1), std::get<1>(red1), std::get<2>(red1),
                                     std::get<0>(red2), std::get<1>(red2), std::get<2>(red2));
    }

    /** @brief Adds connections between nodes belonging to the same Interchange node graph
     */
    void connect_base_on_redirects(std::unordered_set<intermediate_node*>& set,
                                   node_sides side,
                                   std::map<t_location, std::tuple<t_location, e_rr_type, int>>& src,
                                   std::map<t_location, std::tuple<t_location, e_rr_type, int>>& dist) {
        for (auto const node : set) {
            if (!node->links[side])
                continue;

            t_location other_node = add_vec(node->loc, offsets[side]);

            if (dist.find(other_node) == dist.end())
                continue;

            add_short(node->loc, other_node, src, dist);
        }
    }

    /** @brief Translates the Interchange node graph representation in a format suitable
     *         to be translated into the VTR RR graph representation.
     *
     *         In practice this means populating the mappings to better access data in the
     *         real RR graph building stage.
     */
    void graph_reduction_stage2(int node_id,
                                std::map<t_location, intermediate_node*>& existing_nodes,
                                float R,
                                float C) {
        std::unordered_set<intermediate_node*> chanxs, chanys;
        std::unordered_set<t_location, hash_tuple::hash<t_location>> nodes_in_chanxs, nodes_in_chanys;

        // Populate sets containing starting point where horizontal (CHANX) or vertical (CHANY) lines of
        // nodes are linked together.
        //
        // In the following there will be four sets of nodes:
        //      - three sets for CHANY to build the three veritcal sets of nodes
        //      - one set for CHANX to connect together all the bottom nodes
        //
        //    +---+                    +---+                    +---+
        //    |   |                    |   |                    |   |
        //    +-+-+                    +-+-+                    +-+-+
        //      |                        |                        |
        //      |                        |                        |
        //      |                        |                        |
        //      |                        |                        |
        //    +-+-+                    +-+-+                    +-+-+
        //    |   +--------------------+   +--------------------+   |
        //    +---+                    +---+                    +---+
        //
        for (auto const& node : existing_nodes) {
            bool single_node = true;
            for (auto side : NODE_SIDES)
                single_node &= !node.second->links[side];

            bool chanx_start = false;
            bool chany_start = false;

            if (node.second->links[LEFT_EDGE]) {
                intermediate_node* left_node = existing_nodes[add_vec(node.second->loc, offsets[LEFT_EDGE])];
                chanx_start = pip_uses_node_loc(node_id, left_node->loc) || left_node->links[TOP_EDGE] || left_node->links[BOTTOM_EDGE];
            } else {
                chanx_start = node.second->has_pins || single_node;
                if (node.second->links[RIGHT_EDGE])
                    nodes_in_chanxs.insert(node.first);
            }
            if (node.second->links[TOP_EDGE]) {
                intermediate_node* top_node = existing_nodes[add_vec(node.second->loc, offsets[TOP_EDGE])];
                chany_start = pip_uses_node_loc(node_id, top_node->loc) || pin_uses_node_loc(node_id, top_node->loc);
                chany_start |= top_node->links[LEFT_EDGE] || top_node->links[RIGHT_EDGE];
            } else if (node.second->links[BOTTOM_EDGE]) {
                nodes_in_chanys.insert(node.first);
            }

            if (chanx_start)
                chanxs.insert(node.second);
            if (chany_start)
                chanys.insert(node.second);
        }

        std::map<t_location, std::tuple<t_location, e_rr_type, int>> local_redirect_x, local_redirect_y;

        process_set(chanys, nodes_in_chanys, existing_nodes, local_redirect_y, R, C, t_location(0, 0), BOTTOM_EDGE, CHANY);
        process_set(chanxs, nodes_in_chanxs, existing_nodes, local_redirect_x, R, C, offsets[BOTTOM_EDGE], RIGHT_EDGE, CHANX);

        connect_base_on_redirects(chanys, TOP_EDGE, local_redirect_y, local_redirect_y);
        connect_base_on_redirects(chanxs, LEFT_EDGE, local_redirect_x, local_redirect_x);

        // Connect CHANY with CHANX nodes
        for (auto i : nodes_in_chanys) {
            t_location node = i;
            if (nodes_in_chanxs.find(i) != nodes_in_chanxs.end()) {
                bool ry = local_redirect_y.find(node) != local_redirect_y.end();
                bool rx = local_redirect_x.find(node) != local_redirect_x.end();
                t_location x, y;
                if (rx)
                    x = node;
                else
                    x = add_vec(node, offsets[RIGHT_EDGE]);

                if (ry)
                    y = node;
                else
                    y = add_vec(node, offsets[BOTTOM_EDGE]);

                add_short(y, x, local_redirect_y, local_redirect_x);
            }
        }

        for (auto const& node : existing_nodes) {
            bool ry = local_redirect_y.find(node.first) != local_redirect_y.end();
            bool rx = local_redirect_x.find(node.first) != local_redirect_x.end();

            if (rx) {
                virtual_redirect_.emplace(std::make_tuple(node_id, node.first), local_redirect_x[node.first]);
            } else if (ry) {
                virtual_redirect_.emplace(std::make_tuple(node_id, node.first), local_redirect_y[node.first]);
            } else if (node.second->links[RIGHT_EDGE]) {
                t_location right_node = add_vec(node.first, offsets[RIGHT_EDGE]);
                virtual_redirect_.emplace(std::make_tuple(node_id, node.first), local_redirect_x[right_node]);
            } else if (node.second->links[BOTTOM_EDGE]) {
                t_location bottom_node = add_vec(node.first, offsets[BOTTOM_EDGE]);
                virtual_redirect_.emplace(std::make_tuple(node_id, node.first), local_redirect_y[bottom_node]);
            } else {
                VTR_ASSERT(false);
            }
        }
    }

    /** @brief Removes a node graph represented by a root node.
     *
     * This step is used to clean up of earlier intermediate stages.
     */
    void delete_nodes(std::map<t_location, intermediate_node*>& existing_nodes) {
        for (auto i : existing_nodes) {
            delete i.second;
        }
        existing_nodes.clear();
    }

    /** @brief Process FPGA Interchange nodes.
     *
     * This function processes all the wires that are connected together and are part of the same node graph.
     */
    void process_nodes() {
        for (int node_id = 0; node_id < total_node_count_; ++node_id) {
            int seg_id;
            if (useful_node_.find(node_id) == useful_node_.end()) {
                continue;
            }
            std::set<t_location> all_possible_tiles = node_to_locs_[node_id];

            std::map<t_location, intermediate_node*> existing_nodes;

            std::set<intermediate_node*> roots = build_node_graph(node_id, all_possible_tiles, existing_nodes, seg_id);
            intermediate_node* root = connect_roots(roots, existing_nodes);

            VTR_ASSERT(roots.empty());

            int div = reduce_graph_and_count_nodes_left(root, existing_nodes, seg_id);

            if (existing_nodes.empty())
                continue;

            node_id_count_[node_id] = div;
            VTR_ASSERT(div > 0);

            // Given that an Interchange graph node is composed of many electrically connected wires
            // where each one is a separate VTR RR graph node, the resistance and capacitance values
            // need to be split among all of the different nodes, as in the end, they are all part of
            // the same Interchange "node".
            float resistance = node_to_RC_[node_id].first / div;
            float capacitance = node_to_RC_[node_id].second / div;
            graph_reduction_stage2(node_id, existing_nodes, resistance, capacitance);
            delete_nodes(existing_nodes);
        }
    }

    /** @brief Finds the next suitable site index to create SINK and SOURCE nodes
     */
    int next_good_site(int first_idx, const Device::Tile::Reader tile) {
        auto tile_type = ar_.getTileTypeList()[tile.getType()];
        size_t ans = first_idx;
        for (; ans < tile.getSites().size(); ans++) {
            auto site = tile.getSites()[ans];
            auto site_type = ar_.getSiteTypeList()[tile_type.getSiteTypes()[site.getType()].getPrimaryType()];

            bool found = false;
            for (auto bel : site_type.getBels())
                found |= bel_cell_mappings_.find(bel.getName()) != bel_cell_mappings_.end();

            if (found)
                break;
        }

        return ans;
    }

    /** @brief Creates site SINK/SOURCE IPIN/OPIN mappings.
     */
    void create_sink_source_nodes() {
        const auto& tile_types = ar_.getTileTypeList();
        std::map<std::tuple<int /*tile type id*/, int, std::string>, std::tuple<float, int>> tile_type_site_num_pin_name_model;

        // Fill tile pin to wire map
        for (const auto& tileType : tile_types) {
            int it = 0;

            if (!tile_type_to_pb_type_.count(str(tileType.getName())))
                continue;

            for (const auto& siteType : tileType.getSiteTypes()) {
                int pin_count = 0;
                for (const auto& pin_ : ar_.getSiteTypeList()[siteType.getPrimaryType()].getPins()) {
                    std::tuple<int, int, std::string> pin(tileType.getName(), it, str(pin_.getName()));
                    const auto& model = pin_.getModel();
                    float value = 0.0;

                    if (pin_.getDir() == LogicalNetlist::Netlist::Direction::OUTPUT && model.hasResistance())
                        value = get_corner_value(model.getResistance(), "slow", "max");
                    else if (pin_.getDir() == LogicalNetlist::Netlist::Direction::INPUT && model.hasCapacitance())
                        value = get_corner_value(model.getCapacitance(), "slow", "max");

                    int wire_id = siteType.getPrimaryPinsToTileWires()[pin_count];
                    tile_type_site_num_pin_name_model[pin] = std::tuple<float, int>(value, wire_id);
                    pin_count++;
                }
                it++;
            }
        }

        for (const auto& tile : ar_.getTileList()) {
            const auto& tile_type_id = tile_types[tile.getType()].getName();
            const auto& tile_type_name = str(tile_type_id);
            const auto& tile_id = tile.getName();

            if (!tile_type_to_pb_type_.count(tile_type_name))
                continue;

            int it = next_good_site(0, tile);

            sink_source_loc_map_[tile_id].resize(tile_type_to_pb_type_[tile_type_name]->num_pins);
            for (const auto& sub_tile : tile_type_to_pb_type_[tile_type_name]->sub_tiles) {
                for (const auto& port : sub_tile.ports) {
                    int pin_id = sub_tile.sub_tile_to_tile_pin_indices[port.index];

                    std::tuple<int, int, std::string> key{tile_type_id, it, std::string(port.name)};

                    float value;
                    int wire_id;
                    std::tie<float, int>(value, wire_id) = tile_type_site_num_pin_name_model[key];

                    int node_id;
                    if (!wire_to_node_.count(std::make_tuple(tile_id, wire_id)))
                        node_id = -1;
                    else
                        node_id = wire_to_node_[std::make_tuple(tile_id, wire_id)];

                    t_location loc = tile_loc_bimap_[tile_id];

                    used_by_pin_.insert(std::make_tuple(node_id, loc));
                    useful_node_.insert(node_id);
                    sink_source_loc_map_[tile_id][pin_id] = std::make_tuple(port.type == IN_PORT, value, node_id);
                }
                it = next_good_site(it + 1, tile);
            }
        }

        // Add constant sources. Tile ID "-1" corresponds to the constant synthetic tile
        sink_source_loc_map_[-1].resize(2);
        for (auto pin_id : {0, 1}) {
            t_location loc = tile_loc_bimap_[-1];
            used_by_pin_.insert(std::make_tuple(pin_id, loc));
            useful_node_.insert(pin_id);
            sink_source_loc_map_[-1][pin_id] = std::make_tuple(false, 0, pin_id);
        }
    }

    void sweep(t_location loc,
               e_rr_type type,
               int& used_tracks,
               std::unordered_map<int, std::list<int>>& sweeper,
               std::list<int>& avaiable_tracks) {
        auto loc_type_tuple = std::make_tuple(loc, type);
        for (size_t k = 0; k < virtual_chan_loc_map_[loc_type_tuple].size(); ++k) {
            auto track = virtual_chan_loc_map_[loc_type_tuple][k];
            int new_id;

            if (!avaiable_tracks.empty()) {
                new_id = avaiable_tracks.front();
                avaiable_tracks.pop_front();
            } else {
                new_id = used_tracks++;
            }

            virtual_beg_to_real_[std::make_tuple(loc, type, k)] = new_id;
            chan_loc_map_[loc_type_tuple][new_id] = track;
            int stop = type == CHANX ? std::get<3>(track).first : std::get<3>(track).second;
            sweeper[stop].push_back(new_id);
        }

        int pos = type == CHANX ? loc.first : loc.second;

        while (!sweeper[pos].empty()) {
            avaiable_tracks.push_back(sweeper[pos].front());
            sweeper[pos].pop_front();
        }

        virtual_chan_loc_map_[loc_type_tuple].clear();
    }

    /** @brief Creates the mapping from the virtual to physical tracks.
     *
     * It works in O(N*M + L), where N and M are the device dimensions
     * and L is the number of used segments in the various CHANs
     */
    void virtual_to_real() {
        device_ctx_.chan_width.x_list.resize(grid_.height(), 0);
        device_ctx_.chan_width.y_list.resize(grid_.width(), 0);

        int min_x, min_y, max_x, max_y;
        min_x = min_y = std::numeric_limits<int>::max();
        max_x = max_y = 0;

        std::unordered_map<int, std::list<int>> sweeper;
        std::list<int> avaiable_tracks;

        int used_tracks;
        for (size_t i = 0; i < grid_.height(); ++i) {
            used_tracks = 0;
            avaiable_tracks.clear();

            for (size_t j = 0; j < grid_.width(); ++j)
                sweep(t_location(j, i), CHANX, used_tracks, sweeper, avaiable_tracks);

            min_x = std::min(min_x, used_tracks);
            max_x = std::max(max_x, used_tracks);
            device_ctx_.chan_width.x_list[i] = used_tracks;
        }

        for (size_t i = 0; i < grid_.width(); ++i) {
            used_tracks = 0;
            avaiable_tracks.clear();

            for (size_t j = grid_.height() - 1; j < (size_t)-1; --j)
                sweep(t_location(i, j), CHANY, used_tracks, sweeper, avaiable_tracks);

            min_y = std::min(min_y, used_tracks);
            max_y = std::max(max_y, used_tracks);
            device_ctx_.chan_width.y_list[i] = used_tracks;
        }

        device_ctx_.chan_width.max = std::max(max_y, max_x);
        device_ctx_.chan_width.x_min = min_x;
        device_ctx_.chan_width.x_max = max_x;
        device_ctx_.chan_width.y_min = min_y;
        device_ctx_.chan_width.y_max = max_y;
    }

    void pack_tiles() {
        for (auto& node_loc : sink_source_loc_map_) {
            int tile_id = node_loc.first;
            int x, y;
            t_location loc = tile_loc_bimap_[tile_id];
            std::tie(x, y) = loc;

            auto pin_vec = node_loc.second;
            for (size_t pin_id = 0; pin_id < pin_vec.size(); ++pin_id) {
                bool input;
                float RC;
                int interchange_node_id;
                std::tie(input, RC, interchange_node_id) = pin_vec[pin_id];

                e_rr_type pin = input ? e_rr_type::SINK : e_rr_type::SOURCE;

                float R, C;
                std::tie(R, C) = input ? std::tuple<float, float>(0, RC) : std::tuple<float, float>(RC, 0);

                if (interchange_node_id != -1) {
                    t_location track_loc;
                    e_rr_type track_type;
                    int track_idx;
                    if (virtual_redirect_.find(std::make_tuple(interchange_node_id, loc)) == virtual_redirect_.end()) {
                        VTR_LOG("node_id:%d, loc->X:%d Y:%d\n", interchange_node_id, loc.first, loc.second);
                        VTR_ASSERT(false);
                    }
                    auto virtual_key = virtual_redirect_[std::make_tuple(interchange_node_id, loc)];
                    track_idx = virtual_beg_to_real_[virtual_key];
                    std::tie(track_loc, track_type, std::ignore) = virtual_key;

                    auto val = chan_loc_map_[std::make_tuple(track_loc, track_type)][track_idx];
                    int track_seg;
                    float track_R, track_C;
                    t_location end;
                    std::tie(track_seg, track_R, track_C, end) = val;
                    track_R += R;
                    track_C += C;

                    if (node_id_count_[interchange_node_id] == 1)
                        track_seg = 0; //set __generic__ segment type for wires going to/from site and that have pips in tile

                    chan_loc_map_[std::make_tuple(track_loc, track_type)][track_idx] = std::make_tuple(track_seg, track_R, track_C, end);
                }

                rr_nodes_.make_room_for_node(RRNodeId(rr_idx));
                loc_type_idx_to_rr_idx_[std::make_tuple(loc, pin, pin_id)] = rr_idx;
                auto node = rr_nodes_[rr_idx];
                RRNodeId node_id = node.id();

                device_ctx_.rr_graph_builder.set_node_type(node_id, pin);
                device_ctx_.rr_graph_builder.set_node_capacity(node_id, 1);
                device_ctx_.rr_graph_builder.set_node_rc_index(node_id, NodeRCIndex(find_create_rr_rc_data(0, 0)));
                device_ctx_.rr_graph_builder.set_node_coordinates(node_id, x, y, x, y);
                device_ctx_.rr_graph_builder.set_node_ptc_num(node_id, pin_id);

                if (pin == e_rr_type::SOURCE)
                    device_ctx_.rr_graph_builder.set_node_cost_index(node_id, RRIndexedDataId(SOURCE_COST_INDEX));
                else
                    device_ctx_.rr_graph_builder.set_node_cost_index(node_id, RRIndexedDataId(SINK_COST_INDEX));

                rr_idx++;
            }

            for (size_t pin_id = 0; pin_id < pin_vec.size(); ++pin_id) {
                bool input;
                int interchange_node_id;
                std::tie(input, std::ignore, interchange_node_id) = pin_vec[pin_id];

                if (interchange_node_id == -1)
                    continue;

                e_rr_type pin = input ? e_rr_type::IPIN : e_rr_type::OPIN;

                rr_nodes_.make_room_for_node(RRNodeId(rr_idx));
                loc_type_idx_to_rr_idx_[std::make_tuple(loc, pin, pin_id)] = rr_idx;
                auto node = rr_nodes_[rr_idx];
                RRNodeId node_id = node.id();

                device_ctx_.rr_graph_builder.set_node_type(node_id, pin);
                device_ctx_.rr_graph_builder.set_node_capacity(node_id, 1);
                device_ctx_.rr_graph_builder.set_node_rc_index(node_id, NodeRCIndex(find_create_rr_rc_data(0, 0)));
                device_ctx_.rr_graph_builder.set_node_coordinates(node_id, x, y, x, y);
                device_ctx_.rr_graph_builder.set_node_ptc_num(node_id, pin_id);
                device_ctx_.rr_graph_builder.add_node_side(node_id, e_side::BOTTOM);

                if (pin == e_rr_type::IPIN)
                    device_ctx_.rr_graph_builder.set_node_cost_index(node_id, RRIndexedDataId(IPIN_COST_INDEX));
                else
                    device_ctx_.rr_graph_builder.set_node_cost_index(node_id, RRIndexedDataId(OPIN_COST_INDEX));

                rr_idx++;
            }
        }
    }

    void pack_chans() {
        for (auto i : chan_loc_map_) {
            t_location loc;
            e_rr_type type;
            std::tie(loc, type) = i.first;
            int x, y;
            std::tie(x, y) = loc;

            auto tracks = i.second;
            for (auto track : tracks) {
                int seg;
                float R, C;
                t_location end;
                int x1, y1;
                std::tie(seg, R, C, end) = track.second;
                std::tie(x1, y1) = end;

                rr_nodes_.make_room_for_node(RRNodeId(rr_idx));
                loc_type_idx_to_rr_idx_[std::make_tuple(loc, type, track.first)] = rr_idx;
                auto node = rr_nodes_[rr_idx];
                RRNodeId node_id = node.id();

                device_ctx_.rr_graph_builder.set_node_type(node_id, type);
                device_ctx_.rr_graph_builder.set_node_capacity(node_id, 1);
                device_ctx_.rr_graph_builder.set_node_rc_index(node_id, NodeRCIndex(find_create_rr_rc_data(R, C)));
                device_ctx_.rr_graph_builder.set_node_direction(node_id, Direction::BIDIR);

                device_ctx_.rr_graph_builder.set_node_coordinates(node_id, x, y, x1, y1);
                device_ctx_.rr_graph_builder.set_node_ptc_num(node_id, track.first);

                if (type == e_rr_type::CHANX)
                    device_ctx_.rr_graph_builder.set_node_cost_index(node_id, RRIndexedDataId(CHANX_COST_INDEX_START + seg));
                else
                    device_ctx_.rr_graph_builder.set_node_cost_index(node_id, RRIndexedDataId(CHANX_COST_INDEX_START + segment_inf_.size() + seg));

                seg_index_[device_ctx_.rr_graph.node_cost_index(node_id)] = seg;

                rr_idx++;
            }
        }
    }

    void pack_rr_nodes() {
        rr_nodes_.clear();
        seg_index_.resize(CHANX_COST_INDEX_START + 2 * segment_inf_.size(), -1);
        pack_tiles();
        pack_chans();
        rr_nodes_.shrink_to_fit();
    }

    void pack_tiles_edges() {
        for (auto& i : sink_source_loc_map_) {
            int tile_id = i.first;
            int x, y;
            t_location loc = tile_loc_bimap_[tile_id];
            std::tie(x, y) = loc;
            auto pin_vec = i.second;
            for (size_t ipin = 0; ipin < pin_vec.size(); ++ipin) {
                bool input;
                int node_id;
                std::tie(input, std::ignore, node_id) = pin_vec[ipin];

                if (node_id == -1)
                    continue;

                auto virtual_chan_key = virtual_redirect_[std::make_tuple(node_id, loc)];
                e_rr_type pin = input ? e_rr_type::SINK : e_rr_type::SOURCE;
                e_rr_type mux = input ? e_rr_type::IPIN : e_rr_type::OPIN;
                auto chan_key = std::make_tuple(std::get<0>(virtual_chan_key),
                                                std::get<1>(virtual_chan_key),
                                                virtual_beg_to_real_[virtual_chan_key]);

                int pin_id, mux_id, track_id;
                pin_id = loc_type_idx_to_rr_idx_[std::make_tuple(loc, pin, ipin)];
                mux_id = loc_type_idx_to_rr_idx_[std::make_tuple(loc, mux, ipin)];
                track_id = loc_type_idx_to_rr_idx_[chan_key];

                int sink, sink_src, src;
                sink = input ? pin_id : track_id;
                sink_src = mux_id;
                src = input ? track_id : pin_id;

                device_ctx_.rr_graph_builder.emplace_back_edge(RRNodeId(src),
                                                               RRNodeId(sink_src),
                                                               src == track_id ? 1 : 0);
                device_ctx_.rr_graph_builder.emplace_back_edge(RRNodeId(sink_src),
                                                               RRNodeId(sink),
                                                               sink == track_id ? 1 : 0);
            }
        }
    }

    void pack_chans_edges() {
        for (auto& i : virtual_shorts_) {
            t_location l1, l2;
            e_rr_type t1, t2;
            int virtual_idx1, idx1, virtual_idx2, idx2;
            std::tie(l1, t1, virtual_idx1, l2, t2, virtual_idx2) = i;
            idx1 = virtual_beg_to_real_[std::make_tuple(l1, t1, virtual_idx1)];
            idx2 = virtual_beg_to_real_[std::make_tuple(l2, t2, virtual_idx2)];

            std::tuple<t_location, e_rr_type, int> key1(l1, t1, idx1), key2(l2, t2, idx2);

            int src, sink;
            src = loc_type_idx_to_rr_idx_[key1];
            sink = loc_type_idx_to_rr_idx_[key2];

            device_ctx_.rr_graph_builder.emplace_back_edge(RRNodeId(src), RRNodeId(sink), 0);
            device_ctx_.rr_graph_builder.emplace_back_edge(RRNodeId(sink), RRNodeId(src), 0);
        }
    }

    void pack_pips() {
        for (auto& i : pips_) {
            int node1, node2;
            std::tie(node1, node2) = i.first;
            int sw_id, tile_id;
            std::tuple<int, int, int, bool> metadata;
            std::tie(sw_id, tile_id, metadata) = i.second;
            VTR_ASSERT(sw_id > 1);
            int name, wire0, wire1;
            bool forward;
            std::tie(name, wire0, wire1, forward) = metadata;
            t_location loc = tile_loc_bimap_[tile_id];
            auto virtual_key1 = virtual_redirect_[std::make_tuple(node1, loc)];
            auto virtual_key2 = virtual_redirect_[std::make_tuple(node2, loc)];

            auto key1 = std::make_tuple(std::get<0>(virtual_key1),
                                        std::get<1>(virtual_key1),
                                        virtual_beg_to_real_[virtual_key1]);
            auto key2 = std::make_tuple(std::get<0>(virtual_key2),
                                        std::get<1>(virtual_key2),
                                        virtual_beg_to_real_[virtual_key2]);

            int src, sink;
            src = loc_type_idx_to_rr_idx_[key1];
            sink = loc_type_idx_to_rr_idx_[key2];
            device_ctx_.rr_graph_builder.emplace_back_edge(RRNodeId(src), RRNodeId(sink), sw_id);

            // TODO: find a more efficient way to pack PIP information to build the physical netlist out
            //       of a routed design
            char metadata_[100];
            char* temp = int_to_string(metadata_, name);
            *temp++ = '.';
            temp = int_to_string(temp, wire0);
            *temp++ = ',';
            temp = int_to_string(temp, wire1);
            *temp++ = '.';
            temp = int_to_string(temp, forward ? 1 : 0);
            *temp++ = 0;

            vpr::add_rr_edge_metadata(src, sink, sw_id, vtr::string_view("FPGAInterchange"), vtr::string_view(metadata_));
        }
    }

    void pack_rr_edges() {
        pack_tiles_edges();
        pack_chans_edges();
        pack_pips();
        device_ctx_.rr_graph_builder.mark_edges_as_rr_switch_ids();
        device_ctx_.rr_graph_builder.partition_edges();
    }

    /** @brief Fills the device RR graph.
     */
    void pack_to_rr_graph() {
        pack_rr_nodes();
        pack_rr_edges();
    }

    /** @brief Finilizes the RR graph generation.
     *
     *      - Builds the node lookup
     *      - Inits fan_ins
     *      - Loads indexed data
     */
    void finish_rr_generation() {
        for (t_rr_type rr_type : RR_TYPES)
            if (rr_type == CHANX)
                device_ctx_.rr_graph_builder.node_lookup().resize_nodes(grid_.height(), grid_.width(), rr_type, NUM_SIDES);
            else
                device_ctx_.rr_graph_builder.node_lookup().resize_nodes(grid_.width(), grid_.height(), rr_type, NUM_SIDES);

        for (size_t node = 0; node < rr_nodes_.size(); node++) {
            auto rr_node = rr_nodes_[node];
            device_ctx_.rr_graph_builder.add_node_to_all_locs(rr_node.id());
        }

        device_ctx_.rr_graph_builder.init_fan_in();

        // Creates a temporary copy to convert from vtr::vector to sitd::vector.
        //
        // This is required because the ``alloc_and_load_rr_indexed_data()`` function supports only std::vector data
        // type for ``rr_segments``
        // Note that this is a dirty fix (to avoid massive code changes)
        // TODO: The ``alloc_and_load_rr_indexed_data()`` function should embrace ``vtr::vector`` for ``rr_segments``
        std::vector<t_segment_inf> temp_rr_segs;
        temp_rr_segs.reserve(segment_inf_.size());
        for (auto& rr_seg : segment_inf_) {
            temp_rr_segs.push_back(rr_seg);
        }

        alloc_and_load_rr_indexed_data(
            temp_rr_segs,
            0, //we connect ipins to tracks with shorts
            base_cost_type_);

        VTR_ASSERT(device_ctx_.rr_indexed_data.size() == seg_index_.size());

        for (size_t i = 0; i < seg_index_.size(); ++i)
            device_ctx_.rr_indexed_data[RRIndexedDataId(i)].seg_index = seg_index_[RRIndexedDataId(i)];
    }
};

void build_rr_graph_fpga_interchange(const t_graph_type graph_type,
                                     const DeviceGrid& grid,
                                     const std::vector<t_segment_inf>& segment_inf,
                                     const enum e_base_cost_type base_cost_type,
                                     int* wire_to_rr_ipin_switch,
                                     std::string& read_rr_graph_filename,
                                     bool do_check_rr_graph) {
    vtr::ScopedStartFinishTimer timer("Building RR Graph from device database");

    auto& device_ctx = g_vpr_ctx.mutable_device();
    size_t num_segments = segment_inf.size();

    auto& rr_segments = device_ctx.rr_graph_builder.rr_segments();
    rr_segments.reserve(num_segments);
    for (long unsigned int iseg = 0; iseg < num_segments; ++iseg) {
        rr_segments.push_back(segment_inf[(iseg)]);
    }

    // Decompress GZipped capnproto device file
    gzFile file = gzopen(get_arch_file_name(), "r");
    VTR_ASSERT(file != Z_NULL);

    std::vector<uint8_t> output_data;
    output_data.resize(4096);
    std::stringstream sstream(std::ios_base::in | std::ios_base::out | std::ios_base::binary);
    while (true) {
        int ret = gzread(file, output_data.data(), output_data.size());
        VTR_ASSERT(ret >= 0);
        if (ret > 0) {
            sstream.write((const char*)output_data.data(), ret);
            VTR_ASSERT(sstream);
        } else {
            VTR_ASSERT(ret == 0);
            int error;
            gzerror(file, &error);
            VTR_ASSERT(error == Z_OK);
            break;
        }
    }

    VTR_ASSERT(gzclose(file) == Z_OK);

    sstream.seekg(0);
    kj::std::StdInputStream istream(sstream);

    // Reader options
    capnp::ReaderOptions reader_options;
    reader_options.nestingLimit = std::numeric_limits<int>::max();
    reader_options.traversalLimitInWords = std::numeric_limits<uint64_t>::max();

    capnp::InputStreamMessageReader message_reader(istream, reader_options);

    auto device_reader = message_reader.getRoot<DeviceResources::Device>();

    InterchangeRRGraphBuilder builder(
        device_reader,
        grid,
        device_ctx,
        device_ctx.rr_graph_builder.rr_nodes(),
        device_ctx.rr_graph.rr_segments(),
        device_ctx.physical_tile_types,
        base_cost_type);
    builder.build_rr_graph();
    *wire_to_rr_ipin_switch = 0;
    device_ctx.read_rr_graph_filename.assign(get_arch_file_name());
    read_rr_graph_filename.assign(get_arch_file_name());

    if (do_check_rr_graph) {
        check_rr_graph(graph_type, grid, device_ctx.physical_tile_types);
    }
}
