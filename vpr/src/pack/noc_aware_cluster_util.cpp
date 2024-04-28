
#include "noc_aware_cluster_util.h"
#include "globals.h"

#include <queue>

std::vector<AtomBlockId> find_noc_router_atoms() {
    const auto& atom_ctx = g_vpr_ctx.atom();

    // NoC router atoms are expected to have a specific blif model
    const std::string noc_router_blif_model_name = "noc_router_adapter_block";

    // stores found NoC router atoms
    std::vector<AtomBlockId> noc_router_atoms;

    // iterate over all atoms and find those whose blif model matches
    for (auto atom_id : atom_ctx.nlist.blocks()) {
        const t_model* model = atom_ctx.nlist.block_model(atom_id);
        if (noc_router_blif_model_name == model->name) {
            noc_router_atoms.push_back(atom_id);
        }
    }

    return noc_router_atoms;
}

void update_noc_reachability_partitions(const std::vector<AtomBlockId>& noc_atoms) {
    const auto& atom_ctx = g_vpr_ctx.atom();
    auto& constraints = g_vpr_ctx.mutable_floorplanning().constraints;
    const auto& high_fanout_thresholds = g_vpr_ctx.cl_helper().high_fanout_thresholds;
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& grid = device_ctx.grid;

    t_logical_block_type_ptr logic_block_type = infer_logic_block_type(grid);
    const size_t high_fanout_threshold = high_fanout_thresholds.get_threshold(logic_block_type->name);

    // get the total number of atoms
    const size_t n_atoms = atom_ctx.nlist.blocks().size();

    vtr::vector<AtomBlockId, bool> atom_visited(n_atoms, false);

    int exclusivity_cnt = 0;

    RegionRectCoord unconstrained_rect{0,
                                       0,
                                       std::numeric_limits<int>::max(),
                                       std::numeric_limits<int>::max(),
                                       0};
    Region unconstrained_region;
    unconstrained_region.set_region_rect(unconstrained_rect);

    for (auto noc_atom_id : noc_atoms) {
        // check if this NoC router has already been visited
        if (atom_visited[noc_atom_id]) {
            continue;
        }

        exclusivity_cnt++;

        PartitionRegion associated_noc_partition_region;
        associated_noc_partition_region.set_exclusivity_index(exclusivity_cnt);
        associated_noc_partition_region.add_to_part_region(unconstrained_region);

        Partition associated_noc_partition;
        associated_noc_partition.set_name(atom_ctx.nlist.block_name(noc_atom_id));
        associated_noc_partition.set_part_region(associated_noc_partition_region);
        auto associated_noc_partition_id = (PartitionId)constraints.get_num_partitions();
        constraints.add_partition(associated_noc_partition);

        const PartitionId noc_partition_id = constraints.get_atom_partition(noc_atom_id);

        if (noc_partition_id == PartitionId::INVALID()) {
            constraints.add_constrained_atom(noc_atom_id, associated_noc_partition_id);
        } else {    // noc atom is already in a partition
            auto& noc_partition = constraints.get_mutable_partition(noc_partition_id);
            auto& noc_partition_region = noc_partition.get_mutable_part_region();
            VTR_ASSERT(noc_partition_region.get_exclusivity_index() < 0);
            noc_partition_region.set_exclusivity_index(exclusivity_cnt);
        }

        std::queue<AtomBlockId> q;
        q.push(noc_atom_id);
        atom_visited[noc_atom_id] = true;

        while (!q.empty()) {
            AtomBlockId current_atom = q.front();
            q.pop();

            PartitionId atom_partition_id = constraints.get_atom_partition(current_atom);
            if (atom_partition_id == PartitionId::INVALID()) {
                constraints.add_constrained_atom(current_atom, associated_noc_partition_id);
            } else {
                auto& atom_partition = constraints.get_mutable_partition(atom_partition_id);
                auto& atom_partition_region = atom_partition.get_mutable_part_region();
                VTR_ASSERT(atom_partition_region.get_exclusivity_index() < 0 || current_atom == noc_atom_id);
                atom_partition_region.set_exclusivity_index(exclusivity_cnt);
            }

            for(auto pin : atom_ctx.nlist.block_pins(current_atom)) {
                AtomNetId net_id = atom_ctx.nlist.pin_net(pin);
                size_t net_fanout = atom_ctx.nlist.net_sinks(net_id).size();

                if (net_fanout >= high_fanout_threshold) {
                    continue;
                }

                AtomBlockId driver_atom_id = atom_ctx.nlist.net_driver_block(net_id);
                if (!atom_visited[driver_atom_id]) {
                    q.push(driver_atom_id);
                    atom_visited[driver_atom_id] = true;
                }

                for (auto sink_pin : atom_ctx.nlist.net_sinks(net_id)) {
                    AtomBlockId sink_atom_id = atom_ctx.nlist.pin_block(sink_pin);
                    if (!atom_visited[sink_atom_id]) {
                        q.push(sink_atom_id);
                        atom_visited[sink_atom_id] = true;
                    }
                }

            }
        }

    }
}