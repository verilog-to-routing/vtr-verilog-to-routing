/**
 * @file
 * @author  Alex Singer
 * @date    April 2025
 * @brief   Implementation of the pre-cluster timing manager class.
 */

#include "PreClusterTimingManager.h"
#include <algorithm>
#include <memory>
#include "PreClusterDelayCalculator.h"
#include "PreClusterTimingGraphResolver.h"
#include "SetupGrid.h"
#include "atom_lookup.h"
#include "atom_netlist.h"
#include "atom_netlist_fwd.h"
#include "concrete_timing_info.h"
#include "physical_types_util.h"
#include "prepack.h"
#include "tatum/TimingReporter.hpp"
#include "tatum/echo_writer.hpp"
#include "vpr_types.h"
#include "vtr_assert.h"
#include "vtr_time.h"

/**
 * Since the parameters of a switch may change as a function of its fanin,
 * to get an estimation of inter-cluster delays we need a reasonable estimation
 * of the fan-ins of switches that connect clusters together. These switches are
 * 1) opin to wire switch
 * 2) wire to wire switch
 * 3) wire to ipin switch
 * We can estimate the fan-in of these switches based on the Fc_in/Fc_out of
 * a logic block, and the switch block Fs value
 */
static void get_intercluster_switch_fanin_estimates(const t_arch& arch,
                                                    const t_det_routing_arch& routing_arch,
                                                    const std::string& device_layout,
                                                    const int wire_segment_length,
                                                    int* opin_switch_fanin,
                                                    int* wire_switch_fanin,
                                                    int* ipin_switch_fanin);

static float get_arch_switch_info(short switch_index, int switch_fanin, float& Tdel_switch, float& R_switch, float& Cout_switch);

static float approximate_inter_cluster_delay(const t_arch& arch,
                                             const t_det_routing_arch& routing_arch,
                                             const std::string& device_layout);

PreClusterTimingManager::PreClusterTimingManager(bool timing_driven,
                                                 const AtomNetlist& atom_netlist,
                                                 const AtomLookup& atom_lookup,
                                                 const Prepacker& prepacker,
                                                 e_timing_update_type timing_update_type,
                                                 const t_arch& arch,
                                                 const t_det_routing_arch& routing_arch,
                                                 const std::string& device_layout,
                                                 const t_analysis_opts& analysis_opts) {

    // If the flow is not timing driven, do not initialize any of the timing
    // objects and set the valid flag to false. This allows this object to be
    // passed through the VPR flow when timing is turned off.
    if (!timing_driven) {
        is_valid_ = false;
        return;
    }
    is_valid_ = true;

    // Start an overall timer for building the pre-cluster timing info.
    vtr::ScopedStartFinishTimer timer("Initializing Pre-Cluster Timing");

    // Approximate the inter-cluster delay
    // FIXME: This can probably be simplified. It can also be improved using
    //        AP information.
    float inter_cluster_net_delay = approximate_inter_cluster_delay(arch, routing_arch, device_layout);
    VTR_LOG("Using inter-cluster delay: %g\n", inter_cluster_net_delay);

    // Initialize the timing analyzer
    clustering_delay_calc_ = std::make_shared<PreClusterDelayCalculator>(atom_netlist,
                                                                         atom_lookup,
                                                                         inter_cluster_net_delay,
                                                                         prepacker);
    timing_info_ = make_setup_timing_info(clustering_delay_calc_, timing_update_type);

    // Calculate the initial timing
    timing_info_->update();

    // Create the echo file if requested.
    if (isEchoFileEnabled(E_ECHO_PRE_PACKING_TIMING_GRAPH)) {
        auto& timing_ctx = g_vpr_ctx.timing();
        tatum::write_echo(getEchoFileName(E_ECHO_PRE_PACKING_TIMING_GRAPH),
                          *timing_ctx.graph, *timing_ctx.constraints, *clustering_delay_calc_, timing_info_->analyzer());

        tatum::NodeId debug_tnode = id_or_pin_name_to_tnode(analysis_opts.echo_dot_timing_graph_node);
        write_setup_timing_graph_dot(getEchoFileName(E_ECHO_PRE_PACKING_TIMING_GRAPH) + std::string(".dot"),
                                     *timing_info_, debug_tnode);
    }

    // Write a timing report.
    {
        auto& timing_ctx = g_vpr_ctx.timing();
        PreClusterTimingGraphResolver resolver(atom_netlist,
                                               atom_lookup,
                                               *timing_ctx.graph,
                                               *clustering_delay_calc_);
        resolver.set_detail_level(analysis_opts.timing_report_detail);

        tatum::TimingReporter timing_reporter(resolver, *timing_ctx.graph,
                                              *timing_ctx.constraints);

        timing_reporter.report_timing_setup(
            "pre_pack.report_timing.setup.rpt",
            *timing_info_->setup_analyzer(),
            analysis_opts.timing_report_npaths);
    }
}

static float approximate_inter_cluster_delay(const t_arch& arch,
                                             const t_det_routing_arch& routing_arch,
                                             const std::string& device_layout) {

    /* If needed, estimate inter-cluster delay. Assume the average routing hop goes out of
     * a block through an opin switch to a length-4 wire, then through a wire switch to another
     * length-4 wire, then through a wire-to-ipin-switch into another block. */
    constexpr int wire_segment_length = 4;

    /* We want to determine a reasonable fan-in to the opin, wire, and ipin switches, based
     * on which the intercluster delays can be estimated. The fan-in of a switch influences its
     * delay.
     *
     * The fan-in of the switch depends on the architecture (unidirectional/bidirectional), as
     * well as Fc_in/out and Fs */
    int opin_switch_fanin, wire_switch_fanin, ipin_switch_fanin;
    get_intercluster_switch_fanin_estimates(arch, routing_arch, device_layout, wire_segment_length, &opin_switch_fanin,
                                            &wire_switch_fanin, &ipin_switch_fanin);

    float Tdel_opin_switch, R_opin_switch, Cout_opin_switch;
    float opin_switch_del = get_arch_switch_info(arch.Segments[0].arch_opin_switch, opin_switch_fanin,
                                                 Tdel_opin_switch, R_opin_switch, Cout_opin_switch);

    float Tdel_wire_switch, R_wire_switch, Cout_wire_switch;
    float wire_switch_del = get_arch_switch_info(arch.Segments[0].arch_wire_switch, wire_switch_fanin,
                                                 Tdel_wire_switch, R_wire_switch, Cout_wire_switch);

    float Tdel_wtoi_switch, R_wtoi_switch, Cout_wtoi_switch;
    float wtoi_switch_del = get_arch_switch_info(routing_arch.wire_to_arch_ipin_switch, ipin_switch_fanin,
                                                 Tdel_wtoi_switch, R_wtoi_switch, Cout_wtoi_switch);

    float Rmetal = arch.Segments[0].Rmetal;
    float Cmetal = arch.Segments[0].Cmetal;

    /* The delay of a wire with its driving switch is the switch delay plus the
     * product of the equivalent resistance and capacitance experienced by the wire. */

    float first_wire_seg_delay = opin_switch_del
                                 + (R_opin_switch + Rmetal * (float)wire_segment_length / 2)
                                       * (Cout_opin_switch + Cmetal * (float)wire_segment_length);
    float second_wire_seg_delay = wire_switch_del
                                  + (R_wire_switch + Rmetal * (float)wire_segment_length / 2)
                                        * (Cout_wire_switch + Cmetal * (float)wire_segment_length);

    /* multiply by 4 to get a more conservative estimate */
    return 4 * (first_wire_seg_delay + second_wire_seg_delay + wtoi_switch_del);
}

static float get_arch_switch_info(short switch_index, int switch_fanin, float& Tdel_switch, float& R_switch, float& Cout_switch) {
    /* Fetches delay, resistance and output capacitance of the architecture switch at switch_index.
     * Returns the total delay through the switch. Used to calculate inter-cluster net delay. */

    /* The intrinsic delay may depend on fanin to the switch. If the delay map of a
     * switch from the architecture file has multiple (#inputs, delay) entries, we
     * interpolate/extrapolate to get the delay at 'switch_fanin'. */
    auto& device_ctx = g_vpr_ctx.device();

    Tdel_switch = device_ctx.arch_switch_inf[switch_index].Tdel(switch_fanin);
    R_switch = device_ctx.arch_switch_inf[switch_index].R;
    Cout_switch = device_ctx.arch_switch_inf[switch_index].Cout;

    /* The delay through a loaded switch is its intrinsic (unloaded)
     * delay plus the product of its resistance and output capacitance. */
    return Tdel_switch + R_switch * Cout_switch;
}

static void get_intercluster_switch_fanin_estimates(const t_arch& arch,
                                                    const t_det_routing_arch& routing_arch,
                                                    const std::string& device_layout,
                                                    const int wire_segment_length,
                                                    int* opin_switch_fanin,
                                                    int* wire_switch_fanin,
                                                    int* ipin_switch_fanin) {
    // W is unknown pre-packing, so *if* we need W here, we will assume a value of 100
    constexpr int W = 100;

    //Build a dummy 10x10 device to determine the 'best' block type to use
    auto grid = create_device_grid(device_layout, arch.grid_layouts, 10, 10);

    auto type = find_most_common_tile_type(grid);
    /* get Fc_in/out for most common block (e.g. logic blocks) */
    VTR_ASSERT(!type->fc_specs.empty());

    //Estimate the maximum Fc_in/Fc_out
    float Fc_in = 0.f;
    float Fc_out = 0.f;
    for (const t_fc_specification& fc_spec : type->fc_specs) {
        float Fc = fc_spec.fc_value;

        if (fc_spec.fc_value_type == e_fc_value_type::ABSOLUTE) {
            //Convert to estimated fractional
            Fc /= W;
        }
        VTR_ASSERT_MSG(Fc >= 0 && Fc <= 1., "Fc should be fractional");

        for (int ipin : fc_spec.pins) {
            e_pin_type pin_type = get_pin_type_from_pin_physical_num(type, ipin);

            if (pin_type == DRIVER) {
                Fc_out = std::max(Fc, Fc_out);
            } else {
                VTR_ASSERT(pin_type == RECEIVER);
                Fc_in = std::max(Fc, Fc_in);
            }
        }
    }

    /* Estimates of switch fan-in are done as follows:
     * 1) opin to wire switch:
     * 2 CLBs connect to a channel, each with #opins/4 pins. Each pin has Fc_out*W
     * switches, and then we assume the switches are distributed evenly over the W wires.
     * In the unidirectional case, all these switches are then crammed down to W/wire_segment_length wires.
     *
     * Unidirectional: 2 * #opins_per_side * Fc_out * wire_segment_length
     * Bidirectional:  2 * #opins_per_side * Fc_out
     *
     * 2) wire to wire switch
     * A wire segment in a switchblock connects to Fs other wires. Assuming these connections are evenly
     * distributed, each target wire receives Fs connections as well. In the unidirectional case,
     * source wires can only connect to W/wire_segment_length wires.
     *
     * Unidirectional: Fs * wire_segment_length
     * Bidirectional:  Fs
     *
     * 3) wire to ipin switch
     * An input pin of a CLB simply receives Fc_in connections.
     *
     * Unidirectional: Fc_in
     * Bidirectional:  Fc_in
     */

    /* Fan-in to opin/ipin/wire switches depends on whether the architecture is unidirectional/bidirectional */
    (*opin_switch_fanin) = 2.f * type->num_drivers / 4.f * Fc_out;
    (*wire_switch_fanin) = routing_arch.Fs;
    (*ipin_switch_fanin) = Fc_in;
    if (routing_arch.directionality == UNI_DIRECTIONAL) {
        /* adjustments to opin-to-wire and wire-to-wire switch fan-ins */
        (*opin_switch_fanin) *= wire_segment_length;
        (*wire_switch_fanin) *= wire_segment_length;
    } else if (routing_arch.directionality == BI_DIRECTIONAL) {
        /* no adjustments need to be made here */
    } else {
        VPR_FATAL_ERROR(VPR_ERROR_PACK, "Unrecognized directionality: %d\n",
                        (int)routing_arch.directionality);
    }
}

float PreClusterTimingManager::calc_atom_setup_criticality(AtomBlockId blk_id,
                                                           const AtomNetlist& atom_netlist) const {
    VTR_ASSERT_SAFE_MSG(is_valid_,
                        "PreClusterTimingManager has not been initialized");
    VTR_ASSERT_SAFE_MSG(blk_id.is_valid(),
                        "Invalid block ID");

    float crit = 0.0f;
    for (AtomPinId in_pin : atom_netlist.block_input_pins(blk_id)) {
        // Max criticality over incoming nets
        float pin_crit = timing_info_->setup_pin_criticality(in_pin);
        crit = std::max(crit, pin_crit);
    }

    return crit;
}

float PreClusterTimingManager::calc_net_setup_criticality(AtomNetId net_id,
                                                          const AtomNetlist& atom_netlist) const {
    VTR_ASSERT_SAFE_MSG(is_valid_,
                        "PreClusterTimingManager has not been initialized");
    VTR_ASSERT_SAFE_MSG(net_id.is_valid(),
                        "Invalid net ID");

    // We let the criticality of an entire net to be the max criticality of all
    // timing edges within the net. Since all timing paths start at the driver,
    // this is equivalent to the criticality of the driver pin.
    AtomPinId net_driver_pin_id = atom_netlist.net_driver(net_id);
    VTR_ASSERT_SAFE_MSG(net_driver_pin_id.is_valid(),
                        "Net has no driver");
    return timing_info_->setup_pin_criticality(net_driver_pin_id);
}
