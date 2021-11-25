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

using namespace DeviceResources;
using namespace LogicalNetlist;
using namespace capnp;

typedef std::tuple<int, int> location;

/*
 * Intermediate data type.
 * It is used to build and explore FPGA Interchange
 * nodes graph and later convert them to rr_nodes.
 */
struct node_ {
  public:
    location loc;
    int segment_id;
    node_* links[4]; //left, up, right, down
    node_* visited;
    bool on_route;
    bool has_pins;
    node_() = delete;
    node_(int x, int y)
        : loc{x, y} {
        links[0] = links[1] = links[2] = links[3] = nullptr;
        visited = nullptr;
        on_route = false;
        has_pins = false;
    }
};

/*
 * Main class for FPGA Interchange device to rr_graph flow
 */
struct RR_Graph_Builder {
  public:
    RR_Graph_Builder(Device::Reader& arch_reader,
                     const DeviceGrid& grid,
                     DeviceContext& device_ctx,
                     const std::vector<t_physical_tile_type>& physical_tile_types,
                     const vtr::vector<RRSegmentId, t_segment_inf>& segment_inf,
                     const enum e_base_cost_type base_cost_type)
        : ar_(arch_reader)
        , device_ctx_(device_ctx)
        , segment_inf_(segment_inf)
        , grid_(grid)
        , base_cost_type_(base_cost_type)
        , empty_(device_ctx.arch->strings.intern_string(vtr::string_view(""))) {
        for (auto& type : physical_tile_types) {
            tile_type_to_pb_type_[std::string(type.name)] = &type;
        }

        for (size_t i = 0; i < segment_inf.size(); ++i) {
            wire_name_to_seg_idx_[segment_inf_[RRSegmentId(i)].name] = i;
        }

        for (size_t i = 1; i < grid.width() - 1; ++i) {
            for (size_t j = 1; j < grid.height() - 1; ++j) {
                location loc{i, j};
                if (grid[i][j].type->index == 0)
                    empty_tiles.insert(loc);
            }
        }
    }

    void build_rr_graph() {
        create_switch_inf();
        create_tile_id_to_loc();
        create_uniq_node_ids();
        create_sink_source_nodes();
        create_pips_();
        process_nodes();
        pack_to_rr_graph();
#ifdef DEBUG
        print_();
#endif
        finish_rr_generation();
    }

  private:
    Device::Reader& ar_;
    DeviceContext& device_ctx_;
    const vtr::vector<RRSegmentId, t_segment_inf>& segment_inf_;
    const DeviceGrid& grid_;
    const enum e_base_cost_type base_cost_type_;

    vtr::interned_string empty_;

    std::unordered_map<std::string, const t_physical_tile_type*> tile_type_to_pb_type_;

    std::unordered_map<std::string, int> wire_name_to_seg_idx_;

    std::set<location> empty_tiles;

    std::unordered_map<int /*tile_id(name)*/, location> tile_to_loc_;
    std::map<location, int /*tile_id(name)*/> loc_to_tile_;

    std::map<std::tuple<int /*timing_model*/, bool /*buffered*/>, int /*idx*/> pips_models_;
    std::map<std::tuple<int /*node_id*/, int /*node_id*/>, std::tuple<int /*switch_id*/, std::tuple<int /*tile_id*/, int /*wire0*/, int /*wire1*/, bool /*forward*/>>> pips_;

    std::map<std::tuple<int, int>, int> wire_to_node_;
    std::unordered_map<int, std::set<location>> node_to_locs_;
    std::map<std::tuple<int, int> /*<node, tile_id>*/, int /*segment type*/> node_tile_to_segment_;

    /*
     * Offsets for FPGA Interchange node processing
     */
    location offsets[4] = {location(-1, 0), location(0, 1), location(1, 0), location(0, -1)};

    /*
     * Intermediate data storage.
     * - Shorts represent connections between rr_nodes in a single FPGA Interchange node.
     * - Redirect is map from node_id at location to location channel and index of track in that channel.
     * It's useful for ends of the nodes (FPGA Interchange) that don't have representation in CHANS.
     * - CHAN_loc_map maps from location and CHAN to vector containing tracks description.
     */
    std::vector<std::tuple<location, e_rr_type, int /*idx*/, location, e_rr_type, int /*idx*/>> shorts_;
    std::map<std::tuple<int /*node_id*/, location>, std::tuple<location, e_rr_type /*CHANX/CHANY*/, int /*idx*/>> redirect_;
    std::map<std::tuple<location, e_rr_type /*CHANX/CHANY*/>, std::vector<std::tuple<int, float, float>> /*idx = ptc,<seg_idx, R, C>*/> chan_loc_map_;

    /*
     * Sets contain tuples of node ids and location.
     * Each value <n,l> correspondence to node id n being used by either pip or pin at location l.
     */
    std::set<std::tuple<int /*node_id*/, location>> used_by_pip_;
    std::set<std::tuple<int /*node_id*/, location>> used_by_pin_;

    /* Sink_source_loc_map is used to create ink/source and ipin/opin rr_nodes,
     * rr_edges from sink/source to ipin/opin and from ipin/opin to their coresponding segments
     * uniq_id is used to create edge from segment to ipin/opin, edges from ipin/opin to sink/source are created simply as one to one
     */
    std::unordered_map<int /*tile_id*/, std::vector<std::tuple<bool, float, int>> /*idx = ptc,<type, R/C, node_id>*/> sink_source_loc_map_;

    /*
     * RR generation stuff
     */
    vtr::vector<RRIndexedDataId, short> seg_index_;
    std::map<std::tuple<location, e_rr_type, int>, int> loc_type_idx_to_rr_idx_;
    int rr_idx = 0; // Do not decrement!

    std::string str(int idx) {
        return std::string(ar_.getStrList()[idx].cStr());
    }

    /*
     * Debug print function
     */
    void print_() {
        for (const auto& entry : sink_source_loc_map_) {
            const auto& key = entry.first;
            const auto& value = entry.second;
            int x, y;
            std::tie(x, y) = tile_to_loc_[key];
            VTR_LOG("Location x:%d y:%d Name:%s\n", x, y, str(key).c_str());
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

        for (auto const& switch_id : device_ctx_.rr_switch_inf) {
            VTR_LOG("Switch: %d Name:%s\n", it++, switch_id.name);
        }

        for (auto& entry : chan_loc_map_) {
            location loc;
            e_rr_type type;
            std::tie(loc, type) = entry.first;
            VTR_LOG("CHAN%c X:%d Y:%d\n", type == e_rr_type::CHANX ? 'X' : 'Y', std::get<0>(loc), std::get<1>(loc));
            for (auto& seg : entry.second) {
                VTR_LOG("\tSegment id:%d name:%s\n", std::get<0>(seg), segment_inf_[RRSegmentId(std::get<0>(seg))].name.c_str());
            }
        }

        VTR_LOG("Redirects:\n");
        for (auto& entry : redirect_) {
            int node;
            location loc;
            std::tie(node, loc) = entry.first;

            location new_loc;
            e_rr_type new_type;
            int new_idx;
            std::tie(new_loc, new_type, new_idx) = entry.second;
            VTR_LOG("\t Node:%d X:%d Y:%d -> X:%d Y:%d CHAN%c idx:%d\n", node, std::get<0>(loc), std::get<1>(loc), std::get<0>(new_loc),
                    std::get<1>(new_loc), new_type == e_rr_type::CHANX ? 'X' : 'Y',
                    new_idx);
        }

        VTR_LOG("Shorts:\n");
        for (auto& entry : shorts_) {
            location loc1, loc2;
            e_rr_type type1, type2;
            int id1, id2;
            std::tie(loc1, type1, id1, loc2, type2, id2) = entry;
            VTR_LOG("\tCHAN%c X:%d Y%d", type1 == e_rr_type::CHANX ? 'X' : 'Y', std::get<0>(loc1), std::get<1>(loc1));
            VTR_LOG(" Segment id:%d name:%s ->", id1,
                    segment_inf_[RRSegmentId(std::get<0>(chan_loc_map_[std::make_tuple(loc1, type1)][id1]))].name.c_str());
            VTR_LOG(" CHAN%c X:%d Y%d", type2 == e_rr_type::CHANX ? 'X' : 'Y', std::get<0>(loc2), std::get<1>(loc2));
            VTR_LOG(" Segment id:%d name:%s\n", id2,
                    segment_inf_[RRSegmentId(std::get<0>(chan_loc_map_[std::make_tuple(loc2, type2)][id2]))].name.c_str());
        }

        for (auto& pip : pips_) {
            auto& key = pip.first;
            auto& value = pip.second;
            std::tuple<int, int, int, bool> meta;
            int switch_id, tile, wire0, wire1;
            bool forward;
            std::tie(switch_id, meta) = value;
            std::tie(tile, wire0, wire1, forward) = meta;
            auto& r1 = redirect_[std::make_tuple(std::get<0>(key), tile_to_loc_[tile])];
            auto& r2 = redirect_[std::make_tuple(std::get<1>(key), tile_to_loc_[tile])];
            VTR_LOG("Switch_type: %d, %s.%s %s.%s: forward:%s\n",
                    switch_id, str(tile).c_str(), str(wire0).c_str(), str(tile).c_str(), str(wire1).c_str(), forward ? "yes" : "no");
            VTR_LOG("Edge: CHAN%c X:%d Y:%d idx:%d -> CHAN%c X:%d Y:%d idx%d\n",
                    std::get<1>(r1) == e_rr_type::CHANX ? 'X' : 'Y',
                    std::get<0>(std::get<0>(r1)),
                    std::get<1>(std::get<0>(r1)),
                    std::get<2>(r1),
                    std::get<1>(r2) == e_rr_type::CHANX ? 'X' : 'Y',
                    std::get<0>(std::get<0>(r2)),
                    std::get<1>(std::get<0>(r2)),
                    std::get<2>(r2));
        }
        VTR_LOG("RR_Nodes\n");
        for (auto i : loc_type_idx_to_rr_idx_) {
            location loc;
            e_rr_type type;
            int idx;
            std::tie(loc, type, idx) = i.first;
            VTR_LOG("\t X:%d Y:%d type:%d idx:%d->rr_node:%d\n", std::get<0>(loc), std::get<1>(loc), type, idx, i.second);
        }
    }

    /*
     * Fill device_ctx rr_switch_inf structure and store id of each PIP type for future use
     */
    void create_switch_inf() {
        std::set<std::tuple<int /*timing*/, bool /*buffered*/>> seen;
        for (const auto& tile : ar_.getTileTypeList()) {
            for (const auto& pip : tile.getPips()) {
                if (pip.isPseudoCells())
                    continue;
                seen.emplace(pip.getTiming(), pip.getBuffered21());
                if (!pip.getDirectional()) {
                    seen.emplace(pip.getTiming(), pip.getBuffered20());
                }
            }
        }
        device_ctx_.rr_switch_inf.reserve(seen.size() + 1);
        int id = 2;
        make_room_in_vector(&device_ctx_.rr_switch_inf, 0);
        device_ctx_.rr_switch_inf[0].R = 0;
        device_ctx_.rr_switch_inf[0].Cin = 0;
        device_ctx_.rr_switch_inf[0].Cout = 0;
        device_ctx_.rr_switch_inf[0].Cinternal = 0;
        device_ctx_.rr_switch_inf[0].Tdel = 0;
        device_ctx_.rr_switch_inf[0].buf_size = 0;
        device_ctx_.rr_switch_inf[0].mux_trans_size = 0;
        device_ctx_.rr_switch_inf[0].name = "short";
        device_ctx_.rr_switch_inf[0].set_type(SwitchType::SHORT);
        make_room_in_vector(&device_ctx_.rr_switch_inf, 1);
        device_ctx_.rr_switch_inf[1].R = 0;
        device_ctx_.rr_switch_inf[1].Cin = 0;
        device_ctx_.rr_switch_inf[1].Cout = 0;
        device_ctx_.rr_switch_inf[1].Cinternal = 0;
        device_ctx_.rr_switch_inf[1].Tdel = 0;
        device_ctx_.rr_switch_inf[1].buf_size = 0;
        device_ctx_.rr_switch_inf[1].mux_trans_size = 0;
        device_ctx_.rr_switch_inf[1].name = "generic";
        device_ctx_.rr_switch_inf[1].set_type(SwitchType::MUX);
        const auto& pip_models = ar_.getPipTimings();
        float R, Cin, Cout, Cint, Tdel;
        std::string switch_name;
        SwitchType type;
        for (const auto& value : seen) {
            make_room_in_vector(&device_ctx_.rr_switch_inf, id);
            int timing_model_id;
            bool buffered;
            std::tie(timing_model_id, buffered) = value;
            const auto& model = pip_models[timing_model_id];
            pips_models_[std::make_tuple(timing_model_id, buffered)] = id;

            R = Cin = Cint = Cout = Tdel = 0.0;
            std::stringstream name;
            std::string mux_type_string = buffered ? "mux_" : "passGate_";
            name << mux_type_string;

            R = get_corner_value(model.getOutputResistance(), "slow", "min");
            name << "R" << std::scientific << R;

            Cin = get_corner_value(model.getInputCapacitance(), "slow", "min");
            name << "Cin" << std::scientific << Cin;

            Cout = get_corner_value(model.getOutputCapacitance(), "slow", "min");
            name << "Cout" << std::scientific << Cout;

            if (buffered) {
                Cint = get_corner_value(model.getInternalCapacitance(), "slow", "min");
                name << "Cinternal" << std::scientific << Cint;
            }

            Tdel = get_corner_value(model.getInternalDelay(), "slow", "min");
            name << "Tdel" << std::scientific << Tdel;

            switch_name = name.str();
            type = buffered ? SwitchType::MUX : SwitchType::PASS_GATE;

            auto& as = device_ctx_.rr_switch_inf[id];
            as.name = vtr::strdup(switch_name.c_str());
            as.set_type(type);
            as.mux_trans_size = type == SwitchType::MUX ? 1 : 0;

            as.R = R;
            as.Cin = Cin;
            as.Cout = Cout;
            as.Cinternal = Cint;
            as.Tdel = Tdel;
            as.buf_size = 0.;

            id++;
        }
    }

    /*
     * Create mapping form tile_id to its location
     */
    void create_tile_id_to_loc() {
        for (const auto& tile : ar_.getTileList()) {
            tile_to_loc_[tile.getName()] = location{tile.getCol() + 1, tile.getRow() + 1};
            loc_to_tile_[location(tile.getCol() + 1, tile.getRow() + 1)] = tile.getName();
        }
    }

    /*
     * Create uniq id for each FPGA Interchange node.
     * Create mapping from wire to node id and from node id to its locations
     * These ids are used later for site pins and pip conections (rr_edges)
     */
    void create_uniq_node_ids() {
        int id = 0;
        for (const auto& node : ar_.getNodes()) {
            for (const auto& wire_id_ : node.getWires()) {
                const auto& wire = ar_.getWires()[wire_id_];
                int tile_id = wire.getTile();
                int wire_id = wire.getWire();
                wire_to_node_[std::make_tuple(tile_id, wire_id)] = id;
                node_to_locs_[id].insert(tile_to_loc_[tile_id]);
                if (wire_name_to_seg_idx_.find(str(wire_id)) == wire_name_to_seg_idx_.end())
                    wire_name_to_seg_idx_[str(wire_id)] = -1;
                node_tile_to_segment_[std::make_tuple(id, tile_id)] = wire_name_to_seg_idx_[str(wire_id)];
            }
            id++;
        }
    }

    /*
     * Create entry for each pip in device architecture,
     * also store meta data related to that pip such as: tile_name, wire names, and its direction.
     */
    void create_pips_() {
        for (const auto& tile : ar_.getTileList()) {
            int name = tile.getName();
            location loc = tile_to_loc_[name];
            const auto& tile_type = ar_.getTileTypeList()[tile.getType()];

            for (const auto& pip : tile_type.getPips()) {
                if (pip.isPseudoCells())
                    continue;
                if (tile.hasSubTilesPrefices())
                    name = tile.getSubTilesPrefices()[pip.getSubTile()];
                int wire0_name, wire1_name;
                int node0, node1;
                int switch_id;

                wire0_name = tile_type.getWires()[pip.getWire0()];
                wire1_name = tile_type.getWires()[pip.getWire1()];
                if (wire_to_node_.find(std::make_tuple(name, wire0_name)) == wire_to_node_.end())
                    continue;
                if (wire_to_node_.find(std::make_tuple(name, wire1_name)) == wire_to_node_.end())
                    continue;
                node0 = wire_to_node_[std::make_tuple(name, wire0_name)];
                node1 = wire_to_node_[std::make_tuple(name, wire1_name)];

                used_by_pip_.emplace(node0, loc);
                used_by_pip_.emplace(node1, loc);
                switch_id = pips_models_[std::make_tuple(pip.getTiming(), pip.getBuffered21())];
                std::tuple<int, int> source_sink;
                source_sink = std::make_tuple(node0, node1);
                pips_.emplace(source_sink, std::make_tuple(switch_id, std::make_tuple(name, wire0_name, wire1_name, true)));
                if (!pip.getBuffered21() && (pip.getDirectional() || pip.getBuffered20())) {
                    source_sink = std::make_tuple(node1, node0);
                    pips_.emplace(source_sink, std::make_tuple(switch_id, std::make_tuple(name, wire0_name, wire1_name, true)));
                }
                if (!pip.getDirectional()) {
                    switch_id = pips_models_[std::make_tuple(pip.getTiming(), pip.getBuffered20())];
                    source_sink = std::make_tuple(node1, node0);
                    pips_.emplace(source_sink, std::make_tuple(switch_id, std::make_tuple(name, wire0_name, wire1_name, true)));
                    if (!pip.getBuffered20() && pip.getBuffered21()) {
                        source_sink = std::make_tuple(node0, node1);
                        pips_.emplace(source_sink, std::make_tuple(switch_id, std::make_tuple(name, wire0_name, wire1_name, true)));
                    }
                }
            }
        }
    }

    /*
     * Build graph of FPGA Interchange node for further computations
     */
    node_* build_node_graph(int node_id,
                            std::set<location> nodes,
                            std::set<location> empties,
                            int root_x,
                            int root_y) {
        int visited_tiles = 0;
        node_* root_node = new node_(root_x, root_y);
        location key{root_x, root_y};

        root_node->segment_id = node_tile_to_segment_[std::make_tuple(node_id, loc_to_tile_[key])];
        nodes.erase(key);

        std::map<location, node_*> existing_nodes;
        existing_nodes.emplace(key, root_node);

        std::priority_queue<std::tuple<int, int, node_*>> builder;
        builder.emplace(1, 0, root_node);
        while (!builder.empty() && (!nodes.empty() || std::get<0>(builder.top()) != 0)) {
            node_* vertex;
            std::tie(std::ignore, std::ignore, vertex) = builder.top();
            builder.pop();
            location loc = vertex->loc;
            for (int i = 0; i < 4; ++i) {
                location offset = offsets[i];
                location other_loc{std::get<0>(loc) + std::get<0>(offset), std::get<1>(loc) + std::get<1>(offset)};
                node_* temp = nullptr;
                if (existing_nodes.find(other_loc) != existing_nodes.end()) {
                    temp = existing_nodes[other_loc];
                } else if (nodes.find(other_loc) != nodes.end()) {
                    temp = new node_(std::get<0>(other_loc), std::get<1>(other_loc));
                    temp->segment_id = node_tile_to_segment_[std::make_tuple(node_id, loc_to_tile_[other_loc])];
                    nodes.erase(other_loc);
                    existing_nodes.emplace(other_loc, temp);
                    builder.emplace(1, -1 * visited_tiles, temp);
                    visited_tiles++;
                } else if (empties.find(other_loc) != empties.end()) {
                    temp = new node_(std::get<0>(other_loc), std::get<1>(other_loc));
                    temp->segment_id = -1;
                    empties.erase(other_loc);
                    existing_nodes.emplace(other_loc, temp);
                    builder.emplace(0, -1 * visited_tiles, temp);
                    visited_tiles++;
                }
                vertex->links[i] = temp;
            }
        }

        if (!nodes.empty())
            vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), __LINE__, "Node graph could not be created\n");

        while (!builder.empty()) {
            node_* vertex;
            std::tie(std::ignore, std::ignore, vertex) = builder.top();
            builder.pop();
            location loc = vertex->loc;
            for (int i = 0; i < 4; ++i) {
                location offset = offsets[i];
                location other_loc{std::get<0>(loc) + std::get<0>(offset), std::get<1>(loc) + std::get<1>(offset)};
                node_* temp = nullptr;
                if (existing_nodes.find(other_loc) != existing_nodes.end()) {
                    temp = existing_nodes[other_loc];
                    temp->links[(i + 2) % 4] = nullptr;
                }
            }
            existing_nodes.erase(vertex->loc);
            delete (vertex);
        }

        for (auto& node : existing_nodes) {
            if (used_by_pip_.find(std::make_tuple(node_id, node.second->loc)) != used_by_pip_.end()) {
                node.second->on_route = true;
                root_node = node.second;
            }
            if (used_by_pin_.find(std::make_tuple(node_id, node.second->loc)) != used_by_pin_.end()) {
                node.second->has_pins = true;
                node.second->on_route = true;
                root_node = node.second;
            }
        }
        return root_node;
    }

    /*
     * Removes dangling nodes from a graph represented by the root node.
     * Dangling nodes are nodes that do not connect to a pin, a pip or other non-dangling node.
     */
    int reduce_graph_and_count_nodes(node_* root) {
        int cnt = 0;
        std::queue<node_*> walker;
        std::stack<node_*> back_walker;
        walker.push(root);
        root->visited = root;
        bool all_nulls;
        while (!walker.empty()) {
            all_nulls = true;
            node_* vertex = walker.front();
            walker.pop();
            back_walker.emplace(vertex);
            for (int i = 0; i < 4; ++i) {
                if (vertex->links[i] != nullptr) {
                    all_nulls = false;
                    if (i == 0 || i == 1)
                        cnt++;
                    if (vertex->links[i]->visited == nullptr) {
                        vertex->links[i]->visited = vertex;
                        walker.emplace(vertex->links[i]);
                    }
                }
            }
            cnt += all_nulls;
        }
        while (!back_walker.empty()) {
            node_* vertex = back_walker.top();
            back_walker.pop();
            if (!vertex->has_pins && !vertex->on_route) {
                if (vertex->visited->segment_id == -1) {
                    vertex->visited->segment_id = vertex->segment_id;
                }
                for (int i = 0; i < 4; ++i) {
                    node_* temp = nullptr;
                    if (vertex->links[i] != nullptr) {
                        if (i == 2 || i == 3)
                            cnt--;
                        temp = vertex->links[i];
                        temp->links[(i + 2) % 4] = nullptr;
                    }
                }
                delete (vertex);
            } else if (vertex->on_route) {
                vertex->visited->on_route = true;
                if (vertex->visited->segment_id == -1)
                    vertex->visited->segment_id = vertex->segment_id;
            }
        }
        return cnt;
    }

    void populate_chan_loc_map(int node_id, node_* root, float R, float C) {
        /*
         * Create future rr_nodes that corespond to given FPGA Interchange node
         */
        std::map<std::tuple<location, e_rr_type /*CHANX/CHANY*/>, int /*index*/> loc_chan_idx;

        std::queue<node_*> walker;
        std::stack<node_*> back_walker;
        walker.emplace(root);
        bool all_nulls, is_chanx, is_chany;
        while (!walker.empty()) {
            node_* vertex = walker.front();
            walker.pop();
            back_walker.emplace(vertex);
            is_chany = false;
            is_chanx = vertex->has_pins;
            all_nulls = true;
            for (int i = 0; i < 4; ++i) {
                if (vertex->links[i] != nullptr) {
                    all_nulls = false;
                    if (vertex->links[i]->visited == vertex || vertex->links[i] == vertex->visited) {
                        is_chanx = i == 0 ? true : is_chanx;
                        is_chany = i == 1 ? true : is_chany;
                    }
                    if (vertex->links[i]->visited == vertex) {
                        walker.emplace(vertex->links[i]);
                    }
                }
            }
            is_chanx = all_nulls ? all_nulls : is_chanx;
            std::tuple<location, e_rr_type> key1(vertex->loc, e_rr_type::CHANY);
            std::tuple<location, e_rr_type> key2(location(std::get<0>(vertex->loc), std::get<1>(vertex->loc) - 1), e_rr_type::CHANX);

            if (is_chany) {
                loc_chan_idx[key1] = chan_loc_map_[key1].size();
                chan_loc_map_[key1].emplace_back(vertex->segment_id, R, C);
            }
            if (is_chanx) {
                loc_chan_idx[key2] = chan_loc_map_[key2].size();
                chan_loc_map_[key2].emplace_back(vertex->segment_id, R, C);
            }
            if (is_chanx && is_chany) {
                shorts_.emplace_back(std::get<0>(key1), std::get<1>(key1), loc_chan_idx[key1],
                                     std::get<0>(key2), std::get<1>(key2), loc_chan_idx[key2]);
            }
        }

        /*
         * Create edges between future rr_nodes that correspond to the given FPGA Interchange node
         */
        while (!back_walker.empty()) {
            node_* vertex = back_walker.top();
            back_walker.pop();
            location loc = vertex->loc;
            location down(std::get<0>(vertex->loc), std::get<1>(vertex->loc) - 1); // lower tile CHANY, our CHANX
            location right(std::get<0>(down) + 1, std::get<1>(down));              // CHANX of tile to the right

            if (loc_chan_idx.find(std::make_tuple(down, e_rr_type::CHANX)) != loc_chan_idx.end()) {
                redirect_.emplace(std::make_tuple(node_id, loc), std::make_tuple(
                                                                     down, e_rr_type::CHANX, loc_chan_idx[std::make_tuple(down, e_rr_type::CHANX)]));

            } else if (loc_chan_idx.find(std::make_tuple(loc, e_rr_type::CHANY)) != loc_chan_idx.end()) {
                redirect_.emplace(std::make_tuple(node_id, loc), std::make_tuple(
                                                                     loc, e_rr_type::CHANY, loc_chan_idx[std::make_tuple(loc, e_rr_type::CHANY)]));

            } else if (vertex->links[2] != nullptr && (vertex->links[2]->visited == vertex || vertex->links[2] == vertex->visited) && loc_chan_idx.find(std::make_tuple(right, e_rr_type::CHANX)) != loc_chan_idx.end()) {
                redirect_.emplace(std::make_tuple(node_id, loc), std::make_tuple(
                                                                     right, e_rr_type::CHANX, loc_chan_idx[std::make_tuple(right, e_rr_type::CHANX)]));

            } else if (vertex->links[3] != nullptr && (vertex->links[3]->visited == vertex || vertex->links[3] == vertex->visited) && loc_chan_idx.find(std::make_tuple(down, e_rr_type::CHANY)) != loc_chan_idx.end()) {
                redirect_.emplace(std::make_tuple(node_id, loc), std::make_tuple(
                                                                     down, e_rr_type::CHANY, loc_chan_idx[std::make_tuple(down, e_rr_type::CHANY)]));
            }

            /* Check if node x is up-right corner:
             * +-+-+
             * |x|b|
             * +-+-+
             * |a|
             * +-+
             * If so it does not use CHANX or CHANY, and it must connect CHANY from a with CHANX from b
             */

            if (vertex->links[2] != nullptr && vertex->links[3] != nullptr && (vertex->links[3]->visited == vertex || vertex->links[3] == vertex->visited) && (vertex->links[2]->visited == vertex || vertex->links[2] == vertex->visited) && loc_chan_idx.find(std::make_tuple(down, e_rr_type::CHANX)) == loc_chan_idx.end() && loc_chan_idx.find(std::make_tuple(loc, e_rr_type::CHANY)) == loc_chan_idx.end()) {
                shorts_.emplace_back(down, e_rr_type::CHANY, loc_chan_idx[std::make_tuple(down, e_rr_type::CHANY)],
                                     right, e_rr_type::CHANX, loc_chan_idx[std::make_tuple(right, e_rr_type::CHANX)]);
                continue;
            }

            /* If tile has CHANX, try to cannect it to CHANY of tile below,
             * CHANY and CHANX of tile to the left and CHANX of tile to the right
             */
            if (loc_chan_idx.find(std::make_tuple(down, e_rr_type::CHANX)) != loc_chan_idx.end()) {
                if (vertex->links[0] != nullptr && vertex->links[0]->visited == vertex) {
                    location left(std::get<0>(down) - 1, std::get<1>(down));  // CHANX
                    location left_up(std::get<0>(loc) - 1, std::get<1>(loc)); // CHANY
                    if (loc_chan_idx.find(std::make_tuple(left, e_rr_type::CHANX)) != loc_chan_idx.end())
                        shorts_.emplace_back(left, e_rr_type::CHANX, loc_chan_idx[std::make_tuple(left, e_rr_type::CHANX)],
                                             down, e_rr_type::CHANX, loc_chan_idx[std::make_tuple(down, e_rr_type::CHANX)]);
                    if (loc_chan_idx.find(std::make_tuple(left_up, e_rr_type::CHANY)) != loc_chan_idx.end())
                        shorts_.emplace_back(left_up, e_rr_type::CHANY, loc_chan_idx[std::make_tuple(left_up, e_rr_type::CHANY)],
                                             down, e_rr_type::CHANX, loc_chan_idx[std::make_tuple(down, e_rr_type::CHANX)]);
                }
                if (vertex->links[2] != nullptr && vertex->links[2]->visited == vertex) {
                    if (loc_chan_idx.find(std::make_tuple(right, e_rr_type::CHANX)) != loc_chan_idx.end())
                        shorts_.emplace_back(right, e_rr_type::CHANX, loc_chan_idx[std::make_tuple(right, e_rr_type::CHANX)],
                                             down, e_rr_type::CHANX, loc_chan_idx[std::make_tuple(down, e_rr_type::CHANX)]);
                }
                if (vertex->links[3] != nullptr && vertex->links[3]->visited == vertex) {
                    if (loc_chan_idx.find(std::make_tuple(down, e_rr_type::CHANY)) != loc_chan_idx.end())
                        shorts_.emplace_back(down, e_rr_type::CHANX, loc_chan_idx[std::make_tuple(down, e_rr_type::CHANX)],
                                             down, e_rr_type::CHANY, loc_chan_idx[std::make_tuple(down, e_rr_type::CHANY)]);
                }
            }

            /* If tile has CHANY, try to cannect it to CHANY of tile below,
             * CHANY and CHANX of tile above and CHANX of tile to the right
             */
            if (loc_chan_idx.find(std::make_tuple(loc, e_rr_type::CHANY)) != loc_chan_idx.end()) {
                if (vertex->links[1] != nullptr && vertex->links[1]->visited == vertex) {
                    location up(std::get<0>(down), std::get<1>(down) + 1);  // CHANX
                    location up_up(std::get<0>(loc), std::get<1>(loc) + 1); // CHANY
                    if (loc_chan_idx.find(std::make_tuple(up, e_rr_type::CHANX)) != loc_chan_idx.end())
                        shorts_.emplace_back(up, e_rr_type::CHANX, loc_chan_idx[std::make_tuple(up, e_rr_type::CHANX)],
                                             loc, e_rr_type::CHANY, loc_chan_idx[std::make_tuple(loc, e_rr_type::CHANY)]);
                    if (loc_chan_idx.find(std::make_tuple(up_up, e_rr_type::CHANY)) != loc_chan_idx.end())
                        shorts_.emplace_back(up_up, e_rr_type::CHANY, loc_chan_idx[std::make_tuple(up_up, e_rr_type::CHANY)],
                                             loc, e_rr_type::CHANY, loc_chan_idx[std::make_tuple(loc, e_rr_type::CHANY)]);
                }
                if (vertex->links[2] != nullptr && vertex->links[2]->visited == vertex) {
                    if (loc_chan_idx.find(std::make_tuple(right, e_rr_type::CHANX)) != loc_chan_idx.end())
                        shorts_.emplace_back(right, e_rr_type::CHANX, loc_chan_idx[std::make_tuple(right, e_rr_type::CHANX)],
                                             loc, e_rr_type::CHANY, loc_chan_idx[std::make_tuple(loc, e_rr_type::CHANY)]);
                }
                if (vertex->links[3] != nullptr && vertex->links[3]->visited == vertex) {
                    if (loc_chan_idx.find(std::make_tuple(down, e_rr_type::CHANY)) != loc_chan_idx.end())
                        shorts_.emplace_back(down, e_rr_type::CHANY, loc_chan_idx[std::make_tuple(down, e_rr_type::CHANY)],
                                             loc, e_rr_type::CHANY, loc_chan_idx[std::make_tuple(loc, e_rr_type::CHANY)]);
                }
            }
        }
    }

    /*
     * Remove a graph represented by a root node.
     * Clean up of earlier stages
     */
    void delete_nodes(node_* root) {
        std::queue<node_*> walker;
        std::stack<node_*> back_walker;
        walker.emplace(root);
        while (!walker.empty()) {
            node_* vertex = walker.front();
            walker.pop();
            back_walker.emplace(vertex);
            for (int i = 0; i < 4; ++i) {
                if (vertex->links[i] != nullptr) {
                    if (vertex->links[i]->visited == vertex) {
                        walker.emplace(vertex->links[i]);
                    }
                }
            }
        }
        while (!back_walker.empty()) {
            node_* vertex = back_walker.top();
            back_walker.pop();
            for (int i = 0; i < 4; ++i) {
                node_* temp = nullptr;
                if (vertex->links[i] != nullptr) {
                    temp = vertex->links[i];
                    temp->links[(i + 2) % 4] = nullptr;
                }
            }
            delete (vertex);
        }
    }

    /*
     * Process FPGA Interchange nodes
     */
    void process_nodes() {
        auto wires = ar_.getWires();
        for (auto const& node : ar_.getNodes()) {
            std::tuple<int, int> base_wire_{wires[node.getWires()[0]].getTile(), wires[node.getWires()[0]].getWire()};
            int node_id = wire_to_node_[base_wire_];
            std::set<location> all_possible_tiles = node_to_locs_[node_id];
            int root_x, root_y;
            std::tie(root_x, root_y) = *all_possible_tiles.begin();
            std::set<location> empties;
            if (all_possible_tiles.size() != 1) {
                for (auto const& empty_tile : empty_tiles) {
                    empties.insert(empty_tile);
                }
            }

            node_* root = build_node_graph(node_id, all_possible_tiles, empties, root_x, root_y);
            int div = reduce_graph_and_count_nodes(root);
            float capacitance, resistance;
            auto model = ar_.getNodeTimings()[node.getNodeTiming()];
            capacitance = get_corner_value(model.getCapacitance(), "slow", "typ") / div;
            resistance = get_corner_value(model.getResistance(), "slow", "typ") / div;
            populate_chan_loc_map(node_id, root, resistance, capacitance);
            delete_nodes(root);
        }
    }

    /*
     * Create site sink/source ipin/opin mappins.
     */
    void create_sink_source_nodes() {
        const auto& tile_types = ar_.getTileTypeList();
        std::map<std::tuple<int /*tile type id*/, int, std::string>, std::tuple<float, int>> tile_type_site_num_pin_name_model;
        for (const auto& tileType : tile_types) {
            int it = 0;
            if (tile_type_to_pb_type_.find(str(tileType.getName())) == tile_type_to_pb_type_.end())
                continue;
            for (const auto& siteType : tileType.getSiteTypes()) {
                int pin_count = 0;
                for (const auto& pin_ : ar_.getSiteTypeList()[siteType.getPrimaryType()].getPins()) {
                    std::tuple<int, int, std::string> pin(tileType.getName(), it, str(pin_.getName()));
                    const auto& model = pin_.getModel();
                    float value = 0.0;
                    if (pin_.getDir() == LogicalNetlist::Netlist::Direction::OUTPUT && model.hasResistance()) {
                        value = get_corner_value(model.getResistance(), "slow", "max");
                    } else if (pin_.getDir() == LogicalNetlist::Netlist::Direction::INPUT && model.hasCapacitance()) {
                        value = get_corner_value(model.getCapacitance(), "slow", "max");
                    }
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
            if (tile_type_to_pb_type_.find(tile_type_name) == tile_type_to_pb_type_.end())
                continue;
            sink_source_loc_map_[tile_id].resize(tile_type_to_pb_type_[tile_type_name]->num_pins);
            int it = 0;
            for (const auto& sub_tile : tile_type_to_pb_type_[tile_type_name]->sub_tiles) {
                for (const auto& port : sub_tile.ports) {
                    float value;
                    int wire_id;
                    int node_id;
                    int pin_id = sub_tile.sub_tile_to_tile_pin_indices[port.index];
                    std::tuple<int, int, std::string> key{tile_type_id, it, std::string(port.name)};
                    std::tie<float, int>(value, wire_id) = tile_type_site_num_pin_name_model[key];
                    node_id = wire_to_node_[std::make_tuple(tile_id, wire_id)];

                    location loc = tile_to_loc_[tile_id];

                    used_by_pin_.insert(std::make_tuple(node_id, loc));
                    sink_source_loc_map_[tile_id][pin_id] = std::make_tuple(port.type == IN_PORT, value, node_id);
                }
                it++;
            }
        }
    }

    /*
     * This function populate device chan_width structures, they are needed in later stages of VPR flow
     */
    void pack_chan_width() {
        device_ctx_.chan_width.x_list.resize(grid_.height(), 0);
        device_ctx_.chan_width.y_list.resize(grid_.width(), 0);
        int min_x, min_y, max_x, max_y;
        min_x = min_y = 0x7fffffff;
        max_x = max_y = 0;
        for (auto i : chan_loc_map_) {
            auto key = i.first;
            auto vec = i.second;
            location loc;
            e_rr_type type;
            std::tie(loc, type) = key;
            int x, y;
            std::tie(x, y) = loc;
            if (type == e_rr_type::CHANX) {
                min_x = std::min(min_x, (int)vec.size());
                max_x = std::max(max_x, (int)vec.size());
                device_ctx_.chan_width.x_list[y] = std::max((int)device_ctx_.chan_width.x_list[y], (int)vec.size());
            } else {
                min_y = std::min(min_x, (int)vec.size());
                max_y = std::max(max_x, (int)vec.size());
                device_ctx_.chan_width.y_list[x] = std::max((int)device_ctx_.chan_width.y_list[x], (int)vec.size());
            }
        }
        device_ctx_.chan_width.max = std::max(max_y, max_x);
        device_ctx_.chan_width.x_min = min_x;
        device_ctx_.chan_width.x_max = max_x;
        device_ctx_.chan_width.y_min = min_y;
        device_ctx_.chan_width.y_max = max_y;
    }

    void pack_tiles() {
        for (auto& i : sink_source_loc_map_) {
            int tile_id = i.first;
            int x, y;
            location loc = tile_to_loc_[tile_id];
            std::tie(x, y) = loc;
            auto pin_vec = i.second;
            for (size_t j = 0; j < pin_vec.size(); ++j) {
                bool input;
                float RC;
                int FPGA_Interchange_node_id;
                std::tie(input, RC, FPGA_Interchange_node_id) = pin_vec[j];
                e_rr_type pin = input ? e_rr_type::SINK : e_rr_type::SOURCE;

                float R, C;
                std::tie(R, C) = input ? std::tuple<float, float>(0, RC) : std::tuple<float, float>(RC, 0);

                location track_loc;
                e_rr_type track_type;
                int track_idx;
                std::tie(track_loc, track_type, track_idx) = redirect_[std::make_tuple(FPGA_Interchange_node_id, loc)];
                auto val = chan_loc_map_[std::make_tuple(track_loc, track_type)][track_idx];
                int track_seg;
                float track_R, track_C;
                std::tie(track_seg, track_R, track_C) = val;
                track_R += R;
                track_C += C;
                chan_loc_map_[std::make_tuple(track_loc, track_type)][track_idx] = std::make_tuple(track_seg, track_R, track_C);

                device_ctx_.rr_nodes.make_room_for_node(RRNodeId(rr_idx));
                loc_type_idx_to_rr_idx_[std::make_tuple(loc, pin, j)] = rr_idx;
                auto node = device_ctx_.rr_nodes[rr_idx];
                RRNodeId node_id = node.id();
                device_ctx_.rr_graph_builder.set_node_type(node_id, pin);
                device_ctx_.rr_graph_builder.set_node_capacity(node_id, 1);
                if (pin == e_rr_type::SOURCE) {
                    device_ctx_.rr_graph_builder.set_node_cost_index(node_id, RRIndexedDataId(SOURCE_COST_INDEX));
                } else {
                    device_ctx_.rr_graph_builder.set_node_cost_index(node_id, RRIndexedDataId(SINK_COST_INDEX));
                }
                device_ctx_.rr_graph_builder.set_node_rc_index(node_id, NodeRCIndex(find_create_rr_rc_data(0, 0)));
                device_ctx_.rr_graph_builder.set_node_coordinates(node_id, x, y, x, y);
                device_ctx_.rr_graph_builder.set_node_ptc_num(node_id, j);
                rr_idx++;
            }
            for (size_t j = 0; j < pin_vec.size(); ++j) {
                bool input;
                std::tie(input, std::ignore, std::ignore) = pin_vec[j];
                e_rr_type pin = input ? e_rr_type::IPIN : e_rr_type::OPIN;

                device_ctx_.rr_nodes.make_room_for_node(RRNodeId(rr_idx));
                loc_type_idx_to_rr_idx_[std::make_tuple(loc, pin, j)] = rr_idx;
                auto node = device_ctx_.rr_nodes[rr_idx];
                RRNodeId node_id = node.id();
                device_ctx_.rr_graph_builder.set_node_type(node_id, pin);
                device_ctx_.rr_graph_builder.set_node_capacity(node_id, 1);
                if (pin == e_rr_type::IPIN) {
                    device_ctx_.rr_graph_builder.set_node_cost_index(node_id, RRIndexedDataId(IPIN_COST_INDEX));
                } else {
                    device_ctx_.rr_graph_builder.set_node_cost_index(node_id, RRIndexedDataId(OPIN_COST_INDEX));
                }
                device_ctx_.rr_graph_builder.set_node_rc_index(node_id, NodeRCIndex(find_create_rr_rc_data(0, 0)));
                device_ctx_.rr_graph_builder.set_node_coordinates(node_id, x, y, x, y);
                device_ctx_.rr_graph_builder.set_node_ptc_num(node_id, j);
                device_ctx_.rr_graph_builder.add_node_side(node_id, e_side::BOTTOM);
                rr_idx++;
            }
        }
    }

    void pack_chans() {
        for (auto i : chan_loc_map_) {
            location loc;
            e_rr_type type;
            std::tie(loc, type) = i.first;
            int x, y;
            std::tie(x, y) = loc;
            auto tracks = i.second;
            for (size_t j = 0; j < tracks.size(); ++j) {
                int seg;
                float R, C;
                std::tie(seg, R, C) = tracks[j];

                device_ctx_.rr_nodes.make_room_for_node(RRNodeId(rr_idx));
                loc_type_idx_to_rr_idx_[std::make_tuple(loc, type, j)] = rr_idx;
                auto node = device_ctx_.rr_nodes[rr_idx];
                RRNodeId node_id = node.id();

                device_ctx_.rr_graph_builder.set_node_type(node_id, type);
                device_ctx_.rr_graph_builder.set_node_capacity(node_id, 1);
                device_ctx_.rr_graph_builder.set_node_rc_index(node_id, NodeRCIndex(find_create_rr_rc_data(R, C)));
                device_ctx_.rr_graph_builder.set_node_direction(node_id, Direction::BIDIR);

                device_ctx_.rr_graph_builder.set_node_coordinates(node_id, x, y, x, y);
                device_ctx_.rr_graph_builder.set_node_ptc_num(node_id, j);

                if (type == e_rr_type::CHANX) {
                    device_ctx_.rr_graph_builder.set_node_cost_index(node_id, RRIndexedDataId(CHANX_COST_INDEX_START + seg));
                } else {
                    device_ctx_.rr_graph_builder.set_node_cost_index(node_id, RRIndexedDataId(CHANX_COST_INDEX_START + segment_inf_.size() + seg));
                }
                seg_index_[device_ctx_.rr_graph.node_cost_index(node_id)] = seg;

                rr_idx++;
            }
        }
    }

    void pack_rr_nodes() {
        device_ctx_.rr_nodes.clear();
        seg_index_.resize(CHANX_COST_INDEX_START + 2 * segment_inf_.size(), -1);
        pack_tiles();
        pack_chans();
        device_ctx_.rr_nodes.shrink_to_fit();
    }

    void pack_tiles_edges() {
        for (auto& i : sink_source_loc_map_) {
            int tile_id = i.first;
            int x, y;
            location loc = tile_to_loc_[tile_id];
            std::tie(x, y) = loc;
            auto pin_vec = i.second;
            for (size_t j = 0; j < pin_vec.size(); ++j) {
                bool input;
                int node_id;
                std::tie(input, std::ignore, node_id) = pin_vec[j];
                auto chan_key = redirect_[std::make_tuple(node_id, loc)];
                e_rr_type pin = input ? e_rr_type::SINK : e_rr_type::SOURCE;
                e_rr_type mux = input ? e_rr_type::IPIN : e_rr_type::OPIN;

                int pin_id, mux_id, track_id;
                VTR_ASSERT(loc_type_idx_to_rr_idx_.find(std::make_tuple(loc, pin, j)) != loc_type_idx_to_rr_idx_.end());
                VTR_ASSERT(loc_type_idx_to_rr_idx_.find(std::make_tuple(loc, mux, j)) != loc_type_idx_to_rr_idx_.end());
                pin_id = loc_type_idx_to_rr_idx_[std::make_tuple(loc, pin, j)];
                mux_id = loc_type_idx_to_rr_idx_[std::make_tuple(loc, mux, j)];
                track_id = loc_type_idx_to_rr_idx_[chan_key];

                int sink, sink_src, src;
                sink = input ? pin_id : track_id;
                sink_src = mux_id;
                src = input ? track_id : pin_id;

                device_ctx_.rr_graph_builder.emplace_back_edge(RRNodeId(src), RRNodeId(sink_src), 0);
                device_ctx_.rr_graph_builder.emplace_back_edge(RRNodeId(sink_src), RRNodeId(sink), sink == track_id ? 1 : 0);
            }
        }
    }

    void pack_chans_edges() {
        for (auto& i : shorts_) {
            location l1, l2;
            e_rr_type t1, t2;
            int idx1, idx2;
            std::tie(l1, t1, idx1, l2, t2, idx2) = i;
            std::tuple<location, e_rr_type, int> key1(l1, t1, idx1), key2(l2, t2, idx2);

            VTR_ASSERT(loc_type_idx_to_rr_idx_.find(key1) != loc_type_idx_to_rr_idx_.end());
            VTR_ASSERT(loc_type_idx_to_rr_idx_.find(key2) != loc_type_idx_to_rr_idx_.end());

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
            int sw_id;
            std::tuple<int, int, int, bool> metadata;
            std::tie(sw_id, metadata) = i.second;
            VTR_ASSERT(sw_id > 1);
            int tile_id, wire0, wire1;
            bool forward;
            std::tie(tile_id, wire0, wire1, forward) = metadata;
            location loc = tile_to_loc_[tile_id];
            auto key1 = redirect_[std::make_tuple(node1, loc)];
            auto key2 = redirect_[std::make_tuple(node2, loc)];

            VTR_ASSERT(loc_type_idx_to_rr_idx_.find(key1) != loc_type_idx_to_rr_idx_.end());
            VTR_ASSERT(loc_type_idx_to_rr_idx_.find(key2) != loc_type_idx_to_rr_idx_.end());

            int src, sink;
            src = loc_type_idx_to_rr_idx_[key1];
            sink = loc_type_idx_to_rr_idx_[key2];
            device_ctx_.rr_graph_builder.emplace_back_edge(RRNodeId(src), RRNodeId(sink), sw_id);

            vtr::interned_string name_(empty_);
            vtr::interned_string value_(empty_);

            char metadata_[100];
            sprintf(metadata_, "%d,%d,%d,%d", tile_id, wire0, wire1, forward ? 1 : 0);

            name_ = device_ctx_.arch->strings.intern_string(vtr::string_view("FPGAInterchange"));
            value_ = device_ctx_.arch->strings.intern_string(vtr::string_view(metadata_));
            vpr::add_rr_edge_metadata(src, sink, sw_id, name_, value_);
        }
    }

    void pack_rr_edges() {
        pack_tiles_edges();
        pack_chans_edges();
        pack_pips();
        device_ctx_.rr_graph_builder.mark_edges_as_rr_switch_ids();
        device_ctx_.rr_graph_builder.partition_edges();
    }

    /*
     * Fill device rr_graph informations.
     */
    void pack_to_rr_graph() {
        pack_chan_width();
        // switches are already packed
        // segments are already packed before rr_generation
        // physical_tile_types are already packed before rr_generation
        // grid is already packed before rr_generation
        pack_rr_nodes();
        pack_rr_edges();
    }

    /*
     * Finish rr_graph generation.
     * Build node_lookup, inito fan_ins, load indexed data
     */
    void finish_rr_generation() {
        for (t_rr_type rr_type : RR_TYPES) {
            if (rr_type == CHANX) {
                device_ctx_.rr_graph_builder.node_lookup().resize_nodes(grid_.height(), grid_.width(), rr_type, NUM_SIDES);
            } else {
                device_ctx_.rr_graph_builder.node_lookup().resize_nodes(grid_.width(), grid_.height(), rr_type, NUM_SIDES);
            }
        }
        for (size_t node = 0; node < device_ctx_.rr_nodes.size(); node++) {
            auto rr_node = device_ctx_.rr_nodes[node];
            device_ctx_.rr_graph_builder.add_node_to_all_locs(rr_node.id());
        }

        device_ctx_.rr_graph_builder.init_fan_in();

        /* Create a temp copy to convert from vtr::vector to std::vector
         * This is required because the ``alloc_and_load_rr_indexed_data()`` function supports only std::vector data
         * type for ``rr_segments``
         * Note that this is a dirty fix (to avoid massive code changes)
         * TODO: The ``alloc_and_load_rr_indexed_data()`` function should embrace ``vtr::vector`` for ``rr_segments``
         */
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
        for (size_t i = 0; i < seg_index_.size(); ++i) {
            device_ctx_.rr_indexed_data[RRIndexedDataId(i)].seg_index = seg_index_[RRIndexedDataId(i)];
        }
    }
};

void build_rr_graph_fpga_interchange(const t_graph_type graph_type,
                                     const DeviceGrid& grid,
                                     const std::vector<t_segment_inf>& segment_inf,
                                     const enum e_base_cost_type base_cost_type,
                                     int* wire_to_rr_ipin_switch,
                                     bool do_check_rr_graph) {
    vtr::ScopedStartFinishTimer timer("Building RR Graph from device database");

    auto& device_ctx = g_vpr_ctx.mutable_device();
    size_t num_segments = segment_inf.size();
    device_ctx.rr_segments.reserve(num_segments);
    for (long unsigned int iseg = 0; iseg < num_segments; ++iseg) {
        device_ctx.rr_segments.push_back(segment_inf[(iseg)]);
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

    RR_Graph_Builder builder(device_reader, grid, device_ctx, device_ctx.physical_tile_types, device_ctx.rr_segments, base_cost_type);
    builder.build_rr_graph();
    *wire_to_rr_ipin_switch = 0;
    device_ctx.read_rr_graph_filename.assign(get_arch_file_name());

    if (do_check_rr_graph) {
        check_rr_graph(graph_type, grid, device_ctx.physical_tile_types);
    }
}
