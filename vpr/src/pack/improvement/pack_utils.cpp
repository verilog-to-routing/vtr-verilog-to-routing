//
// Created by elgammal on 2022-07-27.
//

#include "pack_utils.h"
#include "re_cluster.h"
#include "re_cluster_util.h"
#include "globals.h"
#include "clustered_netlist_fwd.h"
#include "move_utils.h"
#include "cluster_placement.h"
#include "packing_move_generator.h"
#include "pack_move_utils.h"
#include "string.h"
#include "vtr_time.h"

void init_clb_atoms_lookup(vtr::vector<ClusterBlockId, std::unordered_set<AtomBlockId>>& atoms_lookup){
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.clustering();

#ifdef pack_improve_debug
    vtr::ScopedFinishTimer lookup_timer ("Building CLB atoms lookup");
#endif

    atoms_lookup.resize(cluster_ctx.clb_nlist.blocks().size());

    for (auto atom_blk_id : atom_ctx.nlist.blocks()) {
        ClusterBlockId clb_index = atom_ctx.lookup.atom_clb(atom_blk_id);

        atoms_lookup[clb_index].insert(atom_blk_id);
    }
}

void iteratively_improve_packing(const t_packer_opts& packer_opts, t_clustering_data& clustering_data, int verbosity) {
    /*
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& atom_ctx = g_vpr_ctx.atom();
    */
    int proposed = 0;
    int succeeded = 0;
    std::unique_ptr<packingMoveGenerator> move_generator;

    auto& helper_ctx = g_vpr_ctx.mutable_cl_helper();
    init_clb_atoms_lookup(helper_ctx.atoms_lookup);


    if(strcmp(packer_opts.pack_move_type.c_str(), "randomSwap") == 0 )
        move_generator = std::make_unique<randomPackingSwap>();
    else if(strcmp(packer_opts.pack_move_type.c_str(), "semiDirectedSwap") == 0)
        move_generator = std::make_unique<quasiDirectedPackingSwap>();
    else if(strcmp(packer_opts.pack_move_type.c_str(), "semiDirectedSameTypeSwap") == 0)
        move_generator = std::make_unique<quasiDirectedSameTypePackingSwap>();
    else{
        VTR_LOG("Packing move type (%s) is not correct!\n", packer_opts.pack_move_type.c_str());
        VTR_LOG("Packing iterative improvement is aborted\n");
        return;
    }
    
    bool is_proposed, is_valid, is_successful;
    std::vector<molMoveDescription> new_locs;

#ifdef pack_improve_debug
    float propose_sec = 0;
    float evaluate_sec = 0;
    float apply_suc_sec = 0;
    float apply_fail_sec = 0;
#endif

    for(int i = 0; i < packer_opts.pack_num_moves; i++) {
        new_locs.clear();
#ifdef pack_improve_debug
        vtr::Timer propose_timer;
#endif
        is_proposed = move_generator->propose_move(new_locs);
#ifdef pack_improve_debug
        propose_sec += propose_timer.elapsed_sec();
#endif
        if (!is_proposed) {
            VTR_LOGV(verbosity > 2, "Move failed! Can't propose.\n");
            continue;
        }

#ifdef pack_improve_debug
        vtr::Timer evaluate_timer;
#endif
        is_valid = move_generator->evaluate_move(new_locs);
#ifdef pack_improve_debug
        evaluate_sec += evaluate_timer.elapsed_sec();
#endif
        if (!is_valid) {
            VTR_LOGV(verbosity > 2, "Move failed! Proposed move is bad.\n");
            continue;
        }


        proposed++;
#ifdef pack_improve_debug
        vtr::Timer apply_timer;
#endif
        is_successful = move_generator->apply_move(new_locs, clustering_data);
#ifdef pack_improve_debug
        if(is_successful)
            apply_suc_sec += apply_timer.elapsed_sec();
        else
            apply_fail_sec += apply_timer.elapsed_sec();
#endif
        if (!is_successful) {
            VTR_LOGV(verbosity > 2, "Move failed! Proposed move isn't legal.\n");
            continue;
        }
        succeeded++;
        VTR_LOGV(verbosity > 2, "Packing move succeeded!\n");
    }
    VTR_LOG("\n### Iterative packing stats: \n\tpack move type = %s\n\ttotal pack moves = %zu\n\tgood pack moves = %zu\n\tlegal pack moves = %zu\n\n", packer_opts.pack_move_type.c_str(), packer_opts.pack_num_moves, proposed, succeeded);
    VTR_LOG("#propose time = %6.2f\n", propose_sec);
    VTR_LOG("#evaluate time = %6.2f\n", evaluate_sec);
    VTR_LOG("#apply time = %6.2f\n", apply_suc_sec+apply_fail_sec);
    VTR_LOG("\tapply suc time = %6.2f\n", apply_suc_sec);
    VTR_LOG("\tapply fail time = %6.2f\n", apply_fail_sec);
}