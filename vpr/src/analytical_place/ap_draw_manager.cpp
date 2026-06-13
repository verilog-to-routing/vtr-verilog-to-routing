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
#include "partial_placement.h"
#endif

void init_ap_graphics(const t_vpr_setup& vpr_setup, const t_arch& arch) {
#ifndef NO_GRAPHICS
    bool is_flat = vpr_setup.RouterOpts.flat_routing;
    init_graphics_state(vpr_setup.ShowGraphics, vpr_setup.GraphPause,
                        vpr_setup.RouterOpts.route_type, vpr_setup.SaveGraphics,
                        vpr_setup.GraphicsCommands, is_flat);
    if (vpr_setup.ShowGraphics || vpr_setup.SaveGraphics || !vpr_setup.GraphicsCommands.empty()) {
        alloc_draw_structs(&arch);
    }
#else
    (void)vpr_setup;
    (void)arch;
#endif
}

APDrawManager::APDrawManager(const PartialPlacement& p_placement) {
#ifndef NO_GRAPHICS
    // Set the analytical placement reference in draw state
    get_draw_state_vars()->set_ap_partial_placement_ref(p_placement);
#else
    (void)p_placement;
#endif
}

APDrawManager::~APDrawManager() {
#ifndef NO_GRAPHICS
    // Clear the analytical placement reference in draw state
    get_draw_state_vars()->clear_ap_partial_placement_ref();
#endif
}

void APDrawManager::pause(const std::string& msg) {
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
