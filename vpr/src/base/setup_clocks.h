#pragma once

#include <vector>

#include "physical_types.h"

/// @brief Builds the device's dedicated clock networks and connections (device_ctx.clock_networks
/// / device_ctx.clock_connections) from the architecture's <clocknetworks> description, and
/// appends each network's segment(s) to segment_inf. Safe to call more than once (e.g. after
/// the device grid is resized), since clock network geometry depends on grid width/height.
void setup_clock_networks(const t_arch& Arch, std::vector<t_segment_inf>& segment_inf);
