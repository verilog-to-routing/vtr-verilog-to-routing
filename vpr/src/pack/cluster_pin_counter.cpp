/**
 * @file
 * @author  Haydar Cakan
 * @date    July 2026
 * @brief   Implementation of ClusterPinCounter.
 *
 * Contains the per pb state storage and query methods, and the marking
 * routines that walk each atom pin from its primitive pb up to the cluster
 * root and mark the pin class the pin's net uses at each level.
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
    // One empty vector per pin class.
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
    if (pb_type->is_primitive())
        return;

    if (pb->child_pbs == nullptr)
        return;

    const int mode = pb->mode;
    for (int child_num = 0; child_num < pb_type->modes[mode].num_pb_type_children; child_num++) {
        if (!pb->child_pbs[child_num])
            continue;
        for (int pb_instance = 0; pb_instance < pb_type->modes[mode].pb_type_children[child_num].num_pb; pb_instance++) {
            deallocate_recursive(&pb->child_pbs[child_num][pb_instance]);
        }
    }
}

void ClusterPinCounter::mark_lookahead_input(const t_pb* pb, size_t class_id, AtomNetId net) {
    PerPbState& state = per_pb_state_.at(pb);
    std::vector<AtomNetId>& class_nets = state.lookahead_input_pin_class_nets.at(class_id);
    if (std::find(class_nets.begin(), class_nets.end(), net) == class_nets.end()) {
        class_nets.push_back(net);
    }
}

void ClusterPinCounter::mark_lookahead_output(const t_pb* pb, size_t class_id, AtomNetId net) {
    PerPbState& state = per_pb_state_.at(pb);
    state.lookahead_output_pin_class_nets.at(class_id).push_back(net);
}

void ClusterPinCounter::reset_lookahead() {
    for (auto& [pb, state] : per_pb_state_) {
        for (std::vector<AtomNetId>& class_nets : state.lookahead_input_pin_class_nets) {
            class_nets.clear();
        }
        for (std::vector<AtomNetId>& class_nets : state.lookahead_output_pin_class_nets) {
            class_nets.clear();
        }
    }
}

// TODO [BUG]: this loop grows committed to match lookahead but never shrinks,
// even when lookahead is smaller (e.g., a net is absorbed inside the
// cluster). This is a bug inherited from the legacy pin counting code,
// where committed was stored as unordered_map<pin_index, net> and updated
// with insert() which never overwrites existing keys, so committed size
// could only grow across commits. Kept for the scope of this class refactor
// so behaviour is preserved. The fix is to assign lookahead directly
// to committed, as I implemented intuitively first and observed differences
// with legacy. Leaving this QoR changing small fix to next PR.
void ClusterPinCounter::commit_lookahead() {
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
}

size_t ClusterPinCounter::lookahead_input_size(const t_pb* pb, size_t class_id) const {
    const PerPbState& state = per_pb_state_.at(pb);
    return state.lookahead_input_pin_class_nets.at(class_id).size();
}

size_t ClusterPinCounter::lookahead_output_size(const t_pb* pb, size_t class_id) const {
    const PerPbState& state = per_pb_state_.at(pb);
    return state.lookahead_output_pin_class_nets.at(class_id).size();
}

size_t ClusterPinCounter::committed_input_size(const t_pb* pb, size_t class_id) const {
    const PerPbState& state = per_pb_state_.at(pb);
    return state.committed_input_pin_class_nets.at(class_id).size();
}

size_t ClusterPinCounter::committed_output_size(const t_pb* pb, size_t class_id) const {
    const PerPbState& state = per_pb_state_.at(pb);
    return state.committed_output_pin_class_nets.at(class_id).size();
}

/**
 * @brief Check whether every sink of the given net is reachable from the
 *        given driver pb_graph_pin at the given depth.
 *
 * @param driver_pb_gpin  The pb_graph_pin driving net_id.
 * @param depth           The pb depth at which reachability is checked.
 * @param net_id          The net whose sinks are being checked.
 * @param atom_to_pb      Maps atoms to their assigned primitive pb.
 * @return                True if every sink pin of net_id is reachable from
 *                        driver_pb_gpin at the given depth.
 */
static bool net_sinks_reachable_in_cluster(const t_pb_graph_pin* driver_pb_gpin, const int depth, const AtomNetId net_id, const AtomPBBimap& atom_to_pb) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    // Record the sink pb graph pins we are looking for.
    std::unordered_set<const t_pb_graph_pin*> sink_pb_gpins;
    for (const AtomPinId pin_id : atom_ctx.netlist().net_sinks(net_id)) {
        const t_pb_graph_pin* sink_pb_gpin = find_pb_graph_pin(atom_ctx.netlist(), atom_to_pb, pin_id);
        VTR_ASSERT(sink_pb_gpin);

        sink_pb_gpins.insert(sink_pb_gpin);
    }

    // Count how many sink pins are reachable.
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
 * @brief Returns the pb_graph_pin of the atom pin defined by the
 *        driver_pin_id in the driver_pb.
 *
 * @param driver_pb       The pb whose primitive drives driver_pin_id.
 * @param driver_pin_id   The atom pin to look up on driver_pb.
 * @return                The pb_graph_pin on driver_pb that drives
 *                        driver_pin_id. Asserts if the pin cannot be located.
 */
static t_pb_graph_pin* get_driver_pb_graph_pin(const t_pb* driver_pb, const AtomPinId driver_pin_id) {
    const AtomNetlist& atom_netlist = g_vpr_ctx.atom().netlist();

    const auto driver_pb_type = driver_pb->pb_graph_node->pb_type;
    int output_port = 0;
    // Find the port of the pin driving the net as well as the port model.
    auto driver_port_id = atom_netlist.pin_port(driver_pin_id);
    auto driver_model_port = atom_netlist.port_model(driver_port_id);
    // Find the port id of the port containing the driving pin in the driver_pb_type.
    for (int i = 0; i < driver_pb_type->num_ports; i++) {
        auto& prim_port = driver_pb_type->ports[i];
        if (prim_port.type == OUT_PORT) {
            if (prim_port.model_port == driver_model_port) {
                // Get the output pb_graph_pin driving this input net.
                return &(driver_pb->pb_graph_node->output_pins[output_port][atom_netlist.pin_port_bit(driver_pin_id)]);
            }
            output_port++;
        }
    }

    // The pin should be found.
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
    const auto driver_pin_id = atom_ctx.netlist().net_driver(net_id);
    const auto prim_blk_id = atom_to_pb.pb_atom(primitive_pb);
    const auto driver_pb = atom_to_pb.atom_pb(driver_blk_id);

    // If the driver atom is in the same cluster as primitive_pb, find the
    // pb_graph_pin that drives net_id. Otherwise leave it null; the driver
    // is outside the cluster, so the net must enter via an input pin at
    // every level.
    t_pb_graph_pin* output_pb_graph_pin = nullptr;
    LegalizationClusterId driver_cluster_id = atom_cluster[driver_blk_id];
    LegalizationClusterId prim_cluster_id = atom_cluster[prim_blk_id];
    if (driver_cluster_id == prim_cluster_id) {
        output_pb_graph_pin = get_driver_pb_graph_pin(driver_pb, driver_pin_id);
    }

    // Starting from the parent pb of the input primitive go up in the hierarchy till the root block
    for (auto cur_pb = primitive_pb->parent_pb; cur_pb; cur_pb = cur_pb->parent_pb) {
        const auto depth = cur_pb->pb_graph_node->pb_type->depth;
        const auto pin_class = pb_graph_pin->parent_pin_class[depth];
        VTR_ASSERT(pin_class != UNDEFINED);

        bool is_reachable = false;

        // If the driver pin is within the cluster
        if (output_pb_graph_pin) {
            // Find if the driver pin can reach the input pin of the primitive or not
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

    bool all_sinks_in_cur_cluster = false;
    bool all_sinks_in_cur_cluster_computed = false;

    // Once net_sinks_reachable_in_cluster confirms absorption of the current
    // net at some depth D during below loop, the same net is absorbed at every
    // shallower depth (root direction) too. We can skip the check at those
    // shallower ancestors.
    bool confirmed_absorbed = false;

    // Starting from the parent pb of the given primitive, go up in the hierarchy till the root block
    for (auto cur_pb = primitive_pb->parent_pb; cur_pb; cur_pb = cur_pb->parent_pb) {
        const auto depth = cur_pb->pb_graph_node->pb_type->depth;
        const auto pin_class = pb_graph_pin->parent_pin_class[depth];
        VTR_ASSERT(pin_class != UNDEFINED);

        if (confirmed_absorbed) {
            continue;
        }

        // Determine if this net (which is driven from within this cluster)
        // leaves this cluster (and hence uses an output pin).

        bool net_exits_cluster = true;

        if (pb_graph_pin->num_connectable_primitive_input_pins[depth] >= num_net_sinks) {
            // It is possible the net is completely absorbed in the cluster,
            // since this pin could (potentially) drive all the net's sinks

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

            // Check if all the net sinks are, in fact, inside this cluster
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
                // All the sinks are part of this cluster, so the net may be fully absorbed.
                //
                // Verify this, by counting the number of net sinks reachable from the driver pin.
                // If the count equals the number of net sinks then the net is fully absorbed and
                // the net does not exit the cluster
                // TODO: I should cache the absorbed outputs, once net is absorbed,
                //       net is forever absorbed, no point in rechecking every time
                //       Caching within one pin evaluation is implemented by
                //       confirmed_absorbed; leaving this TODO for the incremental case:
                //       caching absorbed nets across candidate checks within a cluster.
                if (net_sinks_reachable_in_cluster(pb_graph_pin, depth, net_id, atom_to_pb)) {
                    // All the sinks are reachable inside the cluster
                    confirmed_absorbed = true;
                    net_exits_cluster = false;
                }
            }
        }

        if (net_exits_cluster) {
            // This output must exit this cluster at this level.
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

    // Walk through inputs and outputs marking pins off of the same class.
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
                // used as 1.0 allowing molecules that are using up to all the cluster outputs to be
                // packed legally. Therefore, if the seed block is already using more outputs than
                // the allowed maximum utilization, this should become the new maximum pin utilization.
                class_size = std::max<size_t>(class_size, committed_output_size(cur_pb, class_id));
            }

            if (lookahead_output_size(cur_pb, class_id) > class_size) {
                return false;
            }
        }

        if (cur_pb->child_pbs) {
            for (int child_num = 0; child_num < pb_type->modes[cur_pb->mode].num_pb_type_children; child_num++) {
                if (cur_pb->child_pbs[child_num]) {
                    for (int pb_instance = 0; pb_instance < pb_type->modes[cur_pb->mode].pb_type_children[child_num].num_pb; pb_instance++) {
                        if (!check_lookahead_pins_used(&cur_pb->child_pbs[child_num][pb_instance], max_external_pin_util))
                            return false;
                    }
                }
            }
        }
    }

    return true;
}
