
#include "rr_graph_switch_utils.h"

#include <ranges>

#include "rr_graph_builder.h"
#include "rr_graph_area.h"
#include "vpr_error.h"
#include "vpr_types.h"

//
// Static Function Declarations
//

/**
 * @brief Populate rr_switch entries for all fanin variants.
 *
 * Iterates over each architecture switch and its fanins, creates the
 * corresponding rr_switch entries, and fills the fanin --> rr_switch mapping.
 */
static void load_rr_switch_inf(RRGraphBuilder& rr_graph_builder,
                               t_arch_switch_fanin& switch_fanin_remap,
                               const std::map<int, t_arch_switch_inf>& arch_sw_inf,
                               const float R_minW_nmos,
                               const float R_minW_pmos,
                               const t_arch_switch_fanin& arch_switch_fanins);

/**
 * @brief Allocate rr_switch storage and determine fanin variants.
 *
 * Builds the list of fanins observed for each architecture switch and resizes
 * the rr_switch array accordingly.
 */
static void alloc_rr_switch_inf(RRGraphBuilder& rr_graph_builder,
                                t_arch_switch_fanin& arch_switch_fanins,
                                const std::map<int, t_arch_switch_inf>& arch_sw_map);

//
// Static Function Definitions
//

static void load_rr_switch_inf(RRGraphBuilder& rr_graph_builder,
                               t_arch_switch_fanin& switch_fanin_remap,
                               const std::map<int, t_arch_switch_inf>& arch_sw_inf,
                               const float R_minW_nmos,
                               const float R_minW_pmos,
                               const t_arch_switch_fanin& arch_switch_fanins) {
    switch_fanin_remap.clear();
    switch_fanin_remap.resize(arch_sw_inf.size());

    for (const int arch_sw_id : arch_sw_inf | std::views::keys) {
        for (auto fanin_rrswitch : arch_switch_fanins[arch_sw_id]) {
            // the fanin value is in it->first, and we'll need to set what index this i_arch_switch/fanin
            // combination maps to (within rr_switch_inf) in it->second)
            int fanin;
            RRSwitchId i_rr_switch;
            std::tie(fanin, i_rr_switch) = fanin_rrswitch;

            // setup device_ctx.switch_fanin_remap, for future swich usage analysis
            switch_fanin_remap[arch_sw_id][fanin] = i_rr_switch;

            load_rr_switch_from_arch_switch(rr_graph_builder,
                                            arch_sw_inf,
                                            arch_sw_id,
                                            i_rr_switch,
                                            fanin,
                                            R_minW_nmos,
                                            R_minW_pmos);
        }
    }
}

static void alloc_rr_switch_inf(RRGraphBuilder& rr_graph_builder,
                                t_arch_switch_fanin& arch_switch_fanins,
                                const std::map<int, t_arch_switch_inf>& arch_sw_map) {
    std::vector<t_arch_switch_inf> all_sw_inf(arch_sw_map.size());
    for (const auto& map_it : arch_sw_map) {
        all_sw_inf[map_it.first] = map_it.second;
    }
    size_t num_rr_switches = rr_graph_builder.count_rr_switches(
        all_sw_inf,
        arch_switch_fanins);
    rr_graph_builder.resize_switches(num_rr_switches);
}

//
// Non-static Function Definitions
//

void load_rr_switch_from_arch_switch(RRGraphBuilder& rr_graph_builder,
                                     const std::map<int, t_arch_switch_inf>& arch_sw_inf,
                                     int arch_switch_idx,
                                     RRSwitchId rr_switch_idx,
                                     int fanin,
                                     const float R_minW_nmos,
                                     const float R_minW_pmos) {
    // figure out, by looking at the arch switch's Tdel map, what the delay of the new rr switch should be
    double rr_switch_Tdel = arch_sw_inf.at(arch_switch_idx).Tdel(fanin);

    // copy over the arch switch to rr_switch_inf[rr_switch_idx], but with the changed Tdel value
    rr_graph_builder.rr_switch()[rr_switch_idx].set_type(arch_sw_inf.at(arch_switch_idx).type());
    rr_graph_builder.rr_switch()[rr_switch_idx].R = arch_sw_inf.at(arch_switch_idx).R;
    rr_graph_builder.rr_switch()[rr_switch_idx].Cin = arch_sw_inf.at(arch_switch_idx).Cin;
    rr_graph_builder.rr_switch()[rr_switch_idx].Cinternal = arch_sw_inf.at(arch_switch_idx).Cinternal;
    rr_graph_builder.rr_switch()[rr_switch_idx].Cout = arch_sw_inf.at(arch_switch_idx).Cout;
    rr_graph_builder.rr_switch()[rr_switch_idx].Tdel = rr_switch_Tdel;
    rr_graph_builder.rr_switch()[rr_switch_idx].mux_trans_size = arch_sw_inf.at(arch_switch_idx).mux_trans_size;
    if (arch_sw_inf.at(arch_switch_idx).buf_size_type == e_buffer_size::AUTO) {
        // Size based on resistance
        rr_graph_builder.rr_switch()[rr_switch_idx].buf_size = trans_per_buf(arch_sw_inf.at(arch_switch_idx).R, R_minW_nmos, R_minW_pmos);
    } else {
        VTR_ASSERT(arch_sw_inf.at(arch_switch_idx).buf_size_type == e_buffer_size::ABSOLUTE);
        // Use the specified size
        rr_graph_builder.rr_switch()[rr_switch_idx].buf_size = arch_sw_inf.at(arch_switch_idx).buf_size;
    }
    rr_graph_builder.rr_switch()[rr_switch_idx].name = arch_sw_inf.at(arch_switch_idx).name;
    rr_graph_builder.rr_switch()[rr_switch_idx].power_buffer_type = arch_sw_inf.at(arch_switch_idx).power_buffer_type;
    rr_graph_builder.rr_switch()[rr_switch_idx].power_buffer_size = arch_sw_inf.at(arch_switch_idx).power_buffer_size;
}

t_rr_switch_inf create_rr_switch_from_arch_switch(const t_arch_switch_inf& arch_sw_inf,
                                                  const float R_minW_nmos,
                                                  const float R_minW_pmos) {
    t_rr_switch_inf rr_switch_inf;

    // figure out, by looking at the arch switch's Tdel map, what the delay of the new rr switch should be
    double rr_switch_Tdel = arch_sw_inf.Tdel(0);

    // copy over the arch switch to rr_switch_inf[rr_switch_idx], but with the changed Tdel value
    rr_switch_inf.set_type(arch_sw_inf.type());
    rr_switch_inf.R = arch_sw_inf.R;
    rr_switch_inf.Cin = arch_sw_inf.Cin;
    rr_switch_inf.Cinternal = arch_sw_inf.Cinternal;
    rr_switch_inf.Cout = arch_sw_inf.Cout;
    rr_switch_inf.Tdel = rr_switch_Tdel;
    rr_switch_inf.mux_trans_size = arch_sw_inf.mux_trans_size;
    if (arch_sw_inf.buf_size_type == e_buffer_size::AUTO) {
        // Size based on resistance
        rr_switch_inf.buf_size = trans_per_buf(arch_sw_inf.R, R_minW_nmos, R_minW_pmos);
    } else {
        VTR_ASSERT(arch_sw_inf.buf_size_type == e_buffer_size::ABSOLUTE);
        // Use the specified size
        rr_switch_inf.buf_size = arch_sw_inf.buf_size;
    }
    rr_switch_inf.name = arch_sw_inf.name;
    rr_switch_inf.power_buffer_type = arch_sw_inf.power_buffer_type;
    rr_switch_inf.power_buffer_size = arch_sw_inf.power_buffer_size;

    rr_switch_inf.intra_tile = arch_sw_inf.intra_tile;

    return rr_switch_inf;
}

/* Allocates and loads the global rr_switch_inf array based on the global
 * arch_switch_inf array and the fan-ins used by the rr nodes.
 * Also changes switch indices of rr_nodes to index into rr_switch_inf
 * instead of arch_switch_inf.
 *
 * Returns the number of rr switches created.
 * Also returns, through a pointer, the index of a representative ipin cblock switch.
 * - Currently we're not allowing a designer to specify an ipin cblock switch with
 * multiple fan-ins, so there's just one of these switches in the device_ctx.rr_switch_inf array.
 * But in the future if we allow this, we can return an index to a representative switch
 *
 * The rr_switch_inf switches are derived from the arch_switch_inf switches
 * (which were read-in from the architecture file) based on fan-in. The delays of
 * the rr switches depend on their fan-in, so we first go through the rr_nodes
 * and count how many different fan-ins exist for each arch switch.
 * Then we create these rr switches and update the switch indices
 * of rr_nodes to index into the rr_switch_inf array. */
void alloc_and_load_rr_switch_inf(RRGraphBuilder& rr_graph_builder,
                                  t_arch_switch_fanin& switch_fanin_remap,
                                  const std::map<int, t_arch_switch_inf>& arch_sw_inf,
                                  const float R_minW_nmos,
                                  const float R_minW_pmos,
                                  const int wire_to_arch_ipin_switch,
                                  RRSwitchId& wire_to_rr_ipin_switch) {
    // we will potentially be creating a couple of versions of each arch switch where
    // each version corresponds to a different fan-in. We will need to fill device_ctx.rr_switch_inf
    // with this expanded list of switches.
    //
    // To do this we will use arch_switch_fanins, which is indexed as:
    //      arch_switch_fanins[i_arch_switch][fanin] -> new_switch_id
    t_arch_switch_fanin arch_switch_fanins(arch_sw_inf.size());

    // Determine what the different fan-ins are for each arch switch, and also
    // how many entries the rr_switch_inf array should have
    alloc_rr_switch_inf(rr_graph_builder,
                        arch_switch_fanins,
                        arch_sw_inf);

    // create the rr switches. also keep track of, for each arch switch, what index of the rr_switch_inf
    // array each version of its fanin has been mapped to
    load_rr_switch_inf(rr_graph_builder,
                       switch_fanin_remap,
                       arch_sw_inf,
                       R_minW_nmos,
                       R_minW_pmos,
                       arch_switch_fanins);

    // next, walk through rr nodes again and remap their switch indices to rr_switch_inf
    rr_graph_builder.remap_rr_node_switch_indices(arch_switch_fanins);

    // now we need to set the wire_to_rr_ipin_switch variable which points the detailed routing architecture
    // to the representative ipin cblock switch. currently we're not allowing the specification of an ipin cblock switch
    // with multiple fan-ins, so right now there's just one. May change in the future, in which case we'd need to
    // return a representative switch
    if (arch_switch_fanins[wire_to_arch_ipin_switch].count(UNDEFINED)) {
        // only have one ipin cblock switch. OK.
        wire_to_rr_ipin_switch = arch_switch_fanins[wire_to_arch_ipin_switch][UNDEFINED];
    } else if (arch_switch_fanins[wire_to_arch_ipin_switch].size() != 0) {
        VPR_FATAL_ERROR(VPR_ERROR_ARCH,
                        "Not currently allowing an ipin cblock switch to have multiple fan-ins");
    } else {
        // This likely indicates that no connection block has been constructed, indicating significant issues with
        // the generated RR graph.
        // Instead of throwing an error we issue a warning. This means that check_rr_graph() etc. will run to give more information
        // and allow graphics to be brought up for users to debug their architectures.
        wire_to_rr_ipin_switch = RRSwitchId::INVALID();
        VTR_LOG_WARN("No switch found for the ipin cblock in RR graph. Check if there is an error in arch file, or if no connection blocks are being built in RR graph\n");
    }
}
