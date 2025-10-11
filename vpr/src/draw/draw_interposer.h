#pragma once

/**
 * @file draw_interposer.h
 * @brief Declares functions to draw interposer cut lines in the FPGA device.
 *
 * Cut lines are drawn at channel centers during placement and slightly offset
 * during routing for better channel ownership visualization.
 */

#ifndef NO_GRAPHICS

// forward declaration
namespace ezgl {
class renderer;
}

/// @brief Draws interposer cut lines.
/// During placement stage, the cut lines are drawn in the middle of routing channels.
/// During routing, the cut lines are drawn closer to the side of routing channel to make it clear
/// which side of the cut owns the channel.
void draw_interposer_cuts(ezgl::renderer* g);

#endif