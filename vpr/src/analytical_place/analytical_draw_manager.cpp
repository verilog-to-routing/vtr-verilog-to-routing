/**
 * @file
 * @author  Yulang (Robert) Luo
 * @date    October 2025
 * @brief   The definitions of the Analytical Draw Manager class which is used
 *          to handle graphics updates during analytical placement.
 */

#include "analytical_draw_manager.h"
#include "vpr_types.h"

#ifndef NO_GRAPHICS
#include "draw.h"
#include "draw_global.h"
#include "partial_placement.h"
#endif

AnalyticalDrawManager::AnalyticalDrawManager(const PartialPlacement& p_placement) {
#ifndef NO_GRAPHICS
    // Set the analytical placement reference in draw state
    get_draw_state_vars()->set_ap_partial_placement_ref(p_placement);
#else
    (void)p_placement;
#endif
}

AnalyticalDrawManager::~AnalyticalDrawManager() {
#ifndef NO_GRAPHICS
    // Clear the analytical placement reference in draw state
    get_draw_state_vars()->clear_ap_partial_placement_ref();
#endif
}

void AnalyticalDrawManager::update_graphics(const std::string& msg) {
#ifndef NO_GRAPHICS
    update_screen(ScreenUpdatePriority::MAJOR, msg.c_str(), ANALYTICAL_PLACEMENT, nullptr);
#else
    (void)msg;
#endif
}