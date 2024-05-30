
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
    auto& cl_helper_ctx = g_vpr_ctx.mutable_cl_helper();
    const auto& high_fanout_thresholds = g_vpr_ctx.cl_helper().high_fanout_thresholds;
    const auto& grid = g_vpr_ctx.device().grid;

    t_logical_block_type_ptr logic_block_type = infer_logic_block_type(grid);
    const char* logical_block_name = logic_block_type != nullptr ? logic_block_type->name : "";
    const size_t high_fanout_threshold = high_fanout_thresholds.get_threshold(logical_block_name);

    // get the total number of atoms
    const size_t n_atoms = atom_ctx.nlist.blocks().size();

    vtr::vector<AtomBlockId, bool> atom_visited(n_atoms, false);

    cl_helper_ctx.atom_noc_grp_id.resize(n_atoms, NocGroupId::INVALID());

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
        atom_visited[noc_atom_id] = true;

        while (!q.empty()) {
            AtomBlockId current_atom = q.front();
            q.pop();

            cl_helper_ctx.atom_noc_grp_id[current_atom] = noc_grp_id;

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