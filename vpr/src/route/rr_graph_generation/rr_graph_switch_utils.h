
#pragma once

/**
 * @file
 * @brief Utilities for creating and initializing rr_switch structures from architecture switches.
 *
 * This header defines functions that translate high-level architecture switch
 * descriptions (`t_arch_switch_inf`) into detailed rr_switch items used in RR graph.
 * These functions:
 *   - Copy and resolve switch electrical parameters into `t_rr_switch_inf`.
 *   - Expand architecture switches into fanin-specific rr_switch variants.
 *   - Provide mappings from (arch_switch, fanin) --> rr_switch index.
 *
 * They are invoked during RR graph construction to allocate, initialize,
 * and remap all switch information.
 */

#include <map>

#include "rr_graph_fwd.h"
#include "rr_node_types.h"

struct t_arch_switch_inf;
struct t_rr_switch_inf;
class RRGraphBuilder;

/**
 * @brief Initialize one rr_switch entry from an architecture switch.
 *
 * Copies electrical parameters from the architecture switch at @p arch_switch_idx,
 * resolving fanin-dependent delay and buffer sizing into a concrete rr_switch entry.
 *
 * @param rr_graph_builder   RR graph builder providing access to rr_switch storage.
 * @param arch_sw_inf        Map of architecture switch indices to their definitions.
 * @param arch_switch_idx    Index of the source architecture switch.
 * @param rr_switch_idx      Index of the destination rr_switch to fill.
 * @param fanin              Fan-in used to evaluate delay (e.g., Tdel(fanin)).
 * @param R_minW_nmos        Reference R for a minimum-width nMOS (for AUTO buf sizing).
 * @param R_minW_pmos        Reference R for a minimum-width pMOS (for AUTO buf sizing).
 */
void load_rr_switch_from_arch_switch(RRGraphBuilder& rr_graph_builder,
                                     const std::map<int, t_arch_switch_inf>& arch_sw_inf,
                                     int arch_switch_idx,
                                     RRSwitchId rr_switch_idx,
                                     int fanin,
                                     const float R_minW_nmos,
                                     const float R_minW_pmos);

/// @brief Create a fully-specified rr_switch from a single architecture switch.
/// @return A populated rr_switch descriptor.
t_rr_switch_inf create_rr_switch_from_arch_switch(const t_arch_switch_inf& arch_sw_inf,
                                                  const float R_minW_nmos,
                                                  const float R_minW_pmos);

/**
 * @brief Allocate and populate rr_switch entries for all fan-ins used in the RR graph.
 *
 * Expands each architecture switch into one or more rr_switches keyed by observed
 * fan-ins; updates RR nodes to reference rr_switch indices; and provides a mapping
 * from (arch_switch, fanin) --> rr_switch. Also returns a representative IPIN
 * connection-block switch index.
 *
 * @param rr_graph_builder          RR graph builder providing storage and remapping.
 * @param switch_fanin_remap        Output: for each arch switch, maps fanin --> rr_switch index.
 * @param arch_sw_inf               Map of architecture switch indices to their definitions.
 * @param R_minW_nmos               Reference R for a minimum-width nMOS (for AUTO buf sizing).
 * @param R_minW_pmos               Reference R for a minimum-width pMOS (for AUTO buf sizing).
 * @param wire_to_arch_ipin_switch  Architecture index of the IPIN connection-block switch.
 * @param wire_to_rr_ipin_switch    Output: rr_switch index of the representative IPIN cblock switch.
 */
void alloc_and_load_rr_switch_inf(RRGraphBuilder& rr_graph_builder,
                                  t_arch_switch_fanin& switch_fanin_remap,
                                  const std::map<int, t_arch_switch_inf>& arch_sw_inf,
                                  const float R_minW_nmos,
                                  const float R_minW_pmos,
                                  const int wire_to_arch_ipin_switch,
                                  RRSwitchId& wire_to_rr_ipin_switch);
