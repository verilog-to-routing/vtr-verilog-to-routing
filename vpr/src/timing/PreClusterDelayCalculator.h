#pragma once

#include "netlist_fwd.h"
#include "vtr_assert.h"

#include "tatum/Time.hpp"
#include "tatum/delay_calc/DelayCalculator.hpp"
#include "tatum/TimingGraph.hpp"

#include "vpr_error.h"
#include "vpr_utils.h"

#include "atom_netlist.h"
#include "atom_lookup.h"
#include "logic_types.h"
#include "pb_type_graph.h"
#include "physical_types.h"
#include "prepack.h"
#include "vtr_vector.h"

class LogicalModels;

class PreClusterDelayCalculator : public tatum::DelayCalculator {
  public:
    PreClusterDelayCalculator(const AtomNetlist& netlist,
                              const AtomLookup& netlist_lookup,
                              const LogicalModels& models,
                              const vtr::vector<AtomPinId, float>& timing_arc_delays,
                              const Prepacker& prepacker) noexcept
        : netlist_(netlist)
        , netlist_lookup_(netlist_lookup)
        , models_(models)
        , timing_arc_delays_(timing_arc_delays)
        , prepacker_(prepacker) {

        // Timing arcs are uniquely identified by sink pins, ensure that every
        // timing arc delay has an entry for each pin in the atom netlist.
        VTR_ASSERT(timing_arc_delays.size() == netlist_.pins().size());
    }

    tatum::Time max_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const override {
        tatum::NodeId src_node = tg.edge_src_node(edge_id);
        tatum::NodeId sink_node = tg.edge_sink_node(edge_id);

        auto edge_type = tg.edge_type(edge_id);

        if (edge_type == tatum::EdgeType::PRIMITIVE_COMBINATIONAL) {
            return prim_comb_delay(tg, src_node, sink_node);
        } else if (edge_type == tatum::EdgeType::PRIMITIVE_CLOCK_LAUNCH) {
            return prim_tcq_delay(tg, src_node, sink_node);
        } else {
            VTR_ASSERT(edge_type == tatum::EdgeType::INTERCONNECT);

            // Get the sink pin for this timing edge. This is used to get the
            // delay for the timing arc that goes through this sink pin.
            AtomPinId atom_sink_pin = netlist_lookup_.tnode_atom_pin(sink_node);
            VTR_ASSERT_SAFE(atom_sink_pin.is_valid());
            VTR_ASSERT_SAFE(netlist_.pin_type(atom_sink_pin) == PinType::SINK);

            // If the source and sink atoms belong to the same molecule they will
            // be packed into the same cluster, so the inter-cluster delay is a
            // significant overestimate. Use a more accurate intra-cluster delay
            // derived from the pb_graph hierarchy instead.
            AtomPinId atom_src_pin = netlist_lookup_.tnode_atom_pin(src_node);
            if (atom_src_pin.is_valid()) {
                AtomBlockId src_blk = netlist_.pin_block(atom_src_pin);
                AtomBlockId sink_blk = netlist_.pin_block(atom_sink_pin);

                PackMoleculeId src_mol = prepacker_.get_atom_molecule(src_blk);
                PackMoleculeId sink_mol = prepacker_.get_atom_molecule(sink_blk);

                if (src_mol == sink_mol) {
                    return calc_intra_molecule_delay(atom_src_pin, atom_sink_pin);
                }

                // If the source and sink atoms belong to different molecules
                // that are part of the same chain (e.g. a long carry chain
                // split across multiple clusters), the connection uses
                // dedicated chain wiring between clusters rather than
                // general-purpose inter-cluster routing.
                const t_pack_molecule& src_mol_info = prepacker_.get_molecule(src_mol);
                const t_pack_molecule& sink_mol_info = prepacker_.get_molecule(sink_mol);
                if (src_mol_info.chain_id.is_valid() && src_mol_info.chain_id == sink_mol_info.chain_id) {
                    return calc_inter_molecule_chain_delay(atom_src_pin, atom_sink_pin);
                }
            }

            // External net delay
            return tatum::Time(timing_arc_delays_[atom_sink_pin]);
        }
    }

    tatum::Time setup_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const override {
        tatum::NodeId src_node = tg.edge_src_node(edge_id);
        tatum::NodeId sink_node = tg.edge_sink_node(edge_id);
        auto edge_type = tg.edge_type(edge_id);

        VTR_ASSERT_MSG(tg.node_type(src_node) == tatum::NodeType::CPIN, "Edge setup time only valid if source node is a CPIN");
        VTR_ASSERT_MSG(tg.node_type(sink_node) == tatum::NodeType::SINK, "Edge setup time only valid if sink node is a SINK");
        VTR_ASSERT(edge_type == tatum::EdgeType::PRIMITIVE_CLOCK_CAPTURE);

        AtomPinId sink_pin = netlist_lookup_.tnode_atom_pin(sink_node);
        VTR_ASSERT(sink_pin);

        const t_pb_graph_pin* gpin = find_pb_graph_pin(sink_pin);
        VTR_ASSERT(gpin->type == PB_PIN_SEQUENTIAL);

        return tatum::Time(gpin->tsu);
    }

    tatum::Time min_edge_delay(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const override {
        //Currently return the same delay
        //TODO: use true min delay
        return max_edge_delay(tg, edge_id);
    }

    tatum::Time hold_time(const tatum::TimingGraph& tg, tatum::EdgeId edge_id) const override {
        //Currently return the same as hold time
        //TODO: use true hold time
        return setup_time(tg, edge_id);
    }

  private:
    //TODO: use generic AtomDelayCalc class to avoid code duplication

    tatum::Time prim_tcq_delay(const tatum::TimingGraph& tg, tatum::NodeId src_node, tatum::NodeId sink_node) const {
        VTR_ASSERT_MSG(tg.node_type(src_node) == tatum::NodeType::CPIN
                           && tg.node_type(sink_node) == tatum::NodeType::SOURCE,
                       "Tcq only defined from CPIN to SOURCE");

        AtomPinId sink_pin = netlist_lookup_.tnode_atom_pin(sink_node);
        VTR_ASSERT(sink_pin);

        const t_pb_graph_pin* gpin = find_pb_graph_pin(sink_pin);
        VTR_ASSERT(gpin->type == PB_PIN_SEQUENTIAL);

        //Clock-to-q delay marked on the SOURCE node (the sink node of this edge)
        auto tco = tatum::Time(gpin->tco_max);

        VTR_ASSERT_MSG(tco.valid(), "Found no primitive clock-to-q delay");

        return tco;
    }

    tatum::Time prim_comb_delay(const tatum::TimingGraph& tg, tatum::NodeId src_node, tatum::NodeId sink_node) const {
        auto src_node_type = tg.node_type(src_node);
        auto sink_node_type = tg.node_type(sink_node);
        VTR_ASSERT_MSG((src_node_type == tatum::NodeType::IPIN && sink_node_type == tatum::NodeType::OPIN)
                           || (src_node_type == tatum::NodeType::SOURCE && sink_node_type == tatum::NodeType::SINK)
                           || (src_node_type == tatum::NodeType::SOURCE && sink_node_type == tatum::NodeType::OPIN)
                           || (src_node_type == tatum::NodeType::CPIN && sink_node_type == tatum::NodeType::OPIN)
                           || (src_node_type == tatum::NodeType::IPIN && sink_node_type == tatum::NodeType::SINK),
                       "Primitive combinational delay must be between {SOURCE, IPIN} and {SINK, OPIN}, or CPIN/OPIN");

        //Primitive internal combinational delay
        AtomPinId input_pin = netlist_lookup_.tnode_atom_pin(src_node);
        VTR_ASSERT(input_pin);
        const t_pb_graph_pin* input_gpin = find_pb_graph_pin(input_pin);

        AtomPinId output_pin = netlist_lookup_.tnode_atom_pin(sink_node);
        VTR_ASSERT(output_pin);
        const t_pb_graph_pin* output_gpin = find_pb_graph_pin(output_pin);

        tatum::Time time;
        for (int i = 0; i < input_gpin->num_pin_timing; ++i) {
            const t_pb_graph_pin* sink_gpin = input_gpin->pin_timing[i];

            if (sink_gpin == output_gpin) {
                time = tatum::Time(input_gpin->pin_timing_del_max[i]);
                break;
            }
        }

        VTR_ASSERT_MSG(time.valid(), "Found no primitive combinational delay for edge");

        return time;
    }

    const t_pb_graph_pin* find_pb_graph_pin(const AtomPinId pin) const {
        AtomBlockId blk = netlist_.pin_block(pin);

        const t_pb_graph_node* pb_gnode = prepacker_.get_expected_lowest_cost_pb_gnode(blk);

        AtomPortId port = netlist_.pin_port(pin);
        const t_model_ports* model_port = netlist_.port_model(port);
        int ipin = netlist_.pin_port_bit(pin);

        const t_pb_graph_pin* gpin = get_pb_graph_node_pin_from_model_port_pin(model_port, ipin, pb_gnode);
        VTR_ASSERT(gpin);

        return gpin;
    }

    tatum::Time calc_intra_molecule_delay(AtomPinId src_pin, AtomPinId sink_pin) const {
        const t_pb_graph_pin* src_gpin = find_pb_graph_pin(src_pin);
        const t_pb_graph_pin* sink_gpin = find_pb_graph_pin(sink_pin);

        float delay = calc_pb_graph_path_delay(src_gpin, sink_gpin);

        // If a valid path was found through the pb_graph hierarchy, use it.
        // Otherwise we assume that it is a very low delay, close to 0.
        return tatum::Time(delay >= 0.0f ? delay : 0.0f);
    }

    tatum::Time calc_inter_molecule_chain_delay(AtomPinId src_pin, AtomPinId sink_pin) const {
        const t_pb_graph_pin* src_gpin = find_pb_graph_pin(src_pin);
        const t_pb_graph_pin* sink_gpin = find_pb_graph_pin(sink_pin);

        // Estimate the delay as the intra-cluster routing from the source
        // pin out to the source cluster's boundary, plus the intra-cluster
        // routing from the sink cluster's boundary in to the sink pin. The
        // delay of the dedicated chain wiring between clusters is not
        // included here: at this point in the flow no RR graph exists to
        // measure it from, and chain direct connections in VTR architectures
        // commonly use a delayless switch by default. Omitting it is still
        // far more accurate than the pessimistic general inter-cluster delay.
        float src_to_boundary = calc_pb_graph_delay_to_root_pin(src_gpin);
        float boundary_to_sink = calc_pb_graph_delay_from_root_pin(sink_gpin);

        if (src_to_boundary < 0.0f || boundary_to_sink < 0.0f) {
            // Could not find a path to/from the cluster boundary; fall back
            // to 0, which is still more accurate than the pessimistic
            // inter-cluster delay since chain connections use dedicated
            // low-delay wiring.
            return tatum::Time(0.0f);
        }

        return tatum::Time(src_to_boundary + boundary_to_sink);
    }

    const t_pb_graph_pin* find_associated_clock_pin(const AtomPinId io_pin) const {
        const t_pb_graph_pin* io_gpin = find_pb_graph_pin(io_pin);

        const t_pb_graph_pin* clock_gpin = io_gpin->associated_clock_pin;

        if (!clock_gpin) {
            AtomBlockId blk = netlist_.pin_block(io_pin);
            std::string model_name = models_.get_model(netlist_.block_model(blk)).name;
            VPR_FATAL_ERROR(VPR_ERROR_TIMING, "Failed to find clock pin associated with pin '%s' (model '%s')", netlist_.pin_name(io_pin).c_str(), model_name.c_str());
        }
        return clock_gpin;
    }

  private:
    const AtomNetlist& netlist_;
    const AtomLookup& netlist_lookup_;
    const LogicalModels& models_;
    const vtr::vector<AtomPinId, float>& timing_arc_delays_;
    const Prepacker& prepacker_;
};
