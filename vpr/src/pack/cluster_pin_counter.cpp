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
    // VTR_ASSERT_MSG(per_pb_state_.count(pb) == 0, 
    //     "Pin counting state allocation should be called once per each pb in a cluster");

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

// !!! WARNING — LEGACY-MIRRORING QUIRK !!!
//
// This commit uses "peak-size" semantics: for each class, committed only ever
// grows to match the current lookahead; it never shrinks even when the current
// lookahead is smaller (e.g., because a net was absorbed inside the cluster).
//
// This deliberately reproduces a quirk in legacy commit_lookahead_pins_used
// (vpr/src/pack/cluster_legalizer.cpp), which stores committed state as
//   std::unordered_map<size_t /*vector index*/, AtomNetId>
// and uses insert({index, net}) with vector-index-as-key. Since insert does
// not overwrite existing keys, once a class has been committed at some size N
// it stays >= N forever within a cluster's lifetime, even if subsequent
// commits' lookahead has size < N. The map's stored AtomNetId values are also
// frozen at whatever the first commit that inserted that key contributed —
// they are never read anywhere, but they exist.
//
// The only thing anyone actually reads from committed state is `.size()`:
//   - check_lookahead_pins_used (at root only) floors class_size at committed.size()
//   - get_num_cluster_inputs_available sums committed.size() at root
// so matching the size is what makes the new class behaviorally equivalent to
// legacy. The stored AtomNetIds diverge — that's fine; nobody reads them.
//
// Why we mirror the quirk instead of using the semantically obvious
// `committed = lookahead` (current-usage semantics):
//   - Step 1 of the pin-counting rewrite promises byte-for-byte behavioral
//     equivalence with legacy. Changing the semantics here would silently
//     alter the pin-feasibility filter at root pbs (peak is more permissive
//     than current) and change get_num_cluster_inputs_available, potentially
//     shifting packing decisions and QoR.
//   - Any semantic cleanup should be a separate PR after legacy is removed,
//     with a proper QoR study (vtr_reg_strong at minimum) to confirm the
//     change is safe.
//
// TODO (post-migration): once legacy commit_lookahead_pins_used is deleted,
// replace the peak-mirror body below with the current-usage version shown in
// the commented block. Peak semantics is undocumented and arguably a bug: the
// floor's stated intent (see comment near line 975 of cluster_legalizer.cpp)
// is only to protect the seed's committed count, not any historical peak.
// Peak just falls out of the unordered_map insert quirk. Run a QoR study
// (vtr_reg_strong minimum) when making the swap to confirm impact.
void ClusterPinCounter::commit_lookahead() {
    // ---- Legacy-mirroring peak-size behavior (step-1 verification requires this) ----
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

const ClusterPinCounter::PerPbState* ClusterPinCounter::find(const t_pb* pb) const {
    auto it = per_pb_state_.find(pb);
    if (it == per_pb_state_.end()) {
        return nullptr;
    }
    return &it->second;
}
