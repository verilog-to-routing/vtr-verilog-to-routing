#pragma once

#include <unordered_map>
#include <vector>

#include "rr_node_types.h"

/**
 * @brief Fly-weighted resistance and capacitance values of RR nodes.
 *
 * Each distinct (R, C) pair is stored once. Node RC indices point into values().
 */
class RRRCData {
  public:
    /// @brief Returns the index of the entry matching R and C, creating it if there is none.
    NodeRCIndex find_create(float R, float C);

    const std::vector<t_rr_rc_data>& values() const {
        return values_;
    }

  private:
    /// Distinct (R, C) pairs in creation order
    std::vector<t_rr_rc_data> values_;
    /// Index into values_ keyed on the bit patterns of R and C
    std::unordered_map<uint64_t, NodeRCIndex> index_;
};
