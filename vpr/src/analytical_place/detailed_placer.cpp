/**
 * @file
 * @author  Alex Singer
 * @date    February 2025
 * @brief   Implementation of the Detailed Placers.
 */

#include "detailed_placer.h"
#include <memory>
#include "PlacementDelayModelCreator.h"
#include "ap_flow_enums.h"
#include "atom_netlist.h"
#include "clustered_netlist.h"
#include "clustered_netlist_utils.h"
#include "echo_files.h"
#include "flat_placement_types.h"
#include "globals.h"
#include "physical_types.h"
#include "place_and_route.h"
#include "place_delay_model.h"
#include "placer.h"
#include "vpr_error.h"
#include "vpr_types.h"
#include "vpr_utils.h"
#include "vtr_time.h"

std::unique_ptr<DetailedPlacer> make_detailed_placer(e_ap_detailed_placer detailed_placer_type,
                                                     const BlkLocRegistry& curr_clustered_placement,
                                                     const AtomNetlist& atom_netlist,
                                                     const ClusteredNetlist& clustered_netlist,
                                                     t_vpr_setup& vpr_setup,
                                                     const t_arch& arch) {
    switch (detailed_placer_type) {
        case e_ap_detailed_placer::Identity:
            return std::make_unique<IdentityDetailedPlacer>();
        case e_ap_detailed_placer::Annealer:
            return std::make_unique<AnnealerDetailedPlacer>(curr_clustered_placement,
                                                            atom_netlist,
                                                            clustered_netlist,
                                                            vpr_setup,
                                                            arch);
        default:
            VPR_FATAL_ERROR(VPR_ERROR_AP,
                            "Unrecognized detailed placer type");
    }
}

AnnealerDetailedPlacer::AnnealerDetailedPlacer(const BlkLocRegistry& curr_clustered_placement,
                                               const AtomNetlist& atom_netlist,
                                               const ClusteredNetlist& clustered_netlist,
                                               t_vpr_setup& vpr_setup,
                                               const t_arch& arch)
    : DetailedPlacer()
    // TODO: These two variables needed to be stored in the class since
    //       the Placer stores a reference to these objects. These
    //       should really be initialized and stored into the Placer
    //       class directly.
    , pb_gpin_lookup_(g_vpr_ctx.device().logical_block_types)
    , netlist_pin_lookup_(clustered_netlist, atom_netlist, pb_gpin_lookup_) {
    // Initialize the place delay model.
    // TODO: This initialization is complicated. Should be moved within create_delay_model
    //       or something.
    std::shared_ptr<PlaceDelayModel> place_delay_model;
    if (vpr_setup.PlacerOpts.place_algorithm.is_timing_driven()) {
        place_delay_model = PlacementDelayModelCreator::create_delay_model(vpr_setup.PlacerOpts,
                                                                           vpr_setup.RouterOpts,
                                                                           (const Netlist<>&)clustered_netlist,
                                                                           vpr_setup.RoutingArch,
                                                                           vpr_setup.Segments,
                                                                           arch.Chans,
                                                                           arch.directs,
                                                                           false /*is_flat*/);
        if (isEchoFileEnabled(E_ECHO_PLACEMENT_DELTA_DELAY_MODEL)) {
            place_delay_model->dump_echo(getEchoFileName(E_ECHO_PLACEMENT_DELTA_DELAY_MODEL));
        }
    }

    placer_ = std::make_unique<Placer>((const Netlist<>&)clustered_netlist,
                                       curr_clustered_placement,
                                       vpr_setup.PlacerOpts,
                                       vpr_setup.AnalysisOpts,
                                       vpr_setup.NocOpts,
                                       pb_gpin_lookup_,
                                       netlist_pin_lookup_,
                                       FlatPlacementInfo(),
                                       place_delay_model,
                                       vpr_setup.PlacerOpts.place_auto_init_t_scale,
                                       g_vpr_ctx.placement().cube_bb,
                                       false /*is_flat*/,
                                       false /*quiet*/);
}

void AnnealerDetailedPlacer::optimize_placement() {
    // Create a scoped timer for the detailed placer.
    vtr::ScopedStartFinishTimer full_legalizer_timer("AP Detailed Placer");

    // Prevent the annealer from directly modifying the global legal placement.
    // It should only modify its own, local placement.
    g_vpr_ctx.mutable_placement().lock_loc_vars();

    // Run the simulated annealer.
    placer_->place();

    // Copy the placement solution into the global placement solution.
    placer_->update_global_state();

    // Since the placement was modified, need to resynchronize the pins in the
    // clusters.
    post_place_sync();
}
