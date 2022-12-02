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
//#include <mutex>
#include <thread>
void try_n_packing_moves(int thread_num, int n, const std::string& move_type, t_clustering_data& clustering_data, t_pack_iterative_stats& pack_stats);
void init_multithreading_locks();

void init_multithreading_locks() {
    auto& packing_multithreading_ctx = g_vpr_ctx.mutable_packing_multithreading();
    auto& helper_ctx = g_vpr_ctx.cl_helper();

    packing_multithreading_ctx.mu.resize(helper_ctx.total_clb_num);
    for(auto& m : packing_multithreading_ctx.mu) {
        m = new std::mutex;
    }
}

void init_clb_atoms_lookup(vtr::vector<ClusterBlockId, std::unordered_set<AtomBlockId>>& atoms_lookup) {
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.clustering();

#ifdef pack_improve_debug
    vtr::ScopedFinishTimer lookup_timer("Building CLB atoms lookup");
#endif

    atoms_lookup.resize(cluster_ctx.clb_nlist.blocks().size());

    for (auto atom_blk_id : atom_ctx.nlist.blocks()) {
        ClusterBlockId clb_index = atom_ctx.lookup.atom_clb(atom_blk_id);

        atoms_lookup[clb_index].insert(atom_blk_id);
    }
}

void iteratively_improve_packing(const t_packer_opts& packer_opts, t_clustering_data& clustering_data, int) {
    /*
     * auto& cluster_ctx = g_vpr_ctx.clustering();
     * auto& atom_ctx = g_vpr_ctx.atom();
     */
    t_pack_iterative_stats pack_stats;

    auto& helper_ctx = g_vpr_ctx.mutable_cl_helper();
    init_clb_atoms_lookup(helper_ctx.atoms_lookup);

#ifdef pack_improve_debug
    float propose_sec = 0;
    float evaluate_sec = 0;
    float apply_suc_sec = 0;
    float apply_fail_sec = 0;
#endif

    unsigned int total_num_moves = packer_opts.pack_num_moves;
    //unsigned int num_threads = std::thread::hardware_concurrency();
    const int num_threads = 1;
    unsigned int moves_per_thread = total_num_moves / num_threads;
    std::thread my_threads[num_threads];

    init_multithreading_locks();

    for (unsigned int i = 0; i < num_threads - 1; i++) {
        my_threads[i] = std::thread(try_n_packing_moves, i, moves_per_thread, packer_opts.pack_move_type, std::ref(clustering_data), std::ref(pack_stats));
    }
    my_threads[num_threads - 1] = std::thread(try_n_packing_moves, num_threads-1, total_num_moves - (moves_per_thread * (num_threads - 1)), packer_opts.pack_move_type, std::ref(clustering_data), std::ref(pack_stats));

    for (auto & my_thread : my_threads)
        my_thread.join();

    VTR_LOG("\n### Iterative packing stats: \n\tpack move type = %s\n\ttotal pack moves = %zu\n\tgood pack moves = %zu\n\tlegal pack moves = %zu\n\n",
            packer_opts.pack_move_type.c_str(),
            packer_opts.pack_num_moves,
            pack_stats.good_moves,
            pack_stats.legal_moves);
}

void try_n_packing_moves(int thread_num, int n, const std::string& move_type, t_clustering_data& clustering_data, t_pack_iterative_stats& pack_stats) {
    auto& packing_multithreading_ctx = g_vpr_ctx.mutable_packing_multithreading();

    bool is_proposed, is_valid, is_successful;
    std::vector<molMoveDescription> new_locs;
    int num_good_moves = 0;
    int num_legal_moves = 0;

    std::unique_ptr<packingMoveGenerator> move_generator;
    if (strcmp(move_type.c_str(), "randomSwap") == 0)
        move_generator = std::make_unique<randomPackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedSwap") == 0)
        move_generator = std::make_unique<quasiDirectedPackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedSameTypeSwap") == 0)
        move_generator = std::make_unique<quasiDirectedSameTypePackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedCompatibleTypeSwap") == 0)
        move_generator = std::make_unique<quasiDirectedCompatibleTypePackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedSameSizeSwap") == 0)
        move_generator = std::make_unique<quasiDirectedSameSizePackingSwap>();

    else {
        VTR_LOG("Packing move type (%s) is not correct!\n", move_type.c_str());
        VTR_LOG("Packing iterative improvement is aborted\n");
        return;
    }

    for (int i = 0; i < n; i++) {
        new_locs.clear();
        is_proposed = move_generator->propose_move(new_locs);
        if (!is_proposed)
            continue;

        is_valid = move_generator->evaluate_move(new_locs);
        if (!is_valid) {
            packing_multithreading_ctx.mu[new_locs[0].new_clb]->unlock();
            packing_multithreading_ctx.mu[new_locs[1].new_clb]->unlock();
            continue;
        } else
            num_good_moves++;

        is_successful = move_generator->apply_move(new_locs, clustering_data, thread_num);
        if (is_successful)
            num_legal_moves++;

        packing_multithreading_ctx.mu[new_locs[0].new_clb]->unlock();
        packing_multithreading_ctx.mu[new_locs[1].new_clb]->unlock();

    }

    pack_stats.mu.lock();
    pack_stats.good_moves += num_good_moves;
    pack_stats.legal_moves += num_legal_moves;
    pack_stats.mu.unlock();
}
