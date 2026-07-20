/**
 * @file
 * @brief Implementation of ClusterPinCounter.
 */

#include "cluster_pin_counter.h"

#include <algorithm>
#include <unordered_set>

#include "atom_netlist.h"
#include "atom_pb_bimap.h"
#include "globals.h"
#include "physical_types.h"
#include "vpr_context.h"
#include "vpr_types.h"
#include "vpr_utils.h"
#include "vtr_assert.h"

void ClusterPinCounter::allocate_pb_state(const t_pb* pb) {
    VTR_ASSERT(pb != nullptr);
    if (per_pb_state_.count(pb) != 0)
        return;

    const t_pb_graph_node* pb_graph_node = pb->pb_graph_node;
    VTR_ASSERT(pb_graph_node != nullptr);

    const size_t num_input_classes = pb_graph_node->input_pin_class_sizes.size();
    const size_t num_output_classes = pb_graph_node->output_pin_class_sizes.size();

    PerPbState& state = per_pb_state_[pb];
    state.committed_input_pin_class_nets.assign(num_input_classes, {});
    state.committed_output_pin_class_nets.assign(num_output_classes, {});
    state.lookahead_input_pin_class_nets.assign(num_input_classes, {});
    state.lookahead_output_pin_class_nets.assign(num_output_classes, {});
}

void ClusterPinCounter::deallocate(const t_pb* pb) {
    per_pb_state_.erase(pb);
}

void ClusterPinCounter::deallocate_recursive(const t_pb* pb) {                                                                                                                                                                                                                     
      if (pb == nullptr) 
        return;                                                                                                                                                                                                                                                     
      per_pb_state_.erase(pb);
      const t_pb_type* pb_type = pb->pb_graph_node->pb_type;                                                                                                                                                                                                                         
      if (pb_type->blif_model != nullptr) 
        return; // primitive, no children to recurse into
      if (pb->child_pbs == nullptr) 
        return;                                                                                                                                                                                                                                          
      const int mode = pb->mode;
      for (int i = 0; i < pb_type->modes[mode].num_pb_type_children; i++) {                                                                                                                                                                                                          
          if (!pb->child_pbs[i]) 
            continue;
          for (int j = 0; j < pb_type->modes[mode].pb_type_children[i].num_pb; j++) {                                                                                                                                                                                                
              deallocate_recursive(&pb->child_pbs[i][j]);
          }                                                                                                                                                                                                                                                                          
      }           
  }        

void ClusterPinCounter::mark_lookahead_input(const t_pb* pb, size_t class_id, AtomNetId net) {
    auto it = per_pb_state_.find(pb);
    VTR_ASSERT(it != per_pb_state_.end());

    auto& class_nets = it->second.lookahead_input_pin_class_nets[class_id];
    if (std::find(class_nets.begin(), class_nets.end(), net) == class_nets.end()) {
        class_nets.push_back(net);
    }
}

void ClusterPinCounter::mark_lookahead_output(const t_pb* pb, size_t class_id, AtomNetId net) {
    auto it = per_pb_state_.find(pb);
    VTR_ASSERT(it != per_pb_state_.end());

    it->second.lookahead_output_pin_class_nets[class_id].push_back(net);
}

void ClusterPinCounter::reset_lookahead() {
    for (auto& kv : per_pb_state_) {
        for (auto& class_nets : kv.second.lookahead_input_pin_class_nets) {
            class_nets.clear();
        }
        for (auto& class_nets : kv.second.lookahead_output_pin_class_nets) {
            class_nets.clear();
        }
    }
}

// !!! WARNING — INHERITED PEAK-SIZE SEMANTICS !!!
//
// This commit uses "peak-size" semantics: for each class, committed only ever
// grows to match the current lookahead; it never shrinks even when the current
// lookahead is smaller (e.g., because a net was absorbed inside the cluster).
//
// This preserves the behavior of the removed legacy commit_lookahead_pins_used
// in cluster_legalizer.cpp, which stored committed state as
//   std::unordered_map<size_t /*vector index*/, AtomNetId>
// and used insert({index, net}) with vector-index-as-key. Because insert does
// not overwrite existing keys, once a class had been committed at some size N
// it stayed >= N for the rest of the cluster's lifetime, even if subsequent
// commits' lookahead had size < N.
//
// The only thing anyone reads from committed state is `.size()`:
//   - check_lookahead_pins_used (at root only) floors class_size at
//     committed.size() — the "seed pin protection" floor comment in that
//     function explains the intent.
//   - get_num_cluster_inputs_available sums committed.size() at root.
//
// Why we still mirror the peak-size behavior rather than using the
// semantically cleaner `committed = lookahead` (current-usage semantics):
//   - Peak is strictly more permissive than current at the root floor, so
//     switching changes which candidates pass the pin-feasibility filter
//     and shifts QoR. Preserving peak keeps this branch bit-for-bit QoR
//     equivalent to master, which is what the pin-counting rewrite promises.
//   - The semantic swap is a follow-up: replace the loop below with the
//     current-usage version shown in the commented block, then run a
//     vtr_reg_strong QoR study to measure and justify the change.
//
// TODO (follow-up): swap peak-mirror for current-usage semantics. The
// commented block at the bottom of this function is the intended
// replacement. Peak semantics is undocumented in the original code and
// falls out of the unordered_map insert quirk described above; the floor's
// stated intent (see the "when packing the seed block" comment in
// check_lookahead_pins_used) is only to protect the seed's committed count,
// not any historical peak. Run vtr_reg_strong to confirm QoR impact.
void ClusterPinCounter::commit_lookahead() {
    // ---- Peak-size behavior inherited from the removed legacy commit ----
    for (auto& kv : per_pb_state_) {
        PerPbState& state = kv.second;
        for (size_t c = 0; c < state.committed_input_pin_class_nets.size(); c++) {
            auto& committed = state.committed_input_pin_class_nets[c];
            const auto& lookahead = state.lookahead_input_pin_class_nets[c];
            if (lookahead.size() > committed.size()) {
                committed = lookahead;
            }
        }
        for (size_t c = 0; c < state.committed_output_pin_class_nets.size(); c++) {
            auto& committed = state.committed_output_pin_class_nets[c];
            const auto& lookahead = state.lookahead_output_pin_class_nets[c];
            if (lookahead.size() > committed.size()) {
                committed = lookahead;
            }
        }
    }

    // ---- What the correct implementation should look like once legacy is gone ----
    // Current-usage semantics: committed reflects the nets currently claiming
    // pins after this commit, not the historical peak. Swap the block above
    // for the one below when the legacy path is removed (see TODO above).
    //
    // for (auto& kv : per_pb_state_) {
    //     PerPbState& state = kv.second;
    //     state.committed_input_pin_class_nets  = state.lookahead_input_pin_class_nets;
    //     state.committed_output_pin_class_nets = state.lookahead_output_pin_class_nets;
    // }
}

size_t ClusterPinCounter::lookahead_input_size(const t_pb* pb, size_t class_id) const {
    auto it = per_pb_state_.find(pb);
    VTR_ASSERT(it != per_pb_state_.end());
    return it->second.lookahead_input_pin_class_nets[class_id].size();
}

size_t ClusterPinCounter::lookahead_output_size(const t_pb* pb, size_t class_id) const {
    auto it = per_pb_state_.find(pb);
    VTR_ASSERT(it != per_pb_state_.end());
    return it->second.lookahead_output_pin_class_nets[class_id].size();
}

size_t ClusterPinCounter::committed_input_size(const t_pb* pb, size_t class_id) const {
    auto it = per_pb_state_.find(pb);
    VTR_ASSERT(it != per_pb_state_.end());
    return it->second.committed_input_pin_class_nets[class_id].size();
}

size_t ClusterPinCounter::committed_output_size(const t_pb* pb, size_t class_id) const {
    auto it = per_pb_state_.find(pb);
    VTR_ASSERT(it != per_pb_state_.end());
    return it->second.committed_output_pin_class_nets[class_id].size();
}

/*
 * @brief Checks if the sinks of the given net are reachable from the driver
 *        pb gpin.
 */
static int net_sinks_reachable_in_cluster(const t_pb_graph_pin* driver_pb_gpin, const int depth, const AtomNetId net_id, const AtomPBBimap& atom_to_pb) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    //Record the sink pb graph pins we are looking for
    std::unordered_set<const t_pb_graph_pin*> sink_pb_gpins;
    for (const AtomPinId pin_id : atom_ctx.netlist().net_sinks(net_id)) {
        const t_pb_graph_pin* sink_pb_gpin = find_pb_graph_pin(atom_ctx.netlist(), atom_to_pb, pin_id);
        VTR_ASSERT(sink_pb_gpin);

        sink_pb_gpins.insert(sink_pb_gpin);
    }

    //Count how many sink pins are reachable
    size_t num_reachable_sinks = 0;
    for (int i_prim_pin = 0; i_prim_pin < driver_pb_gpin->num_connectable_primitive_input_pins[depth]; ++i_prim_pin) {
        const t_pb_graph_pin* reachable_pb_gpin = driver_pb_gpin->list_of_connectable_input_pin_ptrs[depth][i_prim_pin];

        if (sink_pb_gpins.count(reachable_pb_gpin)) {
            ++num_reachable_sinks;
            if (num_reachable_sinks == atom_ctx.netlist().net_sinks(net_id).size()) {
                return true;
            }
        }
    }

    return false;
}

/**
 * @brief Returns the pb_graph_pin of the atom pin defined by the driver_pin_id in the driver_pb
 */
static t_pb_graph_pin* get_driver_pb_graph_pin(const t_pb* driver_pb, const AtomPinId driver_pin_id) {
    const AtomNetlist& atom_netlist = g_vpr_ctx.atom().netlist();

    const auto driver_pb_type = driver_pb->pb_graph_node->pb_type;
    int output_port = 0;
    // find the port of the pin driving the net as well as the port model
    auto driver_port_id = atom_netlist.pin_port(driver_pin_id);
    auto driver_model_port = atom_netlist.port_model(driver_port_id);
    // find the port id of the port containing the driving pin in the driver_pb_type
    for (int i = 0; i < driver_pb_type->num_ports; i++) {
        auto& prim_port = driver_pb_type->ports[i];
        if (prim_port.type == OUT_PORT) {
            if (prim_port.model_port == driver_model_port) {
                // get the output pb_graph_pin driving this input net
                return &(driver_pb->pb_graph_node->output_pins[output_port][atom_netlist.pin_port_bit(driver_pin_id)]);
            }
            output_port++;
        }
    }
    // the pin should be found
    VTR_ASSERT(false);
    return nullptr;
}

void ClusterPinCounter::compute_and_mark_lookahead_pins_used_for_input_pin(const t_pb_graph_pin* pb_graph_pin,
                                                                           const t_pb* primitive_pb,
                                                                           AtomNetId net_id,
                                                                           const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                                                                           const AtomPBBimap& atom_to_pb) {
    VTR_ASSERT(pb_graph_pin->port->type == IN_PORT);
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    const auto driver_blk_id = atom_ctx.netlist().net_driver_block(net_id);

    /* find location of net driver if exist in clb, NULL otherwise */
    // find the driver of the input net connected to the pin being studied
    const auto driver_pin_id = atom_ctx.netlist().net_driver(net_id);
    // find the id of the atom occupying the input primitive_pb
    const auto prim_blk_id = atom_to_pb.pb_atom(primitive_pb);
    // find the pb block occupied by the driving atom
    const auto driver_pb = atom_to_pb.atom_pb(driver_blk_id);
    // pb_graph_pin driving net_id in the driver pb block
    t_pb_graph_pin* output_pb_graph_pin = nullptr;
    // if the driver block is in the same clb as the input primitive block
    LegalizationClusterId driver_cluster_id = atom_cluster[driver_blk_id];
    LegalizationClusterId prim_cluster_id = atom_cluster[prim_blk_id];
    if (driver_cluster_id == prim_cluster_id) {
        // get pb_graph_pin driving the given net
        output_pb_graph_pin = get_driver_pb_graph_pin(driver_pb, driver_pin_id);
    }

    // starting from the parent pb of the input primitive go up in the hierarchy till the root block
    for (auto cur_pb = primitive_pb->parent_pb; cur_pb; cur_pb = cur_pb->parent_pb) {
        const auto depth = cur_pb->pb_graph_node->pb_type->depth;
        const auto pin_class = pb_graph_pin->parent_pin_class[depth];
        VTR_ASSERT(pin_class != UNDEFINED);

        bool is_reachable = false;

        // if the driver pin is within the cluster
        if (output_pb_graph_pin) {
            // find if the driver pin can reach the input pin of the primitive or not
            const t_pb* check_pb = driver_pb;
            while (check_pb && check_pb != cur_pb) {
                check_pb = check_pb->parent_pb;
            }
            if (check_pb) {
                for (int i = 0; i < output_pb_graph_pin->num_connectable_primitive_input_pins[depth]; i++) {
                    if (pb_graph_pin == output_pb_graph_pin->list_of_connectable_input_pin_ptrs[depth][i]) {
                        is_reachable = true;
                        break;
                    }
                }
            }
        }

        // Must use an input pin to connect the driver to the input pin of the given primitive, either the
        // driver atom is not contained in the cluster or is contained but cannot reach the primitive pin
        if (!is_reachable) {
            mark_lookahead_input(cur_pb, pin_class, net_id);
        }
    }
}

void ClusterPinCounter::compute_and_mark_lookahead_pins_used_for_output_pin(const t_pb_graph_pin* pb_graph_pin,
                                                                            const t_pb* primitive_pb,
                                                                            AtomNetId net_id,
                                                                            const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                                                                            const AtomPBBimap& atom_to_pb) {
    VTR_ASSERT(pb_graph_pin->port->type == OUT_PORT);
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    const auto driver_blk_id = atom_ctx.netlist().net_driver_block(net_id);
    int num_net_sinks = static_cast<int>(atom_ctx.netlist().net_sinks(net_id).size());
    bool all_sinks_in_cur_cluster_computed = false;
    bool all_sinks_in_cur_cluster = false;

    // starting from the parent pb of the input primitive go up in the hierarchy till the root block
    for (auto cur_pb = primitive_pb->parent_pb; cur_pb; cur_pb = cur_pb->parent_pb) {
        const auto depth = cur_pb->pb_graph_node->pb_type->depth;
        const auto pin_class = pb_graph_pin->parent_pin_class[depth];
        VTR_ASSERT(pin_class != UNDEFINED);

        /*
         * Determine if this net (which is driven from within this cluster) leaves this cluster
         * (and hence uses an output pin).
         */

        bool net_exits_cluster = true;

        if (pb_graph_pin->num_connectable_primitive_input_pins[depth] >= num_net_sinks) {
            //It is possible the net is completely absorbed in the cluster,
            //since this pin could (potentially) drive all the net's sinks

            /* Important: This runtime penalty looks a lot scarier than it really is.
             * For high fan-out nets, I at most look at the number of pins within the
             * cluster which limits runtime.
             *
             * DO NOT REMOVE THIS INITIAL FILTER WITHOUT CAREFUL ANALYSIS ON RUNTIME!!!
             *
             * Key Observation:
             * For LUT-based designs it is impossible for the average fanout to exceed
             * the number of LUT inputs so it's usually around 4-5 (pigeon-hole argument,
             * if the average fanout is greater than the number of LUT inputs, where do
             * the extra connections go?  Therefore, average fanout must be capped to a
             * small constant where the constant is equal to the number of LUT inputs).
             * The real danger to runtime is when the number of sinks of a net gets doubled
             */

            //Check if all the net sinks are, in fact, inside this cluster
            if (!all_sinks_in_cur_cluster_computed) {
                const LegalizationClusterId driver_cluster = atom_cluster[driver_blk_id];
                all_sinks_in_cur_cluster = true;
                for (auto pin_id : atom_ctx.netlist().net_sinks(net_id)) {
                    if (atom_cluster[atom_ctx.netlist().pin_block(pin_id)] != driver_cluster) {
                        all_sinks_in_cur_cluster = false;
                        break;
                    }
                }
                all_sinks_in_cur_cluster_computed = true;
            }

            if (all_sinks_in_cur_cluster) {
                //All the sinks are part of this cluster, so the net may be fully absorbed.
                //
                //Verify this, by counting the number of net sinks reachable from the driver pin.
                //If the count equals the number of net sinks then the net is fully absorbed and
                //the net does not exit the cluster
                /* TODO: I should cache the absorbed outputs, once net is absorbed,
                 *       net is forever absorbed, no point in rechecking every time */
                if (net_sinks_reachable_in_cluster(pb_graph_pin, depth, net_id, atom_to_pb)) {
                    //All the sinks are reachable inside the cluster
                    net_exits_cluster = false;
                }
            }
        }

        if (net_exits_cluster) {
            /* This output must exit this cluster */
            mark_lookahead_output(cur_pb, pin_class, net_id);
        }
    }
}

void ClusterPinCounter::compute_and_mark_lookahead_pins_used(
    AtomBlockId blk_id,
    const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
    const AtomPBBimap& atom_to_pb) {
    const AtomNetlist& atom_netlist = g_vpr_ctx.atom().netlist();

    const t_pb* cur_pb = atom_to_pb.atom_pb(blk_id);
    VTR_ASSERT(cur_pb != nullptr);

    /* Walk through inputs, outputs, and clocks marking pins off of the same class */
    for (auto pin_id : atom_netlist.block_pins(blk_id)) {
        auto net_id = atom_netlist.pin_net(pin_id);

        const t_pb_graph_pin* pb_graph_pin = find_pb_graph_pin(atom_netlist, atom_to_pb, pin_id);

        if (pb_graph_pin->port->type == IN_PORT) {
            compute_and_mark_lookahead_pins_used_for_input_pin(pb_graph_pin, cur_pb, net_id, atom_cluster, atom_to_pb);
        } else {
            compute_and_mark_lookahead_pins_used_for_output_pin(pb_graph_pin, cur_pb, net_id, atom_cluster, atom_to_pb);
        }
    }
}

void ClusterPinCounter::try_update_lookahead_pins_used(
    const std::vector<PackMoleculeId>& molecules,
    const Prepacker& prepacker,
    const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
    const AtomPBBimap& atom_to_pb) {
    for (PackMoleculeId molecule_id : molecules) {
        const t_pack_molecule& molecule = prepacker.get_molecule(molecule_id);
        for (AtomBlockId blk_id : molecule.atom_block_ids) {
            if (!blk_id.is_valid()) {
                continue;
            }

            const t_pb* primitive_pb = atom_to_pb.atom_pb(blk_id);
            VTR_ASSERT_SAFE(primitive_pb != nullptr);
            VTR_ASSERT_SAFE(primitive_pb->pb_graph_node->pb_type->is_primitive());

            VTR_ASSERT(primitive_pb->pb_graph_node->pb_type->blif_model != nullptr);
            compute_and_mark_lookahead_pins_used(blk_id, atom_cluster, atom_to_pb);
        }
    }
}

bool ClusterPinCounter::check_lookahead_pins_used(t_pb* cur_pb, t_ext_pin_util max_external_pin_util) {
    const t_pb_type* pb_type = cur_pb->pb_graph_node->pb_type;

    if (!pb_type->is_primitive() && cur_pb->name) {
        for (size_t class_id = 0; class_id < cur_pb->pb_graph_node->input_pin_class_sizes.size(); class_id++) {
            size_t class_size = cur_pb->pb_graph_node->input_pin_class_sizes[class_id];

            if (cur_pb->is_root()) {
                // Scale the class size by the maximum external pin utilization factor
                // Use ceil to avoid classes of size 1 from being scaled to zero
                class_size = std::ceil(max_external_pin_util.input_pin_util * class_size);
                // if the number of pins already used is larger than class size, then the number of
                // cluster inputs already used should be our constraint. Why is this needed? This is
                // needed since when packing the seed block the maximum external pin utilization is
                // used as 1.0 allowing molecules that are using up to all the cluster inputs to be
                // packed legally. Therefore, if the seed block is already using more inputs than
                // the allowed maximum utilization, this should become the new maximum pin utilization.
                class_size = std::max<size_t>(class_size, committed_input_size(cur_pb, class_id));
            }

            if (lookahead_input_size(cur_pb, class_id) > class_size) {
                return false;
            }
        }

        for (size_t class_id = 0; class_id < cur_pb->pb_graph_node->output_pin_class_sizes.size(); class_id++) {
            size_t class_size = cur_pb->pb_graph_node->output_pin_class_sizes[class_id];

            if (cur_pb->is_root()) {
                // Scale the class size by the maximum external pin utilization factor
                // Use ceil to avoid classes of size 1 from being scaled to zero
                class_size = std::ceil(max_external_pin_util.output_pin_util * class_size);
                // if the number of pins already used is larger than class size, then the number of
                // cluster outputs already used should be our constraint. Why is this needed? This is
                // needed since when packing the seed block the maximum external pin utilization is
                // used as 1.0 allowing molecules that are using up to all the cluster inputs to be
                // packed legally. Therefore, if the seed block is already using more inputs than
                // the allowed maximum utilization, this should become the new maximum pin utilization.
                class_size = std::max<size_t>(class_size, committed_output_size(cur_pb, class_id));
            }

            if (lookahead_output_size(cur_pb, class_id) > class_size) {
                return false;
            }
        }

        if (cur_pb->child_pbs) {
            for (int i = 0; i < pb_type->modes[cur_pb->mode].num_pb_type_children; i++) {
                if (cur_pb->child_pbs[i]) {
                    for (int j = 0; j < pb_type->modes[cur_pb->mode].pb_type_children[i].num_pb; j++) {
                        if (!check_lookahead_pins_used(&cur_pb->child_pbs[i][j], max_external_pin_util))
                            return false;
                    }
                }
            }
        }
    }

    return true;
}

