
#pragma once

#include <string>

class APNetlist;
class AtomNetlist;
class DeviceContext;
struct PartialPlacement;

void print_ap_netlist_stats(const APNetlist& netlist);

double get_hpwl(const PartialPlacement& placement, const APNetlist& netlist);

void unicode_art(const PartialPlacement& placement, const APNetlist& netlist, const DeviceContext& device_ctx);

bool export_to_flat_placement_file(const PartialPlacement& placement, const APNetlist& ap_netlist, const AtomNetlist& atom_netlist, const std::string& file_name);

