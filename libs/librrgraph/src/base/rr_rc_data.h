#pragma once

#include <unordered_map>
#include <vector>

#include "rr_graph_fwd.h"
#include "rr_node_types.h"
#include "vtr_assert.h"

/**
 * @brief Fly-weighted resistance and capacitance values of RR nodes.
 *
 * Each distinct (R, C) pair is stored once and is addressed by a NodeRCIndex.
 */
class RRRCData {
  public:
    /// @brief Returns the index of the entry matching R and C, creating it if there is none.
    NodeRCIndex find_create(float R, float C);

    /// @brief Returns the (R, C) pair at the given index.
    const t_rr_rc_data& operator[](NodeRCIndex index) const {
        VTR_ASSERT_SAFE(size_t(index) < values_.size());
        return values_[size_t(index)];
    }

    /// @brief Returns the number of distinct (R, C) pairs.
    size_t size() const {
        return values_.size();
    }

  private:
    /// Distinct (R, C) pairs in creation order
    std::vector<t_rr_rc_data> values_;
    /// Index into values_ keyed on the bit patterns of R and C
    std::unordered_map<uint64_t, NodeRCIndex> index_;
};
