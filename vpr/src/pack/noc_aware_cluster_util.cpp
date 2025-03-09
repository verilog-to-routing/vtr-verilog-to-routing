
#include "noc_aware_cluster_util.h"
#include "atom_netlist.h"
#include "physical_types.h"
#include "physical_types_util.h"
#include "globals.h"
#include "vpr_types.h"

#include <queue>


const t_model* find_noc_router_model(const t_noc_inf& noc_info) {
    const std::string& noc_router_tile_name = noc_info.noc_router_tile_name;

    t_physical_tile_type_ptr noc_tile = find_tile_type_by_name(noc_router_tile_name,
                                                               g_vpr_ctx.device().physical_tile_types);
    VTR_ASSERT(noc_tile != nullptr);

    t_logical_block_type_ptr noc_logical_block = pick_logical_type(noc_tile);
    VTR_ASSERT(noc_logical_block != nullptr);
    VTR_ASSERT(noc_logical_block->pb_type != nullptr);

    return noc_logical_block->pb_type->modes->pb_type_children->model;
}

std::vector<AtomBlockId> find_noc_router_atoms(const AtomNetlist& atom_netlist,
                                               const t_noc_inf& noc_info) {
    // Find the blif model for NoC routers
    const t_model* noc_blif_model = find_noc_router_model(noc_info);
    std::string_view noc_router_blif_model_name = noc_blif_model->name;

    VTR_LOG("Detected NoC router blif model: %s\n", noc_blif_model->name);

    // stores found NoC router atoms
    std::vector<AtomBlockId> noc_router_atoms;

    // iterate over all atoms and find those whose blif model matches
    for (auto atom_id : atom_netlist.blocks()) {
        const t_model* model = atom_netlist.block_model(atom_id);
        if (noc_router_blif_model_name == model->name) {
            noc_router_atoms.push_back(atom_id);
        }
    }

    VTR_LOG("Found %i NoC router blocks in the atom netlist.\n", noc_router_atoms.size());

    return noc_router_atoms;
}

void update_noc_reachability_partitions(const std::vector<AtomBlockId>& noc_atoms,
                                        const AtomNetlist& atom_netlist,
                                        const t_pack_high_fanout_thresholds& high_fanout_thresholds,
                                        vtr::vector<AtomBlockId, NocGroupId>& atom_noc_grp_id) {
    const auto& grid = g_vpr_ctx.device().grid;

    t_logical_block_type_ptr logic_block_type = infer_logic_block_type(grid);
    const char* logical_block_name = logic_block_type != nullptr ? logic_block_type->name.c_str() : "";
    const size_t high_fanout_threshold = high_fanout_thresholds.get_threshold(logical_block_name);

    // get the total number of atoms
    const size_t n_atoms = atom_netlist.blocks().size();

    vtr::vector<AtomBlockId, bool> atom_visited(n_atoms, false);

    atom_noc_grp_id.resize(n_atoms, NocGroupId::INVALID());

    int noc_grp_id_cnt = 0;

    /*
     * Assume that the atom netlist is represented as an undirected graph
     * with all high fanout nets removed. In this graph, we want to find all
     * connected components that include at least one NoC router. We start a
     * BFS from each NoC router and traverse all nets below the high_fanout_threshold,
     * and mark each atom block with a NoC group ID.
     */

    for (auto noc_atom_id : noc_atoms) {
        // check if this NoC router has already been visited
        if (atom_visited[noc_atom_id]) {
            continue;
        }

        auto noc_grp_id = (NocGroupId)noc_grp_id_cnt;
        noc_grp_id_cnt++;

        std::queue<AtomBlockId> q;
        q.push(noc_atom_id);
        size_t n_atoms_in_group = 0;
        atom_visited[noc_atom_id] = true;

        while (!q.empty()) {
            AtomBlockId current_atom = q.front();
            q.pop();
            n_atoms_in_group++;

            atom_noc_grp_id[current_atom] = noc_grp_id;

            for(auto pin : atom_netlist.block_pins(current_atom)) {
                AtomNetId net_id = atom_netlist.pin_net(pin);
                size_t net_fanout = atom_netlist.net_sinks(net_id).size();

                if (net_fanout >= high_fanout_threshold) {
                    continue;
                }

                AtomBlockId driver_atom_id = atom_netlist.net_driver_block(net_id);
                if (!atom_visited[driver_atom_id]) {
                    q.push(driver_atom_id);
                    atom_visited[driver_atom_id] = true;
                }

                for (auto sink_pin : atom_netlist.net_sinks(net_id)) {
                    AtomBlockId sink_atom_id = atom_netlist.pin_block(sink_pin);
                    if (!atom_visited[sink_atom_id]) {
                        q.push(sink_atom_id);
                        atom_visited[sink_atom_id] = true;
                    }
                }

            }
        }

        VTR_LOG("NoC group %i contains %i atoms.\n", noc_grp_id, n_atoms_in_group);
    }
}
