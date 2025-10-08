#pragma once

// forward declaration
namespace ezgl {
    class renderer;
}

/// @brief Draws interposer cut lines.
/// During placement stage, the cut lines are drawn in the middle of routing channels.
/// During routing, the cut lines are drawn closer to the side of routing channel to make it clear
/// which side of the cut owns the channel.
void draw_interposer_cuts(ezgl::renderer* g);