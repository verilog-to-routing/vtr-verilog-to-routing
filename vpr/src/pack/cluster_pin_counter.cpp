/**
 * @file
 * @author  Haydar Cakan
 * @date    July 2026
 * @brief   Implementation of ClusterPinCounter.
 *
 * Contains the per pb state storage and query methods, and the marking
 * routines that walk each atom pin from its primitive pb up to the cluster
 * root and mark the pin class the pin's net uses at each level.
 * Also implements the per candidate check sequence (snapshot -> apply
 * molecule delta -> check -> commit/rollback) used by the pin feasibility
 * filter to speculatively evaluate candidate molecules.
 */

#include "cluster_pin_counter.h"

#include <algorithm>
#include <string>
#include <unordered_set>
#include <utility>

#include "atom_netlist.h"
#include "atom_pb_bimap.h"
#include "globals.h"
#include "physical_types.h"
#include "vpr_context.h"
#include "vpr_error.h"
#include "vpr_types.h"
#include "vpr_utils.h"
#include "vtr_assert.h"
#include "vtr_memory.h"

void ClusterPinCounter::allocate_pin_count_state(const t_pb* pb) {
    VTR_ASSERT(pb != nullptr);
    VTR_ASSERT_MSG(per_pb_state_.count(pb) == 0,
                   "Pin counting state should be empty before allocation");

    const t_pb_graph_node* pb_graph_node = pb->pb_graph_node;
    VTR_ASSERT(pb_graph_node != nullptr);

    const size_t num_input_classes = pb_graph_node->input_pin_class_sizes.size();
    const size_t num_output_classes = pb_graph_node->output_pin_class_sizes.size();

    PerPbState& state = per_pb_state_[pb];
    // One empty vector per pin class.
    state.input_pin_class_net_counts.assign(num_input_classes, {});
    state.output_pin_class_net_counts.assign(num_output_classes, {});
}

void ClusterPinCounter::deallocate_pin_count_state_recursive(const t_pb* pb) {
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
            deallocate_pin_count_state_recursive(&pb->child_pbs[child_num][pb_instance]);
        }
    }
}

void ClusterPinCounter::clean_state() {
    vtr::release_memory(per_pb_state_);
    vtr::release_memory(input_mark_record_);
    vtr::release_memory(output_mark_record_);
    vtr::release_memory(per_pb_state_journal_);
    vtr::release_memory(mark_record_journal_);
    vtr::release_memory(root_input_class_size_snapshot_);
    vtr::release_memory(root_output_class_size_snapshot_);
}

size_t ClusterPinCounter::input_size(const t_pb* pb, size_t class_id) const {
    VTR_ASSERT_SAFE(pb != nullptr);
    VTR_ASSERT_SAFE(per_pb_state_.count(pb) > 0);
    const PerPbState& state = per_pb_state_.at(pb);
    return state.input_pin_class_net_counts.at(class_id).size();
}

size_t ClusterPinCounter::output_size(const t_pb* pb, size_t class_id) const {
    VTR_ASSERT_SAFE(pb != nullptr);
    VTR_ASSERT_SAFE(per_pb_state_.count(pb) > 0);
    const PerPbState& state = per_pb_state_.at(pb);
    return state.output_pin_class_net_counts.at(class_id).size();
}

void ClusterPinCounter::snapshot_root_class_sizes(const t_pb* root) {
    VTR_ASSERT(root != nullptr && root->is_root());
    VTR_ASSERT_SAFE_MSG(per_pb_state_journal_.empty() && mark_record_journal_.empty(),
                        "Both journals must be empty at the start of a candidate check "
                        "(every previous check must be committed or rolled back before starting a new one).");
    const PerPbState& state = per_pb_state_.at(root);

    root_input_class_size_snapshot_.resize(state.input_pin_class_net_counts.size());
    for (size_t class_id = 0; class_id < state.input_pin_class_net_counts.size(); ++class_id) {
        root_input_class_size_snapshot_[class_id] = state.input_pin_class_net_counts[class_id].size();
    }

    root_output_class_size_snapshot_.resize(state.output_pin_class_net_counts.size());
    for (size_t class_id = 0; class_id < state.output_pin_class_net_counts.size(); ++class_id) {
        root_output_class_size_snapshot_[class_id] = state.output_pin_class_net_counts[class_id].size();
    }
}

void ClusterPinCounter::add_mark(const t_pb* pb, bool is_input, size_t class_id, AtomNetId net_id) {
    PerPbState& state = per_pb_state_.at(pb);
    std::unordered_map<AtomNetId, int>& net_counts = is_input ? state.input_pin_class_net_counts.at(class_id)
                                                              : state.output_pin_class_net_counts.at(class_id);
    int& refcount = net_counts[net_id];
    refcount += 1;

    // Output classes should never exceed a refcount of 1 for a given net:
    // each net has a single driver, so at most one mark per (pb, class, net).
    VTR_ASSERT_SAFE(is_input || refcount == 1);
    per_pb_state_journal_.push_back({pb, net_id, static_cast<int>(class_id), is_input, +1});
}

void ClusterPinCounter::remove_mark(const t_pb* pb, bool is_input, size_t class_id, AtomNetId net_id) {
    PerPbState& state = per_pb_state_.at(pb);
    std::unordered_map<AtomNetId, int>& net_counts = is_input ? state.input_pin_class_net_counts.at(class_id)
                                                              : state.output_pin_class_net_counts.at(class_id);

    int& refcount = net_counts.at(net_id);
    VTR_ASSERT(refcount > 0);
    refcount -= 1;
    if (refcount == 0) {
        net_counts.erase(net_id);
    }

    per_pb_state_journal_.push_back({pb, net_id, static_cast<int>(class_id), is_input, -1});
}

void ClusterPinCounter::wipe_all_marks_journaled() {
    // Copy each class map into `entries` first: remove_mark mutates the
    // class map, so iterating it in place is unsafe.
    for (auto& [pb, state] : per_pb_state_) {
        for (size_t class_id = 0; class_id < state.input_pin_class_net_counts.size(); ++class_id) {
            std::vector<std::pair<AtomNetId, int>> entries(
                state.input_pin_class_net_counts[class_id].begin(),
                state.input_pin_class_net_counts[class_id].end());
            for (const auto& [net, refcount] : entries) {
                for (int i = 0; i < refcount; ++i) {
                    remove_mark(pb, /*is_input=*/true, class_id, net);
                }
            }
        }

        for (size_t class_id = 0; class_id < state.output_pin_class_net_counts.size(); ++class_id) {
            std::vector<std::pair<AtomNetId, int>> entries(
                state.output_pin_class_net_counts[class_id].begin(),
                state.output_pin_class_net_counts[class_id].end());
            for (const auto& [net, refcount] : entries) {
                for (int i = 0; i < refcount; ++i) {
                    remove_mark(pb, /*is_input=*/false, class_id, net);
                }
            }
        }
    }
}

void ClusterPinCounter::record_input_mark(AtomPinId pin_id, const t_pb* pb) {
    input_mark_record_[pin_id].push_back(pb);
    mark_record_journal_.push_back({pin_id, pb, /*is_input=*/true, +1});
}

void ClusterPinCounter::record_output_mark(AtomPinId pin_id, const t_pb* pb) {
    output_mark_record_[pin_id].push_back(pb);
    mark_record_journal_.push_back({pin_id, pb, /*is_input=*/false, +1});
}

void ClusterPinCounter::remove_input_pin_marks(AtomPinId pin_id, AtomNetId net_id, const AtomPBBimap& atom_to_pb) {
    auto it = input_mark_record_.find(pin_id);
    if (it == input_mark_record_.end()) {
        // Pin never contributed a mark on the input side.
        return;
    }

    const AtomNetlist& netlist = g_vpr_ctx.atom().netlist();
    const t_pb_graph_pin* pb_graph_pin = find_pb_graph_pin(netlist, atom_to_pb, pin_id);

    for (const t_pb* pb : it->second) {
        VTR_ASSERT_SAFE(per_pb_state_.count(pb) > 0);

        const int depth = pb->pb_graph_node->pb_type->depth;
        const int class_id = pb_graph_pin->parent_pin_class[depth];
        VTR_ASSERT(class_id != UNDEFINED);

        remove_mark(pb, /*is_input=*/true, class_id, net_id);
        mark_record_journal_.push_back({pin_id, pb, /*is_input=*/true, -1});
    }
    input_mark_record_.erase(it);
}

void ClusterPinCounter::remove_output_pin_marks(AtomPinId pin_id, AtomNetId net_id, const AtomPBBimap& atom_to_pb) {
    auto it = output_mark_record_.find(pin_id);
    if (it == output_mark_record_.end()) {
        // Pin never contributed a mark on the output side.
        return;
    }

    const AtomNetlist& netlist = g_vpr_ctx.atom().netlist();
    const t_pb_graph_pin* pb_graph_pin = find_pb_graph_pin(netlist, atom_to_pb, pin_id);

    for (const t_pb* pb : it->second) {
        VTR_ASSERT_SAFE(per_pb_state_.count(pb) > 0);

        const int depth = pb->pb_graph_node->pb_type->depth;
        const int class_id = pb_graph_pin->parent_pin_class[depth];
        VTR_ASSERT(class_id != UNDEFINED);

        remove_mark(pb, /*is_input=*/false, class_id, net_id);
        mark_record_journal_.push_back({pin_id, pb, /*is_input=*/false, -1});
    }
    output_mark_record_.erase(it);
}

void ClusterPinCounter::commit_check() {
    per_pb_state_journal_.clear();
    mark_record_journal_.clear();
}

void ClusterPinCounter::rollback_check() {
    // Replay each journal in reverse. The two journals track independent
    // state (mark records vs. per-pb refcounts).
    while (!mark_record_journal_.empty()) {
        const MarkRecordDelta delta = mark_record_journal_.back();
        mark_record_journal_.pop_back();

        std::unordered_map<AtomPinId, std::vector<const t_pb*>>& record_map = delta.is_input ? input_mark_record_ : output_mark_record_;

        if (delta.change == +1) {
            // Undo an append: pop the last entry.
            auto it = record_map.find(delta.pin);
            VTR_ASSERT(it != record_map.end() && !it->second.empty());
            VTR_ASSERT_SAFE(it->second.back() == delta.pb);
            it->second.pop_back();
            if (it->second.empty()) {
                record_map.erase(it);
            }
        } else {
            VTR_ASSERT(delta.change == -1);
            // Undo a mark-record removal: push pb back. Creates the pin's
            // map entry if remove_*_pin_marks had already erased it.
            record_map[delta.pin].push_back(delta.pb);
        }
    }

    while (!per_pb_state_journal_.empty()) {
        const PerPbStateDelta delta = per_pb_state_journal_.back();
        per_pb_state_journal_.pop_back();

        VTR_ASSERT_SAFE(per_pb_state_.count(delta.pb) > 0);
        auto pb_it = per_pb_state_.find(delta.pb);

        std::unordered_map<AtomNetId, int>& net_counts = delta.is_input ? pb_it->second.input_pin_class_net_counts.at(delta.class_id)
                                                                        : pb_it->second.output_pin_class_net_counts.at(delta.class_id);

        if (delta.change == +1) {
            // Undo an add: decrement, erase at zero.
            int& refcount = net_counts.at(delta.net);
            VTR_ASSERT(refcount > 0);
            refcount -= 1;
            if (refcount == 0) {
                net_counts.erase(delta.net);
            }
        } else {
            VTR_ASSERT(delta.change == -1);
            // Undo a remove: increment, creating the entry if needed.
            net_counts[delta.net] += 1;
        }
    }
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

    const t_pb_type* driver_pb_type = driver_pb->pb_graph_node->pb_type;
    int output_port = 0;
    // Find the port of the pin driving the net as well as the port model.
    AtomPortId driver_port_id = atom_netlist.pin_port(driver_pin_id);
    const t_model_ports* driver_model_port = atom_netlist.port_model(driver_port_id);
    // Find the port id of the port containing the driving pin in the driver_pb_type.
    for (int i = 0; i < driver_pb_type->num_ports; i++) {
        t_port& prim_port = driver_pb_type->ports[i];
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

void ClusterPinCounter::compute_and_mark_pins_used_for_input_pin(AtomPinId pin_id,
                                                                 const t_pb_graph_pin* pb_graph_pin,
                                                                 const t_pb* primitive_pb,
                                                                 AtomNetId net_id,
                                                                 const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                                                                 const AtomPBBimap& atom_to_pb) {
    VTR_ASSERT(pb_graph_pin->port->type == IN_PORT);
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    const AtomBlockId driver_blk_id = atom_ctx.netlist().net_driver_block(net_id);
    const AtomPinId driver_pin_id = atom_ctx.netlist().net_driver(net_id);
    const AtomBlockId prim_blk_id = atom_to_pb.pb_atom(primitive_pb);
    const t_pb* driver_pb = atom_to_pb.atom_pb(driver_blk_id);

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
    for (const t_pb* cur_pb = primitive_pb->parent_pb; cur_pb; cur_pb = cur_pb->parent_pb) {
        const int depth = cur_pb->pb_graph_node->pb_type->depth;
        const int pin_class = pb_graph_pin->parent_pin_class[depth];
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
            add_mark(cur_pb, /*is_input=*/true, pin_class, net_id);
            record_input_mark(pin_id, cur_pb);
        }
    }
}

void ClusterPinCounter::compute_and_mark_pins_used_for_output_pin(AtomPinId pin_id,
                                                                  const t_pb_graph_pin* pb_graph_pin,
                                                                  const t_pb* primitive_pb,
                                                                  AtomNetId net_id,
                                                                  const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                                                                  const AtomPBBimap& atom_to_pb) {
    VTR_ASSERT(pb_graph_pin->port->type == OUT_PORT);
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    const AtomBlockId driver_blk_id = atom_ctx.netlist().net_driver_block(net_id);
    int num_net_sinks = static_cast<int>(atom_ctx.netlist().net_sinks(net_id).size());

    bool all_sinks_in_cur_cluster = false;
    bool all_sinks_in_cur_cluster_computed = false;

    // Once net_sinks_reachable_in_cluster confirms absorption of the current
    // net at some depth D during below loop, the same net is absorbed at every
    // shallower depth (root direction) too. We can skip the check at those
    // shallower ancestors.
    bool confirmed_absorbed = false;

    // Starting from the parent pb of the given primitive, go up in the hierarchy till the root block
    for (const t_pb* cur_pb = primitive_pb->parent_pb; cur_pb; cur_pb = cur_pb->parent_pb) {
        const int depth = cur_pb->pb_graph_node->pb_type->depth;
        const int pin_class = pb_graph_pin->parent_pin_class[depth];
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
                for (AtomPinId sink_pin_id : atom_ctx.netlist().net_sinks(net_id)) {
                    if (atom_cluster[atom_ctx.netlist().pin_block(sink_pin_id)] != driver_cluster) {
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
            add_mark(cur_pb, /*is_input=*/false, pin_class, net_id);
            record_output_mark(pin_id, cur_pb);
        }
    }
}

void ClusterPinCounter::compute_and_mark_pins_used(
    AtomBlockId blk_id,
    const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
    const AtomPBBimap& atom_to_pb) {
    const AtomNetlist& atom_netlist = g_vpr_ctx.atom().netlist();

    const t_pb* cur_pb = atom_to_pb.atom_pb(blk_id);
    VTR_ASSERT(cur_pb != nullptr);

    // Walk through inputs and outputs marking pins off of the same class.
    for (AtomPinId pin_id : atom_netlist.block_pins(blk_id)) {
        AtomNetId net_id = atom_netlist.pin_net(pin_id);

        const t_pb_graph_pin* pb_graph_pin = find_pb_graph_pin(atom_netlist, atom_to_pb, pin_id);

        if (pb_graph_pin->port->type == IN_PORT) {
            compute_and_mark_pins_used_for_input_pin(pin_id, pb_graph_pin, cur_pb, net_id, atom_cluster, atom_to_pb);
        } else {
            compute_and_mark_pins_used_for_output_pin(pin_id, pb_graph_pin, cur_pb, net_id, atom_cluster, atom_to_pb);
        }
    }
}

void ClusterPinCounter::full_recompute_from_molecules(
    const std::vector<PackMoleculeId>& molecules,
    const Prepacker& prepacker,
    const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
    const AtomPBBimap& atom_to_pb) {
    wipe_all_marks_journaled();

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
            compute_and_mark_pins_used(blk_id, atom_cluster, atom_to_pb);
        }
    }
}

bool ClusterPinCounter::check_pins_used_full_reference(t_pb* cur_pb, t_ext_pin_util max_external_pin_util) const {
    const t_pb_type* pb_type = cur_pb->pb_graph_node->pb_type;

    if (!pb_type->is_primitive() && !cur_pb->name.empty()) {
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
                VTR_ASSERT(class_id < root_input_class_size_snapshot_.size());
                class_size = std::max<size_t>(class_size, root_input_class_size_snapshot_[class_id]);
            }

            if (input_size(cur_pb, class_id) > class_size) {
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
                VTR_ASSERT(class_id < root_output_class_size_snapshot_.size());
                class_size = std::max<size_t>(class_size, root_output_class_size_snapshot_[class_id]);
            }

            if (output_size(cur_pb, class_id) > class_size) {
                return false;
            }
        }

        if (cur_pb->child_pbs) {
            for (int child_num = 0; child_num < pb_type->modes[cur_pb->mode].num_pb_type_children; child_num++) {
                if (cur_pb->child_pbs[child_num]) {
                    for (int pb_instance = 0; pb_instance < pb_type->modes[cur_pb->mode].pb_type_children[child_num].num_pb; pb_instance++) {
                        if (!check_pins_used_full_reference(&cur_pb->child_pbs[child_num][pb_instance], max_external_pin_util))
                            return false;
                    }
                }
            }
        }
    }

    return true;
}

/**
 * @brief Test a single (pb, class) pair against its supply, applying the root
 *        clamp when appropriate. Returns true if the class is feasible.
 */
static bool class_is_feasible(const ClusterPinCounter& counter,
                              const t_pb* pb,
                              bool is_input,
                              size_t class_id,
                              t_ext_pin_util max_external_pin_util,
                              const std::vector<size_t>& root_input_snapshot,
                              const std::vector<size_t>& root_output_snapshot) {
    const t_pb_graph_node* pb_graph_node = pb->pb_graph_node;
    size_t class_size = is_input ? pb_graph_node->input_pin_class_sizes[class_id]
                                 : pb_graph_node->output_pin_class_sizes[class_id];

    if (pb->is_root()) {
        const float util = is_input ? max_external_pin_util.input_pin_util
                                    : max_external_pin_util.output_pin_util;
        class_size = std::ceil(util * class_size);
        const std::vector<size_t>& snapshot = is_input ? root_input_snapshot : root_output_snapshot;
        VTR_ASSERT(class_id < snapshot.size());
        class_size = std::max<size_t>(class_size, snapshot[class_id]);
    }

    const size_t current = is_input ? counter.input_size(pb, class_id)
                                    : counter.output_size(pb, class_id);
    return current <= class_size;
}

bool ClusterPinCounter::check_pins_used(t_pb* cur_pb, t_ext_pin_util max_external_pin_util) const {
    // Walk the incremented entries in the state journal, collapse duplicate
    // (pb, class_id, is_input) tuples, and probe each.
    //
    // A (pb, is_input, class_id) tuple can only become infeasible if we
    // incremented it this check. State before this check was feasible,
    // decrements can only help, and the root clamp is handled by
    // snapshot_root_class_sizes.

    /// Identifies a (pb, class_id, is_input) tuple that got incremented this check.
    struct TouchedKey {
        const t_pb* pb; ///< The pb whose class was touched.
        int class_id;   ///< Which pin class within pb.
        bool is_input;  ///< True: input pin class. False: output.
        bool operator<(const TouchedKey& o) const {
            if (pb != o.pb) return pb < o.pb;
            if (is_input != o.is_input) return is_input < o.is_input;
            return class_id < o.class_id;
        }
        bool operator==(const TouchedKey& o) const {
            return pb == o.pb && class_id == o.class_id && is_input == o.is_input;
        }
    };

    // Collect the incremented tuples from the state journal and collapse duplicates.
    std::vector<TouchedKey> touched;
    touched.reserve(per_pb_state_journal_.size());
    for (const PerPbStateDelta& delta : per_pb_state_journal_) {
        if (delta.change != +1) continue;
        touched.push_back({delta.pb, delta.class_id, delta.is_input});
    }
    std::sort(touched.begin(), touched.end());
    touched.erase(std::unique(touched.begin(), touched.end()), touched.end());

    // Probe each touched tuple for feasibility, breaking on the first failure.
    bool incremental_result = true;
    for (const TouchedKey& key : touched) {
        if (key.pb->pb_graph_node->pb_type->is_primitive())
            continue;
        if (key.pb->name == nullptr)
            continue;

        if (!class_is_feasible(*this, key.pb, key.is_input, key.class_id,
                               max_external_pin_util,
                               root_input_class_size_snapshot_,
                               root_output_class_size_snapshot_)) {
            incremental_result = false;
            break;
        }
    }

#ifdef VTR_ASSERT_DEBUG_ENABLED
    // Debug check: run the reference full walk and confirm it agrees.
    const bool full_result = check_pins_used_full_reference(cur_pb, max_external_pin_util);
    if (incremental_result != full_result) {
        VPR_FATAL_ERROR(VPR_ERROR_PACK,
                        "Incremental check_pins_used disagrees with the reference "
                        "full walk: incremental=%s reference=%s\n",
                        incremental_result ? "pass" : "fail",
                        full_result ? "pass" : "fail");
    }
#else
    (void)cur_pb;
#endif

    return incremental_result;
}

void ClusterPinCounter::apply_molecule_delta(PackMoleculeId candidate_id,
                                             const Prepacker& prepacker,
                                             const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                                             const AtomPBBimap& atom_to_pb) {
    VTR_ASSERT_SAFE_MSG(per_pb_state_journal_.empty() && mark_record_journal_.empty(),
                        "Both journals must be empty at the start of a candidate check "
                        "(apply_molecule_delta should run once per candidate check; the previous check must be committed or rolled back first).");
    const t_pack_molecule& molecule = prepacker.get_molecule(candidate_id);
    const AtomNetlist& netlist = g_vpr_ctx.atom().netlist();

    // Build the set of atoms in the molecule for membership tests.
    std::unordered_set<AtomBlockId> molecule_atoms;
    for (AtomBlockId blk : molecule.atom_block_ids) {
        if (blk.is_valid()) molecule_atoms.insert(blk);
    }
    VTR_ASSERT(!molecule_atoms.empty());

    // Every atom in the molecule has already been placed and its cluster recorded by
    // try_place_atom_block_rec, so any of them tells us which cluster we're
    // in. Used to filter "atom is in this cluster" checks below.
    const LegalizationClusterId our_cluster = atom_cluster[*molecule_atoms.begin()];
    VTR_ASSERT(our_cluster.is_valid());

    // Step 1: every net the molecule now drives -> re-evaluate every pre-existing sink.
    // A net has a single driver, so no duplicate net_ids are collected here.
    std::unordered_set<AtomNetId> driven_nets;
    for (AtomBlockId blk : molecule_atoms) {
        for (AtomPinId pin_id : netlist.block_output_pins(blk)) {
            const AtomNetId net_id = netlist.pin_net(pin_id);
            if (net_id.is_valid())
                driven_nets.insert(net_id);
        }
    }
    for (AtomNetId net_id : driven_nets) {
        for (AtomPinId sink_pin : netlist.net_sinks(net_id)) {
            const AtomBlockId sink_atom = netlist.pin_block(sink_pin);
            // Sink belongs to the molecule; Step 3 marks it.
            if (molecule_atoms.count(sink_atom))
                continue;
            // Sink belongs to another cluster; nothing to update here.
            if (atom_cluster[sink_atom] != our_cluster)
                continue;

            const t_pb* sink_prim_pb = atom_to_pb.atom_pb(sink_atom);
            // Sink has not been placed into a primitive pb yet.
            if (sink_prim_pb == nullptr)
                continue;

            remove_input_pin_marks(sink_pin, net_id, atom_to_pb);
            const t_pb_graph_pin* sink_pb_graph_pin = find_pb_graph_pin(netlist, atom_to_pb, sink_pin);
            compute_and_mark_pins_used_for_input_pin(sink_pin, sink_pb_graph_pin, sink_prim_pb, net_id,
                                                     atom_cluster, atom_to_pb);
        }
    }

    // Step 2: every net the molecule now sinks -> re-evaluate the driver.
    // Multiple atoms in the molecule can sink the same net, so a set is required.
    std::unordered_set<AtomNetId> sunk_nets;
    for (AtomBlockId blk : molecule_atoms) {
        for (AtomPinId pin_id : netlist.block_input_pins(blk)) {
            const AtomNetId net_id = netlist.pin_net(pin_id);
            if (net_id.is_valid())
                sunk_nets.insert(net_id);
        }
    }
    for (AtomNetId net_id : sunk_nets) {
        const AtomPinId driver_pin = netlist.net_driver(net_id);
        // Net has no driver (dangling or constant).
        if (!driver_pin.is_valid())
            continue;
        const AtomBlockId driver_atom = netlist.pin_block(driver_pin);
        // Driver belongs to the molecule; Step 3 marks it.
        if (molecule_atoms.count(driver_atom))
            continue;
        // Driver belongs to another cluster; nothing to update here.
        if (atom_cluster[driver_atom] != our_cluster)
            continue;

        const t_pb* driver_prim_pb = atom_to_pb.atom_pb(driver_atom);
        // Driver has not been placed into a primitive pb yet.
        if (driver_prim_pb == nullptr)
            continue;

        remove_output_pin_marks(driver_pin, net_id, atom_to_pb);
        const t_pb_graph_pin* driver_pb_graph_pin = find_pb_graph_pin(netlist, atom_to_pb, driver_pin);
        compute_and_mark_pins_used_for_output_pin(driver_pin, driver_pb_graph_pin, driver_prim_pb, net_id,
                                                  atom_cluster, atom_to_pb);
    }

    // Step 3: mark every pin of every atom in the molecule.
    for (AtomBlockId blk : molecule_atoms) {
        const t_pb* prim_pb = atom_to_pb.atom_pb(blk);
        VTR_ASSERT_SAFE(prim_pb != nullptr);
        VTR_ASSERT_SAFE(prim_pb->pb_graph_node->pb_type->is_primitive());
        VTR_ASSERT(prim_pb->pb_graph_node->pb_type->blif_model != nullptr);
        compute_and_mark_pins_used(blk, atom_cluster, atom_to_pb);
    }
}

/**
 * @brief Compare two per class net -> count maps and fatally error on any
 *        mismatch, reporting the diff for debugging.
 */
static void compare_class_maps_or_die(const t_pb* pb,
                                      bool is_input,
                                      const std::vector<std::unordered_map<AtomNetId, int>>& actual,
                                      const std::vector<std::unordered_map<AtomNetId, int>>& reference) {
    VTR_ASSERT(actual.size() == reference.size());

    for (size_t class_id = 0; class_id < actual.size(); ++class_id) {
        const std::unordered_map<AtomNetId, int>& actual_class = actual[class_id];
        const std::unordered_map<AtomNetId, int>& reference_class = reference[class_id];

        if (actual_class == reference_class) {
            continue;
        }

        std::string diff;
        for (const auto& [net, refcount] : actual_class) {
            auto it = reference_class.find(net);
            if (it == reference_class.end()) {
                diff += " actual has net " + std::to_string(static_cast<size_t>(net)) + " (count " + std::to_string(refcount) + "), reference does not;";
            } else if (it->second != refcount) {
                diff += " net " + std::to_string(static_cast<size_t>(net)) + " count actual=" + std::to_string(refcount) + " reference=" + std::to_string(it->second) + ";";
            }
        }
        for (const auto& [net, refcount] : reference_class) {
            if (actual_class.find(net) == actual_class.end()) {
                diff += " reference has net " + std::to_string(static_cast<size_t>(net)) + " (count " + std::to_string(refcount) + "), actual does not;";
            }
        }

        VPR_FATAL_ERROR(VPR_ERROR_PACK,
                        "ClusterPinCounter state diverged from full recompute at pb '%s', "
                        "%s class %zu:%s\n",
                        pb->name ? pb->name : "<unnamed>",
                        is_input ? "input" : "output",
                        class_id,
                        diff.c_str());
    }
}

void ClusterPinCounter::verify_against_full_recompute(const std::vector<PackMoleculeId>& molecules,
                                                      const Prepacker& prepacker,
                                                      const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                                                      const AtomPBBimap& atom_to_pb) const {
    // Build a scratch counter with the same pb topology as this counter and
    // let it fill its state via the full recompute path.
    ClusterPinCounter scratch;
    for (const auto& [pb, _] : per_pb_state_) {
        scratch.allocate_pin_count_state(pb);
    }
    scratch.full_recompute_from_molecules(molecules, prepacker, atom_cluster, atom_to_pb);

    for (const auto& [pb, this_state] : per_pb_state_) {
        const PerPbState& scratch_state = scratch.per_pb_state_.at(pb);
        compare_class_maps_or_die(pb, /*is_input=*/true,
                                  this_state.input_pin_class_net_counts,
                                  scratch_state.input_pin_class_net_counts);
        compare_class_maps_or_die(pb, /*is_input=*/false,
                                  this_state.output_pin_class_net_counts,
                                  scratch_state.output_pin_class_net_counts);
    }
}

/**
 * @brief Collect every pb reachable from the given root via child_pbs traversal
 *        into the given set.
 */
static void collect_reachable_pbs(const t_pb* pb, std::unordered_set<const t_pb*>& reachable_pbs) {
    if (pb == nullptr)
        return;
    reachable_pbs.insert(pb);

    const t_pb_type* pb_type = pb->pb_graph_node->pb_type;
    if (pb_type->is_primitive())
        return;
    if (pb->child_pbs == nullptr)
        return;

    const int mode = pb->mode;
    for (int child_num = 0; child_num < pb_type->modes[mode].num_pb_type_children; ++child_num) {
        if (!pb->child_pbs[child_num])
            continue;
        for (int pb_instance = 0; pb_instance < pb_type->modes[mode].pb_type_children[child_num].num_pb; ++pb_instance) {
            collect_reachable_pbs(&pb->child_pbs[child_num][pb_instance], reachable_pbs);
        }
    }
}

void ClusterPinCounter::assert_all_pbs_reachable_from(const t_pb* cluster_root) const {
    std::unordered_set<const t_pb*> reachable_pbs;
    collect_reachable_pbs(cluster_root, reachable_pbs);

    for (const auto& [pb, _] : per_pb_state_) {
        if (reachable_pbs.count(pb) == 0) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "ClusterPinCounter tracks a pb (raw pointer %p) that is not "
                            "reachable from the cluster root. A free/cleanup site freed "
                            "the pb without calling deallocate_pin_count_state_recursive "
                            "first, leaving a dangling pointer as a key.\n",
                            static_cast<const void*>(pb));
        }
    }
}
