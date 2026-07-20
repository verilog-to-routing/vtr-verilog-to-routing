/**
 * @file
 * @brief Implementation of ClusterPinCounter.
 */

#include "cluster_pin_counter.h"

#include <algorithm>

#include "physical_types.h"
#include "vpr_types.h"
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

