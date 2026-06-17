#pragma once

#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "physical_types.h"

struct DeviceContext;

/**
 * @brief Architecture-level lookup of the connectivity defined by the <directlist>,
 *        used by the clustered netlist legality check.
 *
 * A top-level output pin with Fc_out = 0 can only reach a sink that is wired to it by a
 * <direct> entry. The clustered netlist checker uses this class to verify that every
 * Fc_out = 0 pin chosen as a net's exit pin is backed by a matching <direct> connection
 * to that net's external sinks.
 */
class DirectConnectionLegality {
  public:
    /**
     * @brief Empty lookup — all queries return false.
     */
    DirectConnectionLegality() = default;

    /**
     * @brief Build the lookup from the architecture's <directlist>.
     *
     * @param directs The architecture's <direct> connection definitions.
     * @param device_ctx Source of the delayless switch index and the logical block types.
     */
    DirectConnectionLegality(const std::vector<t_direct_inf>& directs, const DeviceContext& device_ctx);

    /**
     * @brief Returns true if the given source pin and sink pin are connected by a <direct>.
     */
    bool is_direct_legal(t_logical_block_type_ptr from_lb, int from_top_pin, t_logical_block_type_ptr to_lb, int to_top_pin) const;

  private:
    /// [from_lb_index] -> { from_pin -> { (to_lb_index, to_pin), ... } }.
    /// Outer vector sized to logical_block_types.size() at construction.
    std::vector<std::unordered_map<int, std::set<std::pair<int, int>>>> forward_by_from_lb_;
};
