/**
 * @file
 * @author  Yulang (Robert) Luo
 * @date    October 2025
 * @brief   The definitions of the AP Draw Manager class which is used
 *          to handle graphics updates during analytical placement.
 */

#include "ap_draw_manager.h"
#include "vpr_types.h"

#ifndef NO_GRAPHICS
#include "draw.h"
#include "draw_global.h"
#include "globals.h"
#include "partial_placement.h"
#include "PreClusterTimingManager.h"
#include "ap_netlist_utils.h"
#endif

void init_ap_graphics(const t_vpr_setup& vpr_setup, const t_arch& arch) {
#ifndef NO_GRAPHICS
    bool is_flat = vpr_setup.RouterOpts.flat_routing;
    init_graphics_state(vpr_setup.ShowGraphics, vpr_setup.GraphPause,
                        vpr_setup.RouterOpts.route_type, vpr_setup.SaveGraphics,
                        vpr_setup.GraphicsCommands, vpr_setup.RendererType, is_flat);
    if (vpr_setup.ShowGraphics || vpr_setup.SaveGraphics || !vpr_setup.GraphicsCommands.empty()) {
        alloc_draw_structs(&arch);
        init_draw_coords(vpr_setup.PlacerOpts.place_chan_width,
                         g_vpr_ctx.placement().blk_loc_registry());
    }
#else
    (void)vpr_setup;
    (void)arch;
#endif
}

APDrawManager::APDrawManager(const AtomNetlist& atom_netlist, const APNetlist& ap_netlist, const Prepacker& prepacker, const PartialPlacement& p_placement)
#ifndef NO_GRAPHICS
    : atom_block_ap_block_lookup_(atom_netlist, ap_netlist, prepacker)
#endif
{
#ifndef NO_GRAPHICS
    // Verify the atom block to ap block lookup after construction.
    unsigned num_errors = atom_block_ap_block_lookup_.verify(ap_netlist, prepacker);
    VTR_ASSERT(num_errors == 0);

    // Set references in draw_state to analytical placement variables that we will need for drawing.
    get_draw_state_vars()->set_ap_partial_placement_ptr(&p_placement);
    get_draw_state_vars()->set_atom_block_ap_block_lookup_ptr(&atom_block_ap_block_lookup_);

#else
    (void)atom_netlist;
    (void)prepacker;
    (void)ap_netlist;
    (void)p_placement;
#endif
}

APDrawManager::~APDrawManager() {
#ifndef NO_GRAPHICS
    // Clear references in draw_state to analytical placement variables that we no longer need for drawing.
    get_draw_state_vars()->clear_ap_partial_placement_ptr();
    get_draw_state_vars()->clear_atom_block_ap_block_lookup_ptr();
#endif
}

void APDrawManager::pause_at_initial_scene(const std::string& msg, PreClusterTimingManager& timing_manager) {
#ifndef NO_GRAPHICS
    if (timing_manager.is_valid()) {
        // Note: During each drawing stage (e.g. ANALYTICAL_PLACEMENT, ROUTING), the timing info pointer only needs to be
        // updated once. That is why we only pass the pointer once here, and pass in nullptr to update_screen()
        //in other APDrawManager functions.
        update_screen(ScreenUpdatePriority::MAJOR, msg.c_str(), e_pic_type::ANALYTICAL_PLACEMENT, timing_manager.get_timing_info_ptr());
    } else {
        // No timing info pointer is available when timing analysis is off.
        update_screen(ScreenUpdatePriority::MAJOR, msg.c_str(), e_pic_type::ANALYTICAL_PLACEMENT, nullptr);
    }
#else
    (void)msg;
    (void)timing_manager;
#endif
}

void APDrawManager::pause_at_final_scene(const std::string& msg) {
#ifndef NO_GRAPHICS
    update_screen(ScreenUpdatePriority::MAJOR, msg.c_str(), e_pic_type::ANALYTICAL_PLACEMENT, nullptr);
#else
    (void)msg;
#endif
}

void APDrawManager::update_graphics(unsigned int iteration, enum APDrawType draw_type) {
#ifndef NO_GRAPHICS
    std::string msg;
    if (draw_type == APDrawType::Solver) {
        msg = "Analytical Placement Solver - Iteration: " + std::to_string(iteration);
    } else if (draw_type == APDrawType::Legalizer) {
        msg = "Analytical Placement Legalizer - Iteration: " + std::to_string(iteration);
    } else {
        msg = "Analytical Placement";
    }
    update_screen(ScreenUpdatePriority::MINOR, msg.c_str(), e_pic_type::ANALYTICAL_PLACEMENT, nullptr);
#else
    (void)iteration;
    (void)draw_type;
#endif
}
