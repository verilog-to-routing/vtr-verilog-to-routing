/**
 * @file
 * @author  Haydar Cakan
 * @date    July 2026
 * @brief   Implementation of ClusterPinCounter.
 *
 * Contains the per pb state storage and query methods, the marking routines
 * that walk each atom pin from its primitive pb up to the cluster root and
 * mark the pin class the pin's net uses at each level, and the check-frame
 * mechanism (snapshot -> recompute -> check -> commit/rollback) used by the
 * pin feasibility filter to speculatively evaluate candidate molecules.
 */

#include "cluster_pin_counter.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_set>

#include "atom_netlist.h"
#include "atom_pb_bimap.h"
#include "globals.h"
#include "physical_types.h"
#include "vpr_context.h"
#include "vpr_error.h"
#include "vpr_types.h"
#include "vpr_utils.h"
#include "vtr_assert.h"

void ClusterPinCounter::allocate_pin_count_state(const t_pb* pb) {
    VTR_ASSERT(pb != nullptr);
    VTR_ASSERT_MSG(per_pb_state_.count(pb) == 0,
                   "Pin counting state should be empty before allocation");

    const t_pb_graph_node* pb_graph_node = pb->pb_graph_node;
    VTR_ASSERT(pb_graph_node != nullptr);

    const size_t num_input_classes = pb_graph_node->input_pin_class_sizes.size();
    const size_t num_output_classes = pb_graph_node->output_pin_class_sizes.size();

    PerPbState& state = per_pb_state_[pb];
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

size_t ClusterPinCounter::input_size(const t_pb* pb, size_t class_id) const {
    const PerPbState& state = per_pb_state_.at(pb);
    return state.input_pin_class_net_counts.at(class_id).size();
}

size_t ClusterPinCounter::output_size(const t_pb* pb, size_t class_id) const {
    const PerPbState& state = per_pb_state_.at(pb);
    return state.output_pin_class_net_counts.at(class_id).size();
}

void ClusterPinCounter::snapshot_root_class_sizes(const t_pb* root) {
    VTR_ASSERT(root != nullptr && root->is_root());
    const PerPbState& state = per_pb_state_.at(root);

    root_input_class_size_snapshot_.resize(state.input_pin_class_net_counts.size());
    for (size_t c = 0; c < state.input_pin_class_net_counts.size(); ++c) {
        root_input_class_size_snapshot_[c] = state.input_pin_class_net_counts[c].size();
    }
    root_output_class_size_snapshot_.resize(state.output_pin_class_net_counts.size());
    for (size_t c = 0; c < state.output_pin_class_net_counts.size(); ++c) {
        root_output_class_size_snapshot_[c] = state.output_pin_class_net_counts[c].size();
    }
}

void ClusterPinCounter::add_mark(const t_pb* pb, bool is_input, size_t class_id, AtomNetId net) {
    PerPbState& state = per_pb_state_.at(pb);
    auto& map = is_input ? state.input_pin_class_net_counts.at(class_id)
                         : state.output_pin_class_net_counts.at(class_id);
    uint16_t& cnt = map[net]; // inserts as 0 if missing
    VTR_ASSERT(cnt < std::numeric_limits<uint16_t>::max());
    cnt += 1;
    // Output classes should never exceed a refcount of 1 for a given net:
    // each net has a single driver, so at most one mark per (pb, class, net).
    VTR_ASSERT_SAFE(is_input || cnt == 1);

    journal_.push_back({pb, net, static_cast<uint32_t>(class_id), is_input, +1});
}

void ClusterPinCounter::remove_mark(const t_pb* pb, bool is_input, size_t class_id, AtomNetId net) {
    PerPbState& state = per_pb_state_.at(pb);
    auto& map = is_input ? state.input_pin_class_net_counts.at(class_id)
                         : state.output_pin_class_net_counts.at(class_id);
    auto it = map.find(net);
    VTR_ASSERT(it != map.end() && it->second > 0);
    it->second -= 1;
    if (it->second == 0) {
        map.erase(it);
    }

    journal_.push_back({pb, net, static_cast<uint32_t>(class_id), is_input, -1});
}

void ClusterPinCounter::wipe_all_marks_journaled() {
    // Snapshot entries per class before mutating: remove_mark modifies the map.
    for (auto& [pb, state] : per_pb_state_) {
        for (size_t c = 0; c < state.input_pin_class_net_counts.size(); ++c) {
            std::vector<std::pair<AtomNetId, uint16_t>> entries(
                state.input_pin_class_net_counts[c].begin(),
                state.input_pin_class_net_counts[c].end());
            for (const auto& [net, cnt] : entries) {
                for (uint16_t i = 0; i < cnt; ++i) {
                    remove_mark(pb, /*is_input=*/true, c, net);
                }
            }
        }
        for (size_t c = 0; c < state.output_pin_class_net_counts.size(); ++c) {
            std::vector<std::pair<AtomNetId, uint16_t>> entries(
                state.output_pin_class_net_counts[c].begin(),
                state.output_pin_class_net_counts[c].end());
            for (const auto& [net, cnt] : entries) {
                for (uint16_t i = 0; i < cnt; ++i) {
                    remove_mark(pb, /*is_input=*/false, c, net);
                }
            }
        }
    }
}

void ClusterPinCounter::commit_check() {
    journal_.clear();
}

void ClusterPinCounter::rollback_check() {
    // Replay in reverse: apply the negation of each recorded change.
    while (!journal_.empty()) {
        const MarkDelta e = journal_.back();
        journal_.pop_back();

        // Defence in depth: tolerate pbs that have been erased between
        // journal record and rollback. Prefer ordering rollback before
        // deallocation, but do not crash if the caller got the ordering
        // wrong.
        auto pb_it = per_pb_state_.find(e.pb);
        if (pb_it == per_pb_state_.end()) {
            continue;
        }

        auto& map = e.is_input ? pb_it->second.input_pin_class_net_counts.at(e.class_id)
                               : pb_it->second.output_pin_class_net_counts.at(e.class_id);

        if (e.change == +1) {
            // Undo an add: decrement, erase at zero.
            auto it = map.find(e.net);
            VTR_ASSERT(it != map.end() && it->second > 0);
            it->second -= 1;
            if (it->second == 0) {
                map.erase(it);
            }
        } else {
            VTR_ASSERT(e.change == -1);
            // Undo a remove: increment, creating the entry if needed.
            map[e.net] += 1;
        }
    }
}

/**
 * @brief Check whether every sink of the given net is reachable from the
 *        given driver pb_graph_pin at the given depth.
 */
static bool net_sinks_reachable_in_cluster(const t_pb_graph_pin* driver_pb_gpin, const int depth, const AtomNetId net_id, const AtomPBBimap& atom_to_pb) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    std::unordered_set<const t_pb_graph_pin*> sink_pb_gpins;
    for (const AtomPinId pin_id : atom_ctx.netlist().net_sinks(net_id)) {
        const t_pb_graph_pin* sink_pb_gpin = find_pb_graph_pin(atom_ctx.netlist(), atom_to_pb, pin_id);
        VTR_ASSERT(sink_pb_gpin);
        sink_pb_gpins.insert(sink_pb_gpin);
    }

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
 */
static t_pb_graph_pin* get_driver_pb_graph_pin(const t_pb* driver_pb, const AtomPinId driver_pin_id) {
    const AtomNetlist& atom_netlist = g_vpr_ctx.atom().netlist();

    const auto driver_pb_type = driver_pb->pb_graph_node->pb_type;
    int output_port = 0;
    auto driver_port_id = atom_netlist.pin_port(driver_pin_id);
    auto driver_model_port = atom_netlist.port_model(driver_port_id);
    for (int i = 0; i < driver_pb_type->num_ports; i++) {
        auto& prim_port = driver_pb_type->ports[i];
        if (prim_port.type == OUT_PORT) {
            if (prim_port.model_port == driver_model_port) {
                return &(driver_pb->pb_graph_node->output_pins[output_port][atom_netlist.pin_port_bit(driver_pin_id)]);
            }
            output_port++;
        }
    }

    VTR_ASSERT(false);
    return nullptr;
}

void ClusterPinCounter::compute_and_mark_pins_used_for_input_pin(const t_pb_graph_pin* pb_graph_pin,
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

        if (output_pb_graph_pin) {
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

        if (!is_reachable) {
            add_mark(cur_pb, /*is_input=*/true, pin_class, net_id);
        }
    }
}

void ClusterPinCounter::compute_and_mark_pins_used_for_output_pin(const t_pb_graph_pin* pb_graph_pin,
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

    for (auto cur_pb = primitive_pb->parent_pb; cur_pb; cur_pb = cur_pb->parent_pb) {
        const auto depth = cur_pb->pb_graph_node->pb_type->depth;
        const auto pin_class = pb_graph_pin->parent_pin_class[depth];
        VTR_ASSERT(pin_class != UNDEFINED);

        if (confirmed_absorbed) {
            continue;
        }

        bool net_exits_cluster = true;

        if (pb_graph_pin->num_connectable_primitive_input_pins[depth] >= num_net_sinks) {
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
                // TODO: I should cache the absorbed outputs, once net is absorbed,
                //       net is forever absorbed, no point in rechecking every time
                //       Caching within one pin evaluation is implemented by
                //       confirmed_absorbed; leaving this TODO for the incremental case:
                //       caching absorbed nets across candidate checks within a cluster.
                if (net_sinks_reachable_in_cluster(pb_graph_pin, depth, net_id, atom_to_pb)) {
                    confirmed_absorbed = true;
                    net_exits_cluster = false;
                }
            }
        }

        if (net_exits_cluster) {
            add_mark(cur_pb, /*is_input=*/false, pin_class, net_id);
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

    for (auto pin_id : atom_netlist.block_pins(blk_id)) {
        auto net_id = atom_netlist.pin_net(pin_id);
        const t_pb_graph_pin* pb_graph_pin = find_pb_graph_pin(atom_netlist, atom_to_pb, pin_id);

        if (pb_graph_pin->port->type == IN_PORT) {
            compute_and_mark_pins_used_for_input_pin(pb_graph_pin, cur_pb, net_id, atom_cluster, atom_to_pb);
        } else {
            compute_and_mark_pins_used_for_output_pin(pb_graph_pin, cur_pb, net_id, atom_cluster, atom_to_pb);
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

bool ClusterPinCounter::check_pins_used(t_pb* cur_pb, t_ext_pin_util max_external_pin_util) const {
    const t_pb_type* pb_type = cur_pb->pb_graph_node->pb_type;

    if (!pb_type->is_primitive() && cur_pb->name) {
        for (size_t class_id = 0; class_id < cur_pb->pb_graph_node->input_pin_class_sizes.size(); class_id++) {
            size_t class_size = cur_pb->pb_graph_node->input_pin_class_sizes[class_id];

            if (cur_pb->is_root()) {
                // Scale the class size by the maximum external pin utilization factor.
                // Use ceil to avoid classes of size 1 from being scaled to zero.
                class_size = std::ceil(max_external_pin_util.input_pin_util * class_size);
                // The clamp: if the accepted (pre-check) usage already exceeded the
                // scaled supply, raise the supply to that pre-check level. Needed
                // because the seed molecule is packed with FULL_EXTERNAL_PIN_UTIL,
                // so a seed that uses more pins than the target utilization would
                // permanently fail without this raise. Uses the snapshot taken at
                // the start of this candidate check; after the committed/lookahead
                // collapse we no longer track a separate "committed" size.
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
                class_size = std::ceil(max_external_pin_util.output_pin_util * class_size);
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
                        if (!check_pins_used(&cur_pb->child_pbs[child_num][pb_instance], max_external_pin_util))
                            return false;
                    }
                }
            }
        }
    }

    return true;
}

/**
 * @brief Compare two per class net -> count maps and fatally error on any
 *        mismatch, reporting the diff for debugging.
 */
static void compare_class_maps_or_die(const t_pb* pb,
                                      bool is_input,
                                      const std::vector<std::unordered_map<AtomNetId, uint16_t>>& actual,
                                      const std::vector<std::unordered_map<AtomNetId, uint16_t>>& reference) {
    VTR_ASSERT(actual.size() == reference.size());

    for (size_t class_id = 0; class_id < actual.size(); ++class_id) {
        const auto& a = actual[class_id];
        const auto& r = reference[class_id];

        if (a == r) {
            continue;
        }

        std::string diff;
        for (const auto& [net, cnt] : a) {
            auto it = r.find(net);
            if (it == r.end()) {
                diff += " actual has net " + std::to_string(size_t(net)) + " (count " + std::to_string(cnt) + "), reference does not;";
            } else if (it->second != cnt) {
                diff += " net " + std::to_string(size_t(net)) + " count actual=" + std::to_string(cnt) + " reference=" + std::to_string(it->second) + ";";
            }
        }
        for (const auto& [net, cnt] : r) {
            if (a.find(net) == a.end()) {
                diff += " reference has net " + std::to_string(size_t(net)) + " (count " + std::to_string(cnt) + "), actual does not;";
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

void ClusterPinCounter::verify_against_full_recompute(
    const std::vector<PackMoleculeId>& molecules,
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
static void collect_reachable_pbs(const t_pb* pb, std::unordered_set<const t_pb*>& out) {
    if (pb == nullptr)
        return;
    out.insert(pb);

    const t_pb_type* pb_type = pb->pb_graph_node->pb_type;
    if (pb_type->is_primitive())
        return;
    if (pb->child_pbs == nullptr)
        return;

    const int mode = pb->mode;
    for (int c = 0; c < pb_type->modes[mode].num_pb_type_children; ++c) {
        if (!pb->child_pbs[c])
            continue;
        for (int i = 0; i < pb_type->modes[mode].pb_type_children[c].num_pb; ++i) {
            collect_reachable_pbs(&pb->child_pbs[c][i], out);
        }
    }
}

void ClusterPinCounter::assert_all_pbs_reachable_from(const t_pb* cluster_root) const {
    std::unordered_set<const t_pb*> reachable;
    collect_reachable_pbs(cluster_root, reachable);

    for (const auto& [pb, _] : per_pb_state_) {
        if (reachable.count(pb) == 0) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "ClusterPinCounter tracks a pb (raw pointer %p) that is not "
                            "reachable from the cluster root. A free/cleanup site freed "
                            "the pb without calling deallocate_pin_count_state_recursive "
                            "first, leaving a dangling pointer as a key.\n",
                            static_cast<const void*>(pb));
        }
    }
}
