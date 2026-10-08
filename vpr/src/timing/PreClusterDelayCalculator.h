#pragma once

#include <cmath>
#include <unordered_map>
#include <utility>

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
#include "vtr_hash.h"
#include "vtr_vector.h"

class LogicalModels;

/**
 * @brief How the pre-cluster delay calculator computes the delay of an
 *        interconnect timing arc.
 */
enum class e_pre_cluster_arc_type {
    INTRA_MOLECULE,       ///< The driver and sink are in the same molecule. Uses the intra-cluster delay.
    INTER_MOLECULE_CHAIN, ///< The driver and sink are in different molecules of the same chain. Uses the chain delay.
    EXTERNAL              ///< Any other arc. Uses the arc delay set by set_arc_delay.
};

class PreClusterDelayCalculator : public tatum::DelayCalculator {
  public:
    /**
     * @brief Constructor for the pre-cluster delay calculator.
     *
     *  @param netlist
     *          The primitive netlist to compute delays over.
     *  @param netlist_lookup
     *          A lookup between the primitives and their timing nodes.
     *  @param models
     *          The logical models in the architecture.
     *  @param initial_arc_delay
     *          The delay that every timing arc which is not handled internally
     *          by this calculator (i.e. not intra-molecule or chain arcs) is
     *          initialized to. Use set_arc_delay to update these delays.
     *  @param prepacker
     *          The prepacker object used to prepack primitives into molecules.
     *  @param timing_graph
     *          The timing graph that this calculator computes delays for.
     */
    PreClusterDelayCalculator(const AtomNetlist& netlist,
                              const AtomLookup& netlist_lookup,
                              const LogicalModels& models,
                              float initial_arc_delay,
                              const Prepacker& prepacker,
                              const tatum::TimingGraph& timing_graph)
        : netlist_(netlist)
        , netlist_lookup_(netlist_lookup)
        , models_(models)
        , timing_arc_delays_(netlist.pins().size(), initial_arc_delay)
        , prepacker_(prepacker)
        , intra_molecule_delays_(timing_graph.edges().size(), tatum::Time(NAN))
        , chain_delays_(timing_graph.edges().size(), tatum::Time(NAN)) {
        precompute_internal_arc_delays(timing_graph);
    }

    /**
     * @brief Set the delay of a timing arc, identified by the sink pin that
     *        it terminates at.
     *
     * This delay is only used for arcs which are not handled internally by
     * this calculator (i.e. arcs between different molecules which are not
     * part of the same chain).
     *
     * This must not be called while timing analysis is running.
     */
    void set_arc_delay(AtomPinId sink_pin_id, float delay) {
        VTR_ASSERT_SAFE_MSG(sink_pin_id.is_valid(),
                            "Cannot set arc delay of invalid pin");
        VTR_ASSERT_SAFE_MSG(netlist_.pin_type(sink_pin_id) == PinType::SINK,
                            "Timing arcs are identified by their sink pin");

        timing_arc_delays_[sink_pin_id] = delay;
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

            AtomPinId atom_src_pin = netlist_lookup_.tnode_atom_pin(src_node);
            switch (get_arc_type(atom_src_pin, atom_sink_pin)) {
                case e_pre_cluster_arc_type::INTRA_MOLECULE:
                    // The source and sink atoms will be packed into the same
                    // cluster, so the inter-cluster delay is a significant
                    // overestimate. Use a more accurate intra-cluster delay
                    // derived from the pb_graph hierarchy instead.
                    VTR_ASSERT_SAFE(!std::isnan(intra_molecule_delays_[edge_id].value()));
                    return intra_molecule_delays_[edge_id];
                case e_pre_cluster_arc_type::INTER_MOLECULE_CHAIN:
                    // The connection uses dedicated chain wiring between
                    // clusters rather than general-purpose inter-cluster
                    // routing.
                    VTR_ASSERT_SAFE(!std::isnan(chain_delays_[edge_id].value()));
                    return chain_delays_[edge_id];
                case e_pre_cluster_arc_type::EXTERNAL:
                default:
                    // External net delay
                    return tatum::Time(timing_arc_delays_[atom_sink_pin]);
            }
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

    /**
     * @brief Get the pb_graph pin that the given primitive pin is expected to
     *        be implemented by (based on the lowest cost pb_graph node that
     *        the prepacker expects its block to be packed into).
     */
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

    /**
     * @brief Get how the delay of the interconnect timing arc between the
     *        given source and sink primitive pins is computed.
     *
     * Arcs between atoms in the same molecule use an intra-cluster delay, arcs
     * between molecules of the same chain (e.g. a long carry chain split
     * across multiple clusters) use a chain delay, and all other arcs use the
     * delay set by set_arc_delay.
     *
     *  @param src_pin  The source pin of the arc. May be invalid if the
     *                  source timing node has no atom pin.
     *  @param sink_pin The sink pin of the arc.
     */
    e_pre_cluster_arc_type get_arc_type(AtomPinId src_pin, AtomPinId sink_pin) const {
        if (!src_pin.is_valid())
            return e_pre_cluster_arc_type::EXTERNAL;

        PackMoleculeId src_mol = prepacker_.get_atom_molecule(netlist_.pin_block(src_pin));
        PackMoleculeId sink_mol = prepacker_.get_atom_molecule(netlist_.pin_block(sink_pin));
        if (src_mol == sink_mol)
            return e_pre_cluster_arc_type::INTRA_MOLECULE;

        const t_pack_molecule& src_mol_info = prepacker_.get_molecule(src_mol);
        const t_pack_molecule& sink_mol_info = prepacker_.get_molecule(sink_mol);
        if (src_mol_info.chain_id.is_valid() && src_mol_info.chain_id == sink_mol_info.chain_id)
            return e_pre_cluster_arc_type::INTER_MOLECULE_CHAIN;

        return e_pre_cluster_arc_type::EXTERNAL;
    }

    /**
     * @brief Pre-compute the delays of all interconnect arcs which are handled
     *        internally by this calculator (intra-molecule and chain arcs).
     *
     * These delays only depend on the pb_graph pins of the arc, so they are
     * computed once per distinct pair of pb_graph pins. This is done eagerly
     * (rather than lazily in max_edge_delay) so that the delays can be shared
     * between edges without synchronization during parallel timing analysis.
     */
    void precompute_internal_arc_delays(const tatum::TimingGraph& timing_graph) {
        using t_gpin_pair = std::pair<const t_pb_graph_pin*, const t_pb_graph_pin*>;
        std::unordered_map<t_gpin_pair, tatum::Time, vtr::hash_pair> intra_molecule_cache;
        std::unordered_map<t_gpin_pair, tatum::Time, vtr::hash_pair> chain_cache;

        for (tatum::EdgeId edge_id : timing_graph.edges()) {
            if (timing_graph.edge_type(edge_id) != tatum::EdgeType::INTERCONNECT)
                continue;
            AtomPinId src_pin = netlist_lookup_.tnode_atom_pin(timing_graph.edge_src_node(edge_id));
            AtomPinId sink_pin = netlist_lookup_.tnode_atom_pin(timing_graph.edge_sink_node(edge_id));
            if (!sink_pin.is_valid())
                continue;

            e_pre_cluster_arc_type arc_type = get_arc_type(src_pin, sink_pin);
            if (arc_type == e_pre_cluster_arc_type::EXTERNAL)
                continue;

            t_gpin_pair gpins = {find_pb_graph_pin(src_pin), find_pb_graph_pin(sink_pin)};
            if (arc_type == e_pre_cluster_arc_type::INTRA_MOLECULE) {
                auto [it, inserted] = intra_molecule_cache.try_emplace(gpins);
                if (inserted)
                    it->second = calc_intra_molecule_delay(gpins.first, gpins.second);
                intra_molecule_delays_[edge_id] = it->second;
            } else {
                VTR_ASSERT_SAFE(arc_type == e_pre_cluster_arc_type::INTER_MOLECULE_CHAIN);
                auto [it, inserted] = chain_cache.try_emplace(gpins);
                if (inserted)
                    it->second = calc_inter_molecule_chain_delay(gpins.first, gpins.second);
                chain_delays_[edge_id] = it->second;
            }
        }
    }

    /**
     * @brief Calculates the delay between two primitive pins that belong to
     *        the same molecule (and will therefore be packed into the same
     *        cluster), using the pb_graph path between their pb_graph pins.
     *
     * @param src_gpin  Source pb_graph pin.
     * @param sink_gpin Sink pb_graph pin.
     *
     * @return The intra-cluster delay between the pins, or approximately 0 if
     *         no path was found in the pb_graph.
     */
    static tatum::Time calc_intra_molecule_delay(const t_pb_graph_pin* src_gpin, const t_pb_graph_pin* sink_gpin) {
        float delay = calc_pb_graph_path_delay(src_gpin, sink_gpin);

        // If a valid path was found through the pb_graph hierarchy, use it.
        // Otherwise we assume that it is a very low delay, close to 0.
        return tatum::Time(delay >= 0.0f ? delay : 0.0f);
    }

    /**
     * @brief Calculates the delay between two atom pins that belong to
     *        different molecules of the same chain (e.g. a long carry
     *        chain split across multiple clusters).
     *
     * @param src_gpin  Source pb_graph pin.
     * @param sink_gpin Sink pb_graph pin.
     *
     * @return The estimated delay between the pins, made up of
     *         the intra-cluster routing out to the source cluster's
     *         boundary plus the intra-cluster routing in from the sink
     *         cluster's boundary. The dedicated inter-cluster chain wiring
     *         delay is not included; see the comment below for why.
     */
    static tatum::Time calc_inter_molecule_chain_delay(const t_pb_graph_pin* src_gpin, const t_pb_graph_pin* sink_gpin) {
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
    /// @brief Delays of all timing arcs in the atom netlist, identified by the
    ///        sink pin that they terminate at. These delays are used for any
    ///        arc not handled internally by this calculator.
    vtr::vector<AtomPinId, float> timing_arc_delays_;
    const Prepacker& prepacker_;

    /// @brief The delays of intra-molecule interconnect arcs, indexed by
    ///        timing edge. NaN for all other edges. Pre-computed on
    ///        construction (see precompute_internal_arc_delays).
    vtr::vector<tatum::EdgeId, tatum::Time> intra_molecule_delays_;

    /// @brief The delays of inter-molecule chain interconnect arcs, indexed by
    ///        timing edge. NaN for all other edges. Pre-computed on
    ///        construction (see precompute_internal_arc_delays).
    vtr::vector<tatum::EdgeId, tatum::Time> chain_delays_;
};
