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

static constexpr const char kArchFile[] = "my_arch.xml";
static constexpr const char kRrGraphFile1[] = "test_read_rrgraph.xml";
static constexpr const char kRrGraphFile2[] = "test_write_rrgraph.xml";

//////////////////////////////////////Function Declaration/////////////////////////////////////////////////////////

DeviceGrid create_device_grid(const std::vector<t_grid_def>& grid_layouts,
                           const size_t width,
                           const size_t height,
                           const std::vector<t_physical_tile_type>& physical_tile_types);

void create_segments(RRGraphBuilder* rr_graph_builder,
                    t_chan_width& nodes_per_chan,
                    t_seg_details* seg_details_x,
                    t_seg_details* seg_details_y,
                    int& num_seg_details_x,
                    int& num_seg_details_y,
                    const std::vector<t_segment_inf>& segment_inf,
                    const DeviceGrid& grid,
                    const bool is_global_graph,
                    const bool use_full_seg_groups,
                    const e_directionality directionality);

void create_channels(DeviceGrid& grid,
                     t_chan_width* nodes_per_chan,
                     t_chan_details& chan_details_x,
                     t_chan_details& chan_details_y,
                     const int num_seg_details_x,
                     const int num_seg_details_y,
                     const t_seg_details* seg_details_x,
                     const t_seg_details* seg_details_y);

t_rr_switch_inf create_rr_switches(t_arch_switch_inf* arch_switch_inf, int arch_switch_idx);

void create_sink_src_node(RRGraphBuilder* rr_graph_builder,
                          std::vector<t_rr_rc_data>& rr_rc_data,
                          const e_rr_type node_type,
                          const e_cost_indices node_cost,
                          const int ptc,
                          const int x_coord,
                          const int y_coord);

void create_pin_node(RRGraphBuilder* rr_graph_builder,
                          std::vector<t_rr_rc_data>& rr_rc_data,
                          const e_rr_type node_type,
                          const e_cost_indices node_cost,
                          const e_side node_side,
                          const int ptc,
                          const int x_coord,
                          const int y_coord);

void create_chan_node(RRGraphBuilder* rr_graph_builder,
                          std::vector<t_rr_rc_data>& rr_rc_data,
                          const e_rr_type node_type,
                          const e_cost_indices node_cost,
                          const Direction direction,
                          const int track,
                          const int x_coord,
                          const int y_coord,
                          const int x_offset,
                          const int y_offset);

void create_rr_edges(RRGraphBuilder* rr_graph_builder);

void create_rr_indexed_data(RRGraphBuilder* rr_graph_builder,
                            RRGraphView* rr_graph,
                            vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                            const DeviceGrid& grid,
                            const std::vector<t_segment_inf>& segment_inf,
                            const int wire_to_ipin_switch);

void create_fanin_rr_switches(RRGraphBuilder* rr_graph_builder,
                              t_arch_switch_inf* arch_switch_inf,
                              t_arch_switch_fanin& arch_switch_fanins,
                              std::vector<std::map<int, int>>& switch_fanin_remap,
                              const size_t num_arch_switches);

t_chan_width init_channel(DeviceGrid grid, int chan_width_max, int x_max, int x_min, int y_max, int y_min, int info);

//////////////////////////////////////Tests/////////////////////////////////////////////////////////

TEST_CASE("I/O Test1", "[librrgraph]"){
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

    VTR_LOG("Build Grid\n");
    DeviceGrid grid = create_device_grid(arch.grid_layouts, arch.grid_layouts[0].width, arch.grid_layouts[0].height, physical_tile_types);
    VTR_LOG("... Done\n");

    t_chan_width chan_width = init_channel(grid, 2, 2, 2, 2, 2, 5);

    VTR_LOG("Build Segment\n");
    int num_seg_details_x = 0;
    int num_seg_details_y = 0;
    t_seg_details* seg_details_x = nullptr;
    t_seg_details* seg_details_y = nullptr;

    create_segments(&rr_graph_builder,
                    chan_width,
                    seg_details_x,
                    seg_details_y,
                    num_seg_details_x,
                    num_seg_details_y,
                    segment_inf,
                    grid,
                    /*is_global_graph*/ false,
                    /*use_full_seg_groups*/ false,
                    UNI_DIRECTIONAL);
    VTR_LOG("... Done\n");

    // VTR_LOG("Build Channel\n");
    // /* START CHAN_DETAILS */
    // t_chan_details chan_details_x;
    // t_chan_details chan_details_y;
    // create_channels(grid,
    //                 &chan_width,
    //                 chan_details_x,
    //                 chan_details_y,
    //                 num_seg_details_x,
    //                 num_seg_details_y,
    //                 seg_details_x,
    //                 seg_details_y);
    // VTR_LOG("... Done\n");

    VTR_LOG("Build Switch\n");
    rr_graph_builder.reserve_switches(num_arch_switches);
    // Create the switches
    for (int iswitch = 0; iswitch < num_arch_switches; ++iswitch) {
        const t_rr_switch_inf& temp_rr_switch = create_rr_switches(arch_switch_inf, iswitch);
        rr_graph_builder.add_rr_switch(temp_rr_switch);
    }
    VTR_LOG("... Done\n");
    
    VTR_LOG("Build Node\n");
    rr_graph_builder.reserve_nodes(6);
    create_sink_src_node(&rr_graph_builder, rr_rc_data, SOURCE, SOURCE_COST_INDEX, 0, 0, 0);
    create_pin_node(&rr_graph_builder, rr_rc_data, OPIN, OPIN_COST_INDEX, TOP, 0, 0, 0);
    create_sink_src_node(&rr_graph_builder, rr_rc_data, SINK, SINK_COST_INDEX, 0, 1, 0);
    create_pin_node(&rr_graph_builder, rr_rc_data, IPIN, IPIN_COST_INDEX, TOP, 0, 1, 0);
    create_chan_node(&rr_graph_builder, rr_rc_data, CHANX, CHANX_COST_INDEX_START, Direction::INC, 0, 0, 0, 0, 0);
    create_chan_node(&rr_graph_builder, rr_rc_data, CHANY, CHANX_COST_INDEX_START, Direction::DEC, 0, 0, 0, 0, 0);
    VTR_LOG("... Done\n");

    VTR_LOG("Build Edge\n");
    rr_graph_builder.reserve_edges(5);
    create_rr_edges(&rr_graph_builder);
    VTR_LOG("... Done\n");

    VTR_LOG("Build Switches_Fanin\n");
    rr_graph_builder.init_fan_in();
    size_t num_rr_switches = rr_graph_builder.count_rr_switches(num_arch_switches, arch_switch_inf, arch_switch_fanins);
    rr_graph_builder.resize_switches(num_rr_switches);
    create_fanin_rr_switches(&rr_graph_builder, arch_switch_inf, arch_switch_fanins, switch_fanin_remap, num_arch_switches);
    VTR_LOG("... Done\n");

    VTR_LOG("Edge/Switch remap\n");
    rr_graph_builder.remap_rr_node_switch_indices(arch_switch_fanins);
    VTR_LOG("... Done\n");

    VTR_LOG("Edge partition\n");
    rr_graph_builder.partition_edges();
    VTR_LOG("... Done\n");

    VTR_LOG("Build Indexed Data\n");
    create_rr_indexed_data(&rr_graph_builder, &rr_graph, rr_indexed_data, grid, segment_inf, wire_to_rr_ipin_switch);
    VTR_LOG("... Done\n");

    VTR_LOG("Write RRGraph\n");
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
                   echo_file_name);
    VTR_LOG("... Done\n");

    RRGraphBuilder rr_graph_builder2 {};
    RRGraphView rr_graph2 {rr_graph_builder2.rr_nodes(), rr_graph_builder2.node_lookup(), rr_graph_builder2.rr_node_metadata(), rr_graph_builder2.rr_edge_metadata(), rr_indexed_data, rr_rc_data, rr_graph_builder2.rr_segments(), rr_graph_builder2.rr_switch()};

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
                echo_file_name);

    VTR_LOG("Write RRGraph\n");
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
                echo_file_name);
    VTR_LOG("... Done\n");
}

// TEST_CASE("I/O Test2", "[librrgraph]"){
    
//     t_arch arch;
//     std::vector<t_physical_tile_type> physical_tile_types;
//     std::vector<t_logical_block_type> logical_block_types;

//     XmlReadArch(kArchFile, /*timing_enabled=*/false,
//                 &arch, physical_tile_types, logical_block_types);
    
//     std::vector<t_segment_inf>& segment_inf = arch.Segments;
//     DeviceGrid grid = create_device_grid(arch.grid_layouts, arch.grid_layouts[0].width, arch.grid_layouts[0].height, physical_tile_types);
//     enum e_base_cost_type base_cost_type;

//     size_t num_arch_switches = arch.num_switches;
//     t_arch_switch_inf* arch_switch_inf = arch.Switches;

//     int virtual_clock_network_root_idx{};
//     int wire_to_rr_ipin_switch={};

//     std::string read_rr_graph_filename = kRrGraphFile1;
//     char echo_file_name[] = "rr_indexed_data.echo";

//     bool echo_enabled = 1;
//     bool read_edge_metadata = 1;
//     bool do_check_rr_graph = 0;

//     t_graph_type graph_type = GRAPH_GLOBAL;
//     t_chan_width chan_width;

//     vtr::vector<RRIndexedDataId, t_rr_indexed_data> rr_indexed_data;
//     std::vector<t_rr_rc_data> rr_rc_data;

//     //Output
//     RRGraphBuilder rr_graph_builder {};
//     RRGraphView rr_graph {rr_graph_builder.rr_nodes(), rr_graph_builder.node_lookup(), rr_graph_builder.rr_node_metadata(), rr_graph_builder.rr_edge_metadata(), rr_indexed_data, rr_rc_data, rr_graph_builder.rr_segments(), rr_graph_builder.rr_switch()};

//     load_rr_file(&rr_graph_builder,
//                 &rr_graph,
//                 physical_tile_types,
//                 segment_inf,
//                 &rr_indexed_data,
//                 &rr_rc_data,
//                 grid,
//                 arch_switch_inf,
//                 graph_type,
//                 &arch,
//                 &chan_width,
//                 base_cost_type,
//                 num_arch_switches,
//                 virtual_clock_network_root_idx,
//                 &wire_to_rr_ipin_switch,
//                 read_rr_graph_filename.c_str(),
//                 &read_rr_graph_filename,
//                 read_edge_metadata,
//                 do_check_rr_graph,
//                 echo_enabled,
//                 echo_file_name);

//     VTR_LOG("Write RRGraph\n");
//     write_rr_graph(&rr_graph_builder,
//                 &rr_graph,
//                 physical_tile_types,
//                 &rr_indexed_data,
//                 &rr_rc_data,
//                 grid,
//                 arch_switch_inf,
//                 &arch,
//                 &chan_width,
//                 num_arch_switches,
//                 kRrGraphFile2,
//                 virtual_clock_network_root_idx,
//                 echo_enabled,
//                 echo_file_name);
//     VTR_LOG("... Done\n");
    
// }

//////////////////////////////////////Function Definition/////////////////////////////////////////////////////////

DeviceGrid create_device_grid(const std::vector<t_grid_def>& grid_layouts,
                              const size_t width,
                              const size_t height,
                              const std::vector<t_physical_tile_type>& physical_tile_types){

    FormulaParser p;
    auto grid = vtr::Matrix<t_grid_tile>({width, height});
    auto grid_priorities = vtr::Matrix<int>({width, height}, std::numeric_limits<int>::lowest());
    const auto& grid_def = grid_layouts[0];

    //Initialize the device to all empty blocks
    t_physical_tile_type type = get_empty_physical_type();
    t_physical_tile_type_ptr empty_type = &type;
    VTR_ASSERT(empty_type != nullptr);

    for (size_t x = 0; x < width; ++x) {
        for (size_t y = 0; y < height; ++y) {
            set_grid_block_type(std::numeric_limits<int>::lowest() + 1, //+1 so it overrides without warning
                                empty_type, x, y, grid, grid_priorities, /*meta=*/nullptr);
        }
    }

    //Fill in non-empty blocks
    for (const auto& grid_loc_def : grid_def.loc_defs) {
        auto& meta = grid_loc_def.meta;
        for (const auto& tile_type : physical_tile_types) {
            if (tile_type.name == grid_loc_def.block_type) {
                auto type = &tile_type;
                t_formula_data vars;
                vars.set_var_value("W", width);
                vars.set_var_value("H", height);
                vars.set_var_value("w", type->width);
                vars.set_var_value("h", type->height);

                size_t startx = p.parse_formula(grid_loc_def.x.start_expr, vars);
                size_t starty = p.parse_formula(grid_loc_def.y.start_expr, vars);
                for (size_t x = startx; x < startx + type->width; ++x){
                    size_t x_offset = x - startx;
                    for (size_t y = starty; y < starty + type->height; ++y) {
                        size_t y_offset = y - starty;
                        grid[x][y].type = type;
                        grid[x][y].width_offset = x_offset;
                        grid[x][y].height_offset = y_offset;
                        grid[x][y].meta = meta;
                    }
                }
                break;
            }
        }
    }
    return DeviceGrid(grid_def.name, grid);
}

t_chan_width init_channel(DeviceGrid grid, int chan_width_max, int x_max, int x_min, int y_max, int y_min, int info){
    t_chan_width chan_width;

    chan_width.max = chan_width_max;
    chan_width.max = x_min;
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

void create_segments(RRGraphBuilder* rr_graph_builder,
                    t_chan_width& nodes_per_chan,
                    t_seg_details* seg_details_x,
                    t_seg_details* seg_details_y,
                    int& num_seg_details_x,
                    int& num_seg_details_y,
                    const std::vector<t_segment_inf>& segment_inf,
                    const DeviceGrid& grid,
                    const bool is_global_graph,
                    const bool use_full_seg_groups,
                    const e_directionality directionality){

    int max_chan_width_x = nodes_per_chan.x_max = (is_global_graph ? 1 : nodes_per_chan.x_max);
    int max_chan_width_y = nodes_per_chan.y_max = (is_global_graph ? 1 : nodes_per_chan.y_max);

    size_t num_segments = segment_inf.size();
    rr_graph_builder->reserve_segments(num_segments);
    for (size_t iseg = 0; iseg < num_segments; ++iseg) {
        rr_graph_builder->add_rr_segment(segment_inf[iseg]);
    }

    t_unified_to_parallel_seg_index segment_index_map;
    std::vector<t_segment_inf> segment_inf_x = get_parallel_segs(segment_inf, segment_index_map, X_AXIS);
    std::vector<t_segment_inf> segment_inf_y = get_parallel_segs(segment_inf, segment_index_map, Y_AXIS);

    // seg_details_x = alloc_and_load_global_route_seg_details(0, &num_seg_details_x);
    // seg_details_y = alloc_and_load_global_route_seg_details(0, &num_seg_details_y);

    /* Setup segments including distrubuting tracks and staggering.
        * If use_full_seg_groups is specified, max_chan_width may be
        * changed. Warning should be singled to caller if this happens. */

    /* Need to setup segments along x & y axis seperately, due to different 
        * max_channel_widths and segment specifications. */

    size_t max_dim = std::max(grid.width(), grid.height()) - 2; //-2 for no perim channels

    /*Get x & y segments seperately*/
    seg_details_x = alloc_and_load_seg_details(&max_chan_width_x,
                                                max_dim, segment_inf_x,
                                                use_full_seg_groups, directionality,
                                                &num_seg_details_x);

    seg_details_y = alloc_and_load_seg_details(&max_chan_width_y,
                                                max_dim, segment_inf_y,
                                                use_full_seg_groups, directionality,
                                                &num_seg_details_y);

}

void create_channels(DeviceGrid& grid,
                     t_chan_width* chan_width,
                     t_chan_details& chan_details_x,
                     t_chan_details& chan_details_y,
                     const int num_seg_details_x,
                     const int num_seg_details_y,
                     const t_seg_details* seg_details_x,
                     const t_seg_details* seg_details_y){

    chan_details_x = init_chan_details(grid, chan_width,
                                       num_seg_details_x, seg_details_x, X_AXIS);
    chan_details_y = init_chan_details(grid, chan_width,
                                       num_seg_details_y, seg_details_y, Y_AXIS);

    /* Adjust segment start/end based on obstructed channels, if any */
    adjust_chan_details(grid, chan_width,
                        chan_details_x, chan_details_y);

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

void create_sink_src_node(RRGraphBuilder* rr_graph_builder,
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
}

void create_pin_node(RRGraphBuilder* rr_graph_builder,
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

        rr_graph_builder->node_lookup().add_node(node_id, x_coord, y_coord, node_type, ptc);

}

void create_chan_node(RRGraphBuilder* rr_graph_builder,
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

}

void create_rr_edges(RRGraphBuilder* rr_graph_builder){
    t_rr_edge_info_set rr_edges_to_create;
    rr_edges_to_create.emplace_back(RRNodeId(0), RRNodeId(1), 0);
    rr_edges_to_create.emplace_back(RRNodeId(1), RRNodeId(4), 0);
    rr_edges_to_create.emplace_back(RRNodeId(4), RRNodeId(5), 0);
    rr_edges_to_create.emplace_back(RRNodeId(5), RRNodeId(3), 0);
    rr_edges_to_create.emplace_back(RRNodeId(3), RRNodeId(2), 0);
    rr_graph_builder->alloc_and_load_edges(&rr_edges_to_create);
}

void create_fanin_rr_switches(RRGraphBuilder* rr_graph_builder,
                              t_arch_switch_inf* arch_switch_inf,
                              t_arch_switch_fanin& arch_switch_fanins,
                              std::vector<std::map<int, int>>& switch_fanin_remap,
                              const size_t num_arch_switches){
    if (!switch_fanin_remap.empty()) {
        // at this stage, we rebuild the rr_graph (probably in binary search)
        // so old switch_fanin_remap is obsolete
        switch_fanin_remap.clear();
    }

    switch_fanin_remap.resize(num_arch_switches);
    for (int i_arch_switch = 0; i_arch_switch < num_arch_switches; i_arch_switch++) {
        std::map<int, int>::iterator it;
        for (auto fanin_rrswitch : arch_switch_fanins[i_arch_switch]) {
            /* the fanin value is in it->first, and we'll need to set what index this i_arch_switch/fanin
             * combination maps to (within rr_switch_inf) in it->second) */
            int fanin;
            int i_rr_switch;
            std::tie(fanin, i_rr_switch) = fanin_rrswitch;

            // setup switch_fanin_remap, for future swich usage analysis
            switch_fanin_remap[i_arch_switch][fanin] = i_rr_switch;

            load_rr_switch_from_arch_switch(rr_graph_builder, arch_switch_inf, i_arch_switch, i_rr_switch, fanin, 8926, 16067);
        }
    }
}

void create_rr_indexed_data(RRGraphBuilder* rr_graph_builder,
                            RRGraphView* rr_graph,
                            vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                            const DeviceGrid& grid,
                            const std::vector<t_segment_inf>& segment_inf,
                            const int wire_to_ipin_switch
                            ){

    int i, length, index;

    VTR_LOG("segment_inf size: %d\n", segment_inf.size());

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