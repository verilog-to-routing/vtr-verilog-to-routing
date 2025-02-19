
#pragma once

#include "vpr_types.h"

class FlatPlacementInfo;
class PlacementContext;

void try_place(const Netlist<>& net_list,
               const t_placer_opts& placer_opts,
               const t_router_opts& router_opts,
               const t_analysis_opts& analysis_opts,
               const t_noc_opts& noc_opts,
               t_chan_width_dist chan_width_dist,
               t_det_routing_arch* det_routing_arch,
               std::vector<t_segment_inf>& segment_inf,
               const std::vector<t_direct_inf>& directs,
               const FlatPlacementInfo& flat_placement_info,
               bool is_flat);

/**
 * @brief Initialize the variables stored within the placement context. This
 *        must be called before using the Placer class.
 *
 *  @param place_ctx
 *      The placement context to initialize.
 *  @param placer_opts
 *      The options passed into the placer.
 *  @param noc_opts
 *      The options passed into the placer for NoCs.
 *  @param directs
 *      A list of the direct connections in the architecture.
 */
void init_placement_context(PlacementContext& place_ctx,
                            const t_placer_opts& placer_opts,
                            const t_noc_opts& noc_opts,
                            const std::vector<t_direct_inf>& directs);

/**
 * @brief Clean variables from the placement context which are not used outside
 *        of placement.
 *
 * These are some variables that are stored in the placement context and are
 * only used in placement; while there are some that are used outside of
 * placement. This method frees up the memory of the variables used only within
 * placement.
 */
void clean_placement_context(PlacementContext& place_ctx);

