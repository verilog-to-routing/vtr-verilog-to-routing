// test framework
#include "catch2/catch_test_macros.hpp"

#include "rr_rc_data.h"
#include "read_xml_arch_file.h"
#include "vtr_expr_eval.h"
#include "vpr_error.h"
#include "get_parallel_segs.h"
#include "alloc_and_load_rr_indexed_data.h"

#include "test_utils.cpp"

using vtr::FormulaParser;
using vtr::t_formula_data;

////////////////////////////////////////////////////////////////////////////////////////////////////
DeviceGrid create_device_grid(const std::vector<t_grid_def>& grid_layouts,
                           const size_t width,
                           const size_t height,
                           const std::vector<t_physical_tile_type>& physical_tile_types);

t_chan_width init_channel(DeviceGrid grid, int chan_width_max, int x_max, int x_min, int y_max, int y_min, int info);

t_rr_switch_inf create_rr_switches(t_arch_switch_inf* arch_switch_inf, int arch_switch_idx);

void create_rr_nodes(RRGraphBuilder* rr_graph_builder,
                     std::vector<t_rr_rc_data>& rr_rc_data,
                     t_rr_edge_info_set* rr_edges_to_create,
                     const std::vector<t_physical_tile_type>& physical_tile_types,
                     const DeviceGrid& grid);

void create_rr_chan(RRGraphBuilder* rr_graph_builder,
                    std::vector<t_rr_rc_data>& rr_rc_data,
                    t_chan_width& nodes_per_chan,
                    DeviceGrid& grid);

RRNodeId create_sink_src_node(RRGraphBuilder* rr_graph_builder,
                          std::vector<t_rr_rc_data>& rr_rc_data,
                          const e_rr_type node_type,
                          const e_cost_indices node_cost,
                          const int ptc,
                          const int x_coord,
                          const int y_coord);

RRNodeId create_pin_node(RRGraphBuilder* rr_graph_builder,
                          std::vector<t_rr_rc_data>& rr_rc_data,
                          const e_rr_type node_type,
                          const e_cost_indices node_cost,
                          const e_side node_side,
                          const int ptc,
                          const int x_coord,
                          const int y_coord);

RRNodeId create_chan_node(RRGraphBuilder* rr_graph_builder,
                          std::vector<t_rr_rc_data>& rr_rc_data,
                          const e_rr_type node_type,
                          const e_cost_indices node_cost,
                          const Direction direction,
                          const int track,
                          const int x_coord,
                          const int y_coord,
                          const int x_offset,
                          const int y_offset);

void create_rr_edges(RRGraphBuilder* rr_graph_builder, t_rr_edge_info_set* rr_edges_to_create);

void create_rr_indexed_data(RRGraphView* rr_graph,
                            vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                            const DeviceGrid& grid,
                            const std::vector<t_segment_inf>& segment_inf,
                            const int wire_to_ipin_switch);
////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("I/O Test_Base_10N", "[librrgraph]"){
    static constexpr const char kArchFile[] = "test_arch_base.xml";
    static constexpr const char kRrGraphFile1[] = "test_read_rrgraph_base.xml";
    static constexpr const char kRrGraphFile2[] = "test_write_rrgraph_base.xml";

    t_arch arch;
    std::vector<t_physical_tile_type> physical_tile_types;
    std::vector<t_logical_block_type> logical_block_types;

    XmlReadArch(kArchFile, /*timing_enabled=*/false,
                &arch, physical_tile_types, logical_block_types);

    std::vector<t_segment_inf>& segment_inf = arch.Segments;
    enum e_base_cost_type base_cost_type;

    size_t num_arch_switches = arch.num_switches;
    t_arch_switch_inf* arch_switch_inf = arch.Switches;
    t_arch_switch_fanin arch_switch_fanins(num_arch_switches);
    std::vector<std::map<int, int>> switch_fanin_remap;

    int virtual_clock_network_root_idx{};
    int wire_to_rr_ipin_switch = -1;

    std::string read_rr_graph_filename = kRrGraphFile1;
    char echo_file_name[] = "rr_indexed_data.echo";

    bool echo_enabled = 1;
    bool read_edge_metadata = 1;
    bool do_check_rr_graph = 0;
    
    t_graph_type graph_type = GRAPH_UNIDIR;

    vtr::vector<RRIndexedDataId, t_rr_indexed_data> rr_indexed_data;
    std::vector<t_rr_rc_data> rr_rc_data;

    vtr::vector<RRIndexedDataId, short> seg_index;

    RRGraphBuilder rr_graph_builder {};
    RRGraphView rr_graph {rr_graph_builder.rr_nodes(), rr_graph_builder.node_lookup(), rr_graph_builder.rr_node_metadata(), rr_graph_builder.rr_edge_metadata(), rr_indexed_data, rr_rc_data, rr_graph_builder.rr_segments(), rr_graph_builder.rr_switch()};

    //Create Grid
    DeviceGrid grid = create_device_grid(arch.grid_layouts, arch.grid_layouts[0].width, arch.grid_layouts[0].height, physical_tile_types);

    //Init Chan_Width
    t_chan_width chan_width = init_channel(grid, 2, 2, 2, 2, 2, 5);

    //Create Segment
    size_t num_segments = segment_inf.size();
    rr_graph_builder.reserve_segments(num_segments);
    for (size_t iseg = 0; iseg < num_segments; ++iseg) {
        rr_graph_builder.add_rr_segment(segment_inf[iseg]);
    }
    
    //Create Switches
    rr_graph_builder.reserve_switches(num_arch_switches);
    for (size_t iswitch = 0; iswitch < num_arch_switches; ++iswitch) {
        const t_rr_switch_inf& temp_rr_switch = create_rr_switches(arch_switch_inf, iswitch);
        rr_graph_builder.add_rr_switch(temp_rr_switch);
    }
    
    //Create Nodes
    create_sink_src_node(&rr_graph_builder, rr_rc_data, SOURCE, SOURCE_COST_INDEX, 0, 0, 1);
    create_sink_src_node(&rr_graph_builder, rr_rc_data, SINK, SINK_COST_INDEX, 1, 0, 1);
    create_pin_node(&rr_graph_builder, rr_rc_data, OPIN, OPIN_COST_INDEX, RIGHT, 0, 0, 1);
    create_pin_node(&rr_graph_builder, rr_rc_data, IPIN, IPIN_COST_INDEX, RIGHT, 1, 0, 1);
    create_sink_src_node(&rr_graph_builder, rr_rc_data, SOURCE, SOURCE_COST_INDEX, 0, 1, 0);
    create_sink_src_node(&rr_graph_builder, rr_rc_data, SINK, SINK_COST_INDEX, 1, 1, 0);
    create_pin_node(&rr_graph_builder, rr_rc_data, OPIN, OPIN_COST_INDEX, TOP, 0, 1, 0);
    create_pin_node(&rr_graph_builder, rr_rc_data, IPIN, IPIN_COST_INDEX, TOP, 1, 1, 0);
    create_chan_node(&rr_graph_builder, rr_rc_data, CHANX, CHANX_COST_INDEX_START, Direction::INC, 0, 1, 0, 0, 0);
    create_chan_node(&rr_graph_builder, rr_rc_data, CHANX, CHANX_COST_INDEX_START, Direction::DEC, 1, 1, 0, 0, 0);
    create_chan_node(&rr_graph_builder, rr_rc_data, CHANY, CHANX_COST_INDEX_START, Direction::INC, 0, 0, 1, 0, 0);
    create_chan_node(&rr_graph_builder, rr_rc_data, CHANY, CHANX_COST_INDEX_START, Direction::DEC, 1, 0, 1, 0, 0);

    //Create Edges
    t_rr_edge_info_set rr_edges_to_create;
    rr_edges_to_create.emplace_back(RRNodeId(0), RRNodeId(2), 0);
    rr_edges_to_create.emplace_back(RRNodeId(2), RRNodeId(11), 0);
    rr_edges_to_create.emplace_back(RRNodeId(11), RRNodeId(8), 0);
    rr_edges_to_create.emplace_back(RRNodeId(8), RRNodeId(7), 0);
    rr_edges_to_create.emplace_back(RRNodeId(7), RRNodeId(5), 0);

    rr_edges_to_create.emplace_back(RRNodeId(4), RRNodeId(6), 0);
    rr_edges_to_create.emplace_back(RRNodeId(6), RRNodeId(9), 0);
    rr_edges_to_create.emplace_back(RRNodeId(9), RRNodeId(10), 0);
    rr_edges_to_create.emplace_back(RRNodeId(10), RRNodeId(3), 0);
    rr_edges_to_create.emplace_back(RRNodeId(3), RRNodeId(1), 0);
    
    uniquify_edges(rr_edges_to_create);
    create_rr_edges(&rr_graph_builder, &rr_edges_to_create);

    //Create Fanin
    rr_graph_builder.init_fan_in();
    size_t num_rr_switches = rr_graph_builder.count_rr_switches(num_arch_switches, arch_switch_inf, arch_switch_fanins);
    rr_graph_builder.resize_switches(num_rr_switches);    

    //Edge/Switch Remap
    rr_graph_builder.remap_rr_node_switch_indices(arch_switch_fanins);

    //Edge Partition
    rr_graph_builder.partition_edges();

    //Create Indexed Data
    create_rr_indexed_data(&rr_graph, rr_indexed_data, grid, segment_inf, wire_to_rr_ipin_switch);

    //Write drafted rrgraph test case
    write_rr_graph(&rr_graph_builder,
                   &rr_graph,
                   physical_tile_types,
                   &rr_indexed_data,
                   &rr_rc_data,
                   grid,
                   arch_switch_inf,
                   &arch,
                   &chan_width,
                   num_arch_switches,
                   kRrGraphFile1,
                   virtual_clock_network_root_idx,
                   echo_enabled,
                   echo_file_name,
                   true);
    
    //New rr_graph_builder for I/O test
    RRGraphBuilder rr_graph_builder2 {};
    RRGraphView rr_graph2 {rr_graph_builder2.rr_nodes(), rr_graph_builder2.node_lookup(), rr_graph_builder2.rr_node_metadata(), rr_graph_builder2.rr_edge_metadata(), rr_indexed_data, rr_rc_data, rr_graph_builder2.rr_segments(), rr_graph_builder2.rr_switch()};

    //Read drafted rrgraph
    load_rr_file(&rr_graph_builder2,
                &rr_graph2,
                physical_tile_types,
                segment_inf,
                &rr_indexed_data,
                &rr_rc_data,
                grid,
                arch_switch_inf,
                graph_type,
                &arch,
                &chan_width,
                base_cost_type,
                num_arch_switches,
                virtual_clock_network_root_idx,
                &wire_to_rr_ipin_switch,
                read_rr_graph_filename.c_str(),
                &read_rr_graph_filename,
                read_edge_metadata,
                do_check_rr_graph,
                echo_enabled,
                echo_file_name,
                true);

    //Write rrgraph
    write_rr_graph(&rr_graph_builder,
                &rr_graph,
                physical_tile_types,
                &rr_indexed_data,
                &rr_rc_data,
                grid,
                arch_switch_inf,
                &arch,
                &chan_width,
                num_arch_switches,
                kRrGraphFile2,
                virtual_clock_network_root_idx,
                echo_enabled,
                echo_file_name,
                true);
}

TEST_CASE("I/O Test_mult", "[librrgraph]"){
    static constexpr const char kArchFile[] = "test_arch.xml";
    static constexpr const char kRrGraphFile1[] = "test_read_rrgraph.xml";
    static constexpr const char kRrGraphFile2[] = "test_write_rrgraph.xml";

    t_arch arch;
    std::vector<t_physical_tile_type> physical_tile_types;
    std::vector<t_logical_block_type> logical_block_types;

    XmlReadArch(kArchFile, /*timing_enabled=*/false,
                &arch, physical_tile_types, logical_block_types);

    std::vector<t_segment_inf>& segment_inf = arch.Segments;
    enum e_base_cost_type base_cost_type;

    size_t num_arch_switches = arch.num_switches;
    t_arch_switch_inf* arch_switch_inf = arch.Switches;
    t_arch_switch_fanin arch_switch_fanins(num_arch_switches);
    std::vector<std::map<int, int>> switch_fanin_remap;

    int virtual_clock_network_root_idx{};
    int wire_to_rr_ipin_switch = -1;

    std::string read_rr_graph_filename = kRrGraphFile1;
    char echo_file_name[] = "rr_indexed_data.echo";

    bool echo_enabled = 1;
    bool read_edge_metadata = 1;
    bool do_check_rr_graph = 0;
    
    t_graph_type graph_type = GRAPH_UNIDIR;

    vtr::vector<RRIndexedDataId, t_rr_indexed_data> rr_indexed_data;
    std::vector<t_rr_rc_data> rr_rc_data;

    vtr::vector<RRIndexedDataId, short> seg_index;

    RRGraphBuilder rr_graph_builder {};
    RRGraphView rr_graph {rr_graph_builder.rr_nodes(), rr_graph_builder.node_lookup(), rr_graph_builder.rr_node_metadata(), rr_graph_builder.rr_edge_metadata(), rr_indexed_data, rr_rc_data, rr_graph_builder.rr_segments(), rr_graph_builder.rr_switch()};

    //Create Grid
    DeviceGrid grid = create_device_grid(arch.grid_layouts, arch.grid_layouts[0].width, arch.grid_layouts[0].height, physical_tile_types);    

    //Init Chan_Width
    t_chan_width chan_width = init_channel(grid, 4, 4, 4, 4, 4, 4);

    //Create Segment
    size_t num_segments = segment_inf.size();
    rr_graph_builder.reserve_segments(num_segments);
    for (size_t iseg = 0; iseg < num_segments; ++iseg) {
        rr_graph_builder.add_rr_segment(segment_inf[iseg]);
    }
    
    //Create Switches
    rr_graph_builder.reserve_switches(num_arch_switches);
    // Create the switches
    for (size_t iswitch = 0; iswitch < num_arch_switches; ++iswitch) {
        const t_rr_switch_inf& temp_rr_switch = create_rr_switches(arch_switch_inf, iswitch);
        rr_graph_builder.add_rr_switch(temp_rr_switch);
    }
    
    //Create Nodes (SOURCE/SINK/IPIN/OPIN)
    t_rr_edge_info_set rr_edges_to_create;
    create_rr_nodes(&rr_graph_builder, rr_rc_data, &rr_edges_to_create, physical_tile_types, grid);
    
    //Create Chan Nodes
    create_rr_chan(&rr_graph_builder, rr_rc_data, chan_width, grid);

    /*TODO: add edges to rr_edges_to_create for linking of opin_to_track, track_to_track, ipin_to_track.
            May Need to modify create_rr_nodes and create_rr_chan.*/
    
    //Create Edges
    uniquify_edges(rr_edges_to_create);
    create_rr_edges(&rr_graph_builder, &rr_edges_to_create);
    
    //Create Fan_in
    rr_graph_builder.init_fan_in();
    size_t num_rr_switches = rr_graph_builder.count_rr_switches(num_arch_switches, arch_switch_inf, arch_switch_fanins);
    rr_graph_builder.resize_switches(num_rr_switches);
    
    //Edge/Switch Remap
    rr_graph_builder.remap_rr_node_switch_indices(arch_switch_fanins);
    
    //Edge Partition
    rr_graph_builder.partition_edges();
    
    //Create Indexed Data
    create_rr_indexed_data(&rr_graph, rr_indexed_data, grid, segment_inf, wire_to_rr_ipin_switch);
    
    //Write drafted rrgraph test case
    write_rr_graph(&rr_graph_builder,
                   &rr_graph,
                   physical_tile_types,
                   &rr_indexed_data,
                   &rr_rc_data,
                   grid,
                   arch_switch_inf,
                   &arch,
                   &chan_width,
                   num_arch_switches,
                   kRrGraphFile1,
                   virtual_clock_network_root_idx,
                   echo_enabled,
                   echo_file_name,
                   true);
    
    //New rr_graph_builder for I/O test
    RRGraphBuilder rr_graph_builder2 {};
    RRGraphView rr_graph2 {rr_graph_builder2.rr_nodes(), rr_graph_builder2.node_lookup(), rr_graph_builder2.rr_node_metadata(), rr_graph_builder2.rr_edge_metadata(), rr_indexed_data, rr_rc_data, rr_graph_builder2.rr_segments(), rr_graph_builder2.rr_switch()};

    //Read drafted rrgraph
    load_rr_file(&rr_graph_builder2,
                &rr_graph2,
                physical_tile_types,
                segment_inf,
                &rr_indexed_data,
                &rr_rc_data,
                grid,
                arch_switch_inf,
                graph_type,
                &arch,
                &chan_width,
                base_cost_type,
                num_arch_switches,
                virtual_clock_network_root_idx,
                &wire_to_rr_ipin_switch,
                read_rr_graph_filename.c_str(),
                &read_rr_graph_filename,
                read_edge_metadata,
                do_check_rr_graph,
                echo_enabled,
                echo_file_name,
                true);

    //Write rrgraph
    write_rr_graph(&rr_graph_builder,
                &rr_graph,
                physical_tile_types,
                &rr_indexed_data,
                &rr_rc_data,
                grid,
                arch_switch_inf,
                &arch,
                &chan_width,
                num_arch_switches,
                kRrGraphFile2,
                virtual_clock_network_root_idx,
                echo_enabled,
                echo_file_name,
                true);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
DeviceGrid create_device_grid(const std::vector<t_grid_def>& grid_layouts,
                              const size_t width,
                              const size_t height,
                              const std::vector<t_physical_tile_type>& physical_tile_types){

    FormulaParser p;
    auto grid = vtr::Matrix<t_grid_tile>({width, height});
    auto grid_priorities = vtr::Matrix<int>({width, height}, std::numeric_limits<int>::lowest());
    const auto& grid_def = grid_layouts[0];

    //Initialize all blocks in device to empty blocks
    t_physical_tile_type type_ = get_empty_physical_type();
    t_physical_tile_type_ptr empty_type = &type_;
    VTR_ASSERT(empty_type != nullptr);

    for (size_t x = 0; x < width; ++x) {
        for (size_t y = 0; y < height; ++y) {
            set_grid_block_type(std::numeric_limits<int>::lowest() + 1, //+1 so it overrides without warning
                                empty_type, x, y, grid, grid_priorities, /*meta=*/nullptr);
        }
    }

    //Fill in non-empty blocks
    for (const auto& grid_loc_def : grid_def.loc_defs) {
        for (const auto& tile_type : physical_tile_types) {
            if (tile_type.name == grid_loc_def.block_type) {
                auto type = &tile_type;
                t_formula_data vars;
                vars.set_var_value("W", width);
                vars.set_var_value("H", height);
                vars.set_var_value("w", type->width);
                vars.set_var_value("h", type->height);

                //Load the x specification
                auto& xspec = grid_loc_def.x;

                size_t startx = p.parse_formula(xspec.start_expr, vars);
                size_t endx = p.parse_formula(xspec.end_expr, vars);
                size_t incrx = p.parse_formula(xspec.incr_expr, vars);
                size_t repeatx = p.parse_formula(xspec.repeat_expr, vars);

                //Load the y specification
                auto& yspec = grid_loc_def.y;
                size_t starty = p.parse_formula(yspec.start_expr, vars);
                size_t endy = p.parse_formula(yspec.end_expr, vars);
                size_t incry = p.parse_formula(yspec.incr_expr, vars);
                size_t repeaty = p.parse_formula(yspec.repeat_expr, vars);

                size_t x_end = 0;
                for (size_t kx = 0; x_end < width; ++kx) { //Repeat in x direction
                    size_t x_start = startx + kx * repeatx;
                    x_end = endx + kx * repeatx;

                    size_t y_end = 0;
                    for (size_t ky = 0; y_end < height; ++ky) { //Repeat in y direction
                        size_t y_start = starty + ky * repeaty;
                        y_end = endy + ky * repeaty;

                        size_t x_max = std::min(x_end, width - 1);
                        size_t y_max = std::min(y_end, height - 1);

                        //Fill in the region
                        for (size_t x = x_start; x + (type->width - 1) <= x_max; x += incrx) {
                            for (size_t y = y_start; y + (type->height - 1) <= y_max; y += incry) {
                                set_grid_block_type(grid_loc_def.priority, type, x, y, grid, grid_priorities, grid_loc_def.meta);
                            }
                        }
                    }
                }
            }
        }
    }
    return DeviceGrid(grid_def.name, grid);
}

t_chan_width init_channel(DeviceGrid grid, int chan_width_max, int x_max, int x_min, int y_max, int y_min, int info){
    t_chan_width chan_width;

    chan_width.max = chan_width_max;
    chan_width.max = x_min;
    chan_width.x_min = x_min;
    chan_width.y_min = y_min;
    chan_width.x_max = x_max;
    chan_width.y_max = y_max;
    chan_width.x_list.resize(grid.height());
    chan_width.y_list.resize(grid.width());

    for(size_t x=0; x< chan_width.x_list.size(); ++x){
        chan_width.x_list[x] = info;
    }
    for(size_t y=0; y< chan_width.y_list.size(); ++y){
        chan_width.y_list[y] = info;
    }

    return chan_width;
}

t_rr_switch_inf create_rr_switches(t_arch_switch_inf* arch_switch_inf, int arch_switch_idx){
    t_rr_switch_inf rr_switch_inf;

    /* figure out, by looking at the arch switch's Tdel map, what the delay of the new
     * rr switch should be */
    double rr_switch_Tdel = arch_switch_inf[arch_switch_idx].Tdel(0);

    /* copy over the arch switch to rr_switch_inf[rr_switch_idx], but with the changed Tdel value */
    rr_switch_inf.set_type(arch_switch_inf[arch_switch_idx].type());
    rr_switch_inf.R = arch_switch_inf[arch_switch_idx].R;
    rr_switch_inf.Cin = arch_switch_inf[arch_switch_idx].Cin;
    rr_switch_inf.Cinternal = arch_switch_inf[arch_switch_idx].Cinternal;
    rr_switch_inf.Cout = arch_switch_inf[arch_switch_idx].Cout;
    rr_switch_inf.Tdel = rr_switch_Tdel;
    rr_switch_inf.mux_trans_size = arch_switch_inf[arch_switch_idx].mux_trans_size;

    rr_switch_inf.buf_size = arch_switch_inf[arch_switch_idx].buf_size;
    
    rr_switch_inf.name = arch_switch_inf[arch_switch_idx].name;
    rr_switch_inf.power_buffer_type = arch_switch_inf[arch_switch_idx].power_buffer_type;
    rr_switch_inf.power_buffer_size = arch_switch_inf[arch_switch_idx].power_buffer_size;

    return rr_switch_inf;
}

RRNodeId create_sink_src_node(RRGraphBuilder* rr_graph_builder,
                          std::vector<t_rr_rc_data>& rr_rc_data,
                          const e_rr_type node_type,
                          const e_cost_indices node_cost,
                          const int ptc,
                          const int x_coord,
                          const int y_coord){

    rr_graph_builder->emplace_back();
    auto node_id = RRNodeId(rr_graph_builder->rr_nodes().size() - 1);
    
    rr_graph_builder->set_node_cost_index(node_id, RRIndexedDataId(node_cost));
    rr_graph_builder->set_node_type(node_id, node_type);
    rr_graph_builder->set_node_capacity(node_id, 1);
    rr_graph_builder->set_node_coordinates(node_id, x_coord, y_coord, x_coord, y_coord);
    rr_graph_builder->set_node_rc_index(node_id, NodeRCIndex(find_create_rr_rc_data(0.,0.,rr_rc_data)));
    rr_graph_builder->set_node_class_num(node_id, ptc);

    rr_graph_builder->node_lookup().add_node(node_id, x_coord, y_coord, node_type, ptc);

    VTR_ASSERT(rr_graph_builder->node_lookup().find_node(x_coord, y_coord, node_type, ptc));

    return node_id;
}

RRNodeId create_pin_node(RRGraphBuilder* rr_graph_builder,
                          std::vector<t_rr_rc_data>& rr_rc_data,
                          const e_rr_type node_type,
                          const e_cost_indices node_cost,
                          const e_side node_side,
                          const int ptc,
                          const int x_coord,
                          const int y_coord){

    rr_graph_builder->emplace_back();
    auto node_id = RRNodeId(rr_graph_builder->rr_nodes().size() - 1);

    rr_graph_builder->set_node_cost_index(node_id, RRIndexedDataId(node_cost));
    rr_graph_builder->set_node_type(node_id, node_type);      
    rr_graph_builder->set_node_capacity(node_id, 1);
    rr_graph_builder->set_node_coordinates(node_id, x_coord, y_coord, x_coord, y_coord);
    rr_graph_builder->set_node_rc_index(node_id, NodeRCIndex(find_create_rr_rc_data(0.,0.,rr_rc_data)));

    rr_graph_builder->set_node_pin_num(node_id, ptc);
    rr_graph_builder->add_node_side(node_id, node_side);

    rr_graph_builder->node_lookup().add_node(node_id, x_coord, y_coord, node_type, ptc, node_side);

    VTR_ASSERT(rr_graph_builder->node_lookup().find_node(x_coord, y_coord, node_type, ptc, node_side));

    return node_id;
}

RRNodeId create_chan_node(RRGraphBuilder* rr_graph_builder,
                          std::vector<t_rr_rc_data>& rr_rc_data,
                          const e_rr_type node_type,
                          const e_cost_indices node_cost,
                          const Direction direction,
                          const int track,
                          const int x_coord,
                          const int y_coord,
                          const int x_offset,
                          const int y_offset){

    rr_graph_builder->emplace_back();
    auto node_id = RRNodeId(rr_graph_builder->rr_nodes().size() - 1);

    if (node_type == CHANX) {
        rr_graph_builder->set_node_cost_index(node_id, RRIndexedDataId(node_cost));
        rr_graph_builder->set_node_capacity(node_id, 1);
        rr_graph_builder->set_node_coordinates(node_id, x_coord, y_coord, x_coord + x_offset, y_coord);
    } else {
        VTR_ASSERT(node_type == CHANY);
        rr_graph_builder->set_node_cost_index(node_id, RRIndexedDataId(node_cost+1));
        rr_graph_builder->set_node_capacity(node_id, 1);
        rr_graph_builder->set_node_coordinates(node_id, x_coord, y_coord, x_coord, y_coord + y_offset);
    }

    rr_graph_builder->set_node_rc_index(node_id, NodeRCIndex(find_create_rr_rc_data(0.,0.,rr_rc_data)));
    
    rr_graph_builder->set_node_type(node_id, node_type);
    rr_graph_builder->set_node_track_num(node_id, track);
    rr_graph_builder->set_node_direction(node_id, direction);

    rr_graph_builder->node_lookup().add_node(node_id, x_coord, y_coord, node_type, track);

    if (node_type == CHANX) 
        VTR_ASSERT(rr_graph_builder->node_lookup().find_node(y_coord, x_coord, node_type, track));
    else
        VTR_ASSERT(rr_graph_builder->node_lookup().find_node(x_coord, y_coord, node_type, track));

    return node_id;
}

void create_rr_nodes(RRGraphBuilder* rr_graph_builder,
                     std::vector<t_rr_rc_data>& rr_rc_data,
                     t_rr_edge_info_set* rr_edges_to_create,
                     const std::vector<t_physical_tile_type>& physical_tile_types,
                     const DeviceGrid& grid){

    for(size_t x=0; x<grid.height(); x++){
        for(size_t y=0; y<grid.width(); y++){
            for (const auto& tile_type : physical_tile_types) {
                if(grid[x][y].type->name == tile_type.name){
                    e_side side;
                    if(x == 0){
                        side = RIGHT;
                    } else if(x == grid.height()-1){
                        side = LEFT;
                    } else if(y == 0){
                        side = TOP;
                    } else {
                        side = BOTTOM;
                    }

                    if(tile_type.name != std::string("EMPTY")){
                        std::vector<std::tuple<e_rr_type, RRNodeId>> pin_list;
                        RRNodeId inode = RRNodeId::INVALID();
                        rr_graph_builder->node_lookup().reserve_nodes(x,y,SINK,2*tile_type.capacity);
                        rr_graph_builder->node_lookup().reserve_nodes(x,y,SOURCE,tile_type.num_output_pins);
                        
                        for(int i=0; i<tile_type.capacity; i++){
                            inode = create_sink_src_node(rr_graph_builder, rr_rc_data, SINK, SINK_COST_INDEX, 0 + i*3, x, y);
                            for(int j=0; j<tile_type.num_input_pins/tile_type.capacity;j++){
                                pin_list.insert(pin_list.end(), std::make_tuple(SINK, RRNodeId(inode)));
                            }
                            inode = create_sink_src_node(rr_graph_builder, rr_rc_data, SOURCE, SOURCE_COST_INDEX, 1 + i*3, x, y);
                            for(int j=0; j<tile_type.num_output_pins/tile_type.capacity;j++){
                                pin_list.insert(pin_list.end(), std::make_tuple(SOURCE, RRNodeId(inode)));
                            }
                            inode = create_sink_src_node(rr_graph_builder, rr_rc_data, SINK, SINK_COST_INDEX, 2 + i*3, x, y);
                            for(int j=0; j<tile_type.num_clock_pins/tile_type.capacity;j++){
                                pin_list.insert(pin_list.end(), std::make_tuple(SINK, RRNodeId(inode)));
                            }
                        }

                        if(tile_type.name == std::string("clb")){
                            int dir_ipin[4] = {0};
                            int dir_opin[4] = {0};
                            for(std::vector<std::tuple<e_rr_type, RRNodeId>>::size_type ptc = 0; ptc<pin_list.size(); ptc++){
                                auto& node_type = std::get<0>(pin_list[ptc]);
                                if(node_type == SINK){
                                    dir_ipin[ptc%4]++;
                                } else if(node_type == SOURCE){
                                    dir_opin[ptc%4]++;
                                }
                            }
                            for(int i=0; i<NUM_SIDES;i++){
                                rr_graph_builder->node_lookup().reserve_nodes(x, y, IPIN, dir_ipin[i], SIDES[i]);
                                rr_graph_builder->node_lookup().reserve_nodes(x, y, OPIN, dir_ipin[i], SIDES[i]);
                            }
                        } else if(tile_type.name == std::string("io")){
                            rr_graph_builder->node_lookup().reserve_nodes(x,y,IPIN,tile_type.num_input_pins+tile_type.num_clock_pins, side);
                            rr_graph_builder->node_lookup().reserve_nodes(x,y,OPIN,tile_type.num_output_pins, side);
                        }

                        for(std::vector<std::tuple<e_rr_type, RRNodeId>>::size_type ptc = 0; ptc<pin_list.size(); ptc++){
                            auto& node_type = std::get<0>(pin_list[ptc]);
                            auto& node_id = std::get<1>(pin_list[ptc]);  
                            if(node_type == SINK){
                                rr_edges_to_create->emplace_back(create_pin_node(rr_graph_builder, rr_rc_data, IPIN, IPIN_COST_INDEX, (tile_type.name == std::string("io") ? side : SIDES[ptc%4]), ptc, x, y), node_id, 0);
                            } else if(node_type == SOURCE)
                                rr_edges_to_create->emplace_back(node_id, create_pin_node(rr_graph_builder, rr_rc_data, OPIN, OPIN_COST_INDEX, (tile_type.name == std::string("io") ? side : SIDES[ptc%4]), ptc, x, y), 0);
                        }
                    }
                }
            }
        }
    }
}

void create_rr_chan(RRGraphBuilder* rr_graph_builder,
                    std::vector<t_rr_rc_data>& rr_rc_data,
                    t_chan_width& nodes_per_chan,
                    DeviceGrid& grid){
    
    for(size_t x=0; x<grid.width()-1; x++){
        for(size_t y=0; y<grid.height()-1; y++){
            if(x != 0){
                rr_graph_builder->node_lookup().reserve_nodes(x, y, CHANX, nodes_per_chan.x_max);
                for(int track=0;track<nodes_per_chan.x_max;track++){
                    create_chan_node(rr_graph_builder, rr_rc_data, CHANX, CHANX_COST_INDEX_START, (track%2==0 ? Direction::INC : Direction::DEC), track, x, y, 0, 0);
                }
            }
            if(y != 0){
                rr_graph_builder->node_lookup().reserve_nodes(x, y, CHANY, nodes_per_chan.y_max);
                for(int track=0;track<nodes_per_chan.y_max;track++){
                    create_chan_node(rr_graph_builder, rr_rc_data, CHANY, CHANX_COST_INDEX_START, (track%2==0 ? Direction::INC : Direction::DEC), track, x, y, 0, 0);
                }
            }
        }
    }
}

void create_rr_edges(RRGraphBuilder* rr_graph_builder, t_rr_edge_info_set* rr_edges_to_create){
    rr_graph_builder->alloc_and_load_edges(rr_edges_to_create);
}

void create_rr_indexed_data(RRGraphView* rr_graph,
                            vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                            const DeviceGrid& grid,
                            const std::vector<t_segment_inf>& segment_inf,
                            const int wire_to_ipin_switch
                            ){
    int i, length, index;

    t_unified_to_parallel_seg_index segment_index_map;
    std::vector<t_segment_inf> segment_inf_x = get_parallel_segs(segment_inf, segment_index_map, X_AXIS);
    std::vector<t_segment_inf> segment_inf_y = get_parallel_segs(segment_inf, segment_index_map, Y_AXIS);\
    int total_num_segment = segment_inf_x.size() + segment_inf_y.size();

    int num_rr_indexed_data = CHANX_COST_INDEX_START + total_num_segment;
    rr_indexed_data.resize(num_rr_indexed_data);

    constexpr float nan = std::numeric_limits<float>::quiet_NaN();
    for (i = SOURCE_COST_INDEX; i <= IPIN_COST_INDEX; i++) {
        rr_indexed_data[RRIndexedDataId(i)].ortho_cost_index = OPEN;
        rr_indexed_data[RRIndexedDataId(i)].seg_index = OPEN;
        rr_indexed_data[RRIndexedDataId(i)].inv_length = nan;
        rr_indexed_data[RRIndexedDataId(i)].T_linear = 0.;
        rr_indexed_data[RRIndexedDataId(i)].T_quadratic = 0.;
        rr_indexed_data[RRIndexedDataId(i)].C_load = 0.;
    }
    rr_indexed_data[RRIndexedDataId(IPIN_COST_INDEX)].T_linear = rr_graph->rr_switch_inf(RRSwitchId(wire_to_ipin_switch)).Tdel;

    std::vector<int> ortho_costs;
    ortho_costs = find_ortho_cost_index(*rr_graph, segment_inf_x, segment_inf_y, X_AXIS);

    /* X-directed segments*/
    for (size_t iseg = 0; iseg < segment_inf_x.size(); ++iseg) {
        index = iseg + CHANX_COST_INDEX_START;
        rr_indexed_data[RRIndexedDataId(index)].ortho_cost_index = ortho_costs[iseg];

        if (segment_inf_x[iseg].longline)
            length = grid.width();
        else
            length = std::min<int>(segment_inf_x[iseg].length, grid.width());

        rr_indexed_data[RRIndexedDataId(index)].inv_length = 1. / length;
        /*We use the index for the segment in the **unified** seg_inf vector not iseg which is relative 
         * to parallel axis segments vector */
        rr_indexed_data[RRIndexedDataId(index)].seg_index = segment_inf_x[iseg].seg_index;
        
        rr_indexed_data[RRIndexedDataId(index)].T_linear = 0.;
        rr_indexed_data[RRIndexedDataId(index)].T_quadratic = 0.;
        rr_indexed_data[RRIndexedDataId(index)].C_load = 0.;
    }

    /* Y-directed segments*/
    for (size_t iseg = segment_inf_x.size(); iseg < ortho_costs.size(); ++iseg) {
        index = iseg + CHANX_COST_INDEX_START;
        rr_indexed_data[RRIndexedDataId(index)].ortho_cost_index = ortho_costs[iseg];

        if (segment_inf_x[iseg - segment_inf_x.size()].longline)
            length = grid.width();
        else
            length = std::min<int>(segment_inf_y[iseg - segment_inf_x.size()].length, grid.width());

        rr_indexed_data[RRIndexedDataId(index)].inv_length = 1. / length;
        /*We use the index for the segment in the **unified** seg_inf vector not iseg which is relative 
         * to parallel axis segments vector */
        rr_indexed_data[RRIndexedDataId(index)].seg_index = segment_inf_y[iseg - segment_inf_x.size()].seg_index;

        rr_indexed_data[RRIndexedDataId(index)].T_linear = 0.;
        rr_indexed_data[RRIndexedDataId(index)].T_quadratic = 0.;
        rr_indexed_data[RRIndexedDataId(index)].C_load = 0.;
    }
}
