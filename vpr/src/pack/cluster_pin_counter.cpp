/**
 * @file
 * @brief Implementation of ClusterPinCounter.
 */

#include "cluster_pin_counter.h"

#include <algorithm>

#include "physical_types.h"
#include "vpr_types.h"
#include "vtr_assert.h"

void ClusterPinCounter::ensure_allocated(const t_pb* pb) {
    VTR_ASSERT(pb != nullptr);
    if (per_pb_state_.count(pb) != 0) {
        return;
    }

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

void ClusterPinCounter::commit_lookahead() {
    for (auto& kv : per_pb_state_) {
        PerPbState& state = kv.second;
        state.committed_input_pin_class_nets = state.lookahead_input_pin_class_nets;
        state.committed_output_pin_class_nets = state.lookahead_output_pin_class_nets;
    }
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
