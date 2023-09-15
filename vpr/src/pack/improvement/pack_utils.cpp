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
void printProgressBar(double progress);
void try_n_packing_moves(int thread_num, int n, const std::string& move_type, t_clustering_data& clustering_data, t_pack_iterative_stats& pack_stats);

#ifdef PACK_MULTITHREADED
void init_multithreading_locks();
void free_multithreading_locks();
#endif

#ifdef PACK_MULTITHREADED
void init_multithreading_locks() {
    auto& packing_multithreading_ctx = g_vpr_ctx.mutable_packing_multithreading();
    auto& helper_ctx = g_vpr_ctx.cl_helper();

    packing_multithreading_ctx.mu.resize(helper_ctx.total_clb_num);
    for (auto& m : packing_multithreading_ctx.mu) {
        m = new std::mutex;
    }
}
#endif

#ifdef PACK_MULTITHREADED
void free_multithreading_locks() {
    auto& packing_multithreading_ctx = g_vpr_ctx.mutable_packing_multithreading();
    for (auto& m : packing_multithreading_ctx.mu) {
        delete m;
    }
}
#endif

void iteratively_improve_packing(const t_packer_opts& packer_opts, t_clustering_data& clustering_data, int) {
    /*
     * auto& cluster_ctx = g_vpr_ctx.clustering();
     * auto& atom_ctx = g_vpr_ctx.atom();
     */
    t_pack_iterative_stats pack_stats;

    auto& helper_ctx = g_vpr_ctx.mutable_cl_helper();
    init_clb_atoms_lookup(helper_ctx.atoms_lookup);
    //init_clb_clb_conn_numbers(helper_ctx.clb_conn_counts);
    //print_block_connections(helper_ctx.clb_conn_counts);
#ifdef pack_improve_debug
    float propose_sec = 0;
    float evaluate_sec = 0;
    float apply_suc_sec = 0;
    float apply_fail_sec = 0;
#endif

    unsigned int total_num_moves = packer_opts.pack_num_moves;
    const int num_threads = packer_opts.pack_num_threads;
    unsigned int moves_per_thread = total_num_moves / num_threads;
    std::thread* my_threads = new std::thread[num_threads];
#ifdef PACK_MULTITHREADED
    init_multithreading_locks();
#endif

    for (int i = 0; i < (num_threads - 1); i++) {
        my_threads[i] = std::thread(try_n_packing_moves, i, moves_per_thread, packer_opts.pack_move_type, std::ref(clustering_data), std::ref(pack_stats));
    }
    my_threads[num_threads - 1] = std::thread(try_n_packing_moves, num_threads - 1, total_num_moves - (moves_per_thread * (num_threads - 1)), packer_opts.pack_move_type, std::ref(clustering_data), std::ref(pack_stats));

    for (int i = 0; i < num_threads; i++)
        my_threads[i].join();

    VTR_LOG("\n### Iterative packing stats: \n\tpack move type = %s\n\ttotal pack moves = %zu\n\tgood pack moves = %zu\n\tlegal pack moves = %zu\n\n",
            packer_opts.pack_move_type.c_str(),
            packer_opts.pack_num_moves,
            pack_stats.good_moves,
            pack_stats.legal_moves);

    delete[] my_threads;
#ifdef PACK_MULTITHREADED
    free_multithreading_locks();
#endif
}

void try_n_packing_moves(int thread_num, int n, const std::string& move_type, t_clustering_data& clustering_data, t_pack_iterative_stats& pack_stats) {
#ifdef PACK_MULTITHREADED
    auto& packing_multithreading_ctx = g_vpr_ctx.mutable_packing_multithreading();
#endif


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
    else if (strcmp(move_type.c_str(), "randomConnSwap") == 0)
        move_generator = std::make_unique<randomConnPackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedConnSwap") == 0)
        move_generator = std::make_unique<quasiDirectedConnPackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedSameTypeConnSwap") == 0)
        move_generator = std::make_unique<quasiDirectedSameTypeConnPackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedCompatibleTypeConnSwap") == 0)
        move_generator = std::make_unique<quasiDirectedCompatibleTypeConnPackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedSameSizeConnSwap") == 0)
        move_generator = std::make_unique<quasiDirectedSameSizeConnPackingSwap>();
    else if (strcmp(move_type.c_str(), "randomTerminalSwap") == 0)
        move_generator = std::make_unique<randomTerminalPackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedTerminalSwap") == 0)
        move_generator = std::make_unique<quasiDirectedTerminalPackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedSameTypeTerminalSwap") == 0)
        move_generator = std::make_unique<quasiDirectedSameTypeTerminalPackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedCompatibleTypeTerminalSwap") == 0)
        move_generator = std::make_unique<quasiDirectedCompatibleTypeTerminalPackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedSameSizeTerminalSwap") == 0)
        move_generator = std::make_unique<quasiDirectedSameSizeTerminalPackingSwap>();
    else if (strcmp(move_type.c_str(), "randomTerminalNetSwap") == 0)
        move_generator = std::make_unique<randomTerminalNetPackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedTerminalNetSwap") == 0)
        move_generator = std::make_unique<quasiDirectedTerminalNetPackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedSameTypeTerminalNetSwap") == 0)
        move_generator = std::make_unique<quasiDirectedSameTypeTerminalNetPackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedCompatibleTypeTerminalNetSwap") == 0)
        move_generator = std::make_unique<quasiDirectedCompatibleTypeTerminalNetPackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedSameSizeTerminalNetSwap") == 0)
        move_generator = std::make_unique<quasiDirectedSameSizeTerminalNetPackingSwap>();
    else if (strcmp(move_type.c_str(), "randomTerminalNetNewFormulaSwap") == 0)
        move_generator = std::make_unique<randomTerminalNetNewFormulaPackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedTerminalNetNewFormulaSwap") == 0)
        move_generator = std::make_unique<quasiDirectedTerminalNetNewFormulaPackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedSameTypeTerminalNetNewFormulaSwap") == 0)
        move_generator = std::make_unique<quasiDirectedSameTypeTerminalNetNewFormulaPackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedCompatibleTypeTerminalNetNewFormulaSwap") == 0)
        move_generator = std::make_unique<quasiDirectedCompatibleTypeTerminalNetNewFormulaPackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedSameSizeTerminalNetNewFormulaSwap") == 0)
        move_generator = std::make_unique<quasiDirectedSameSizeTerminalNetNewFormulaPackingSwap>();
    else if (strcmp(move_type.c_str(), "randomTerminalOutsideSwap") == 0)
        move_generator = std::make_unique<randomTerminalOutsidePackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedTerminalOutsideSwap") == 0)
        move_generator = std::make_unique<quasiDirectedTerminalOutsidePackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedSameTypeTerminalOutsideSwap") == 0)
        move_generator = std::make_unique<quasiDirectedSameTypeTerminalOutsidePackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedCompatibleTypeTerminalOutsideSwap") == 0)
        move_generator = std::make_unique<quasiDirectedCompatibleTypeTerminalOutsidePackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedSameSizeTerminalOutsideSwap") == 0)
        move_generator = std::make_unique<quasiDirectedSameSizeTerminalOutsidePackingSwap>();

    else if (strcmp(move_type.c_str(), "randomCostEvaluationSwap") == 0)
        move_generator = std::make_unique<randomCostEvaluationPackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedCostEvaluationSwap") == 0)
        move_generator = std::make_unique<quasiDirectedCostEvaluationPackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedSameTypeCostEvaluationSwap") == 0)
        move_generator = std::make_unique<quasiDirectedSameTypeCostEvaluationPackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedCompatibleTypeCostEvaluationSwap") == 0)
        move_generator = std::make_unique<quasiDirectedCompatibleTypeCostEvaluationPackingSwap>();
    else if (strcmp(move_type.c_str(), "semiDirectedSameSizeCostEvaluationSwap") == 0)
        move_generator = std::make_unique<quasiDirectedSameSizeCostEvaluationPackingSwap>();

    else {
        VTR_LOG("Packing move type (%s) is not correct!\n", move_type.c_str());
        VTR_LOG("Packing iterative improvement is aborted\n");
        return;
    }

    for (int i = 0; i < n; i++) {
        if (thread_num == 0 && (i * 10) % n == 0) {
            printProgressBar(double(i) / n);
        }
        new_locs.clear();
        is_proposed = move_generator->propose_move(new_locs);
        if (!is_proposed) {
            continue;
        }
        is_valid = move_generator->evaluate_move(new_locs);
        if (!is_valid) {
#ifdef PACK_MULTITHREADED
            packing_multithreading_ctx.mu[new_locs[0].new_clb]->unlock();
            packing_multithreading_ctx.mu[new_locs[1].new_clb]->unlock();
#endif
            continue;
        } else {
            num_good_moves++;
        }

        is_successful = move_generator->apply_move(new_locs, clustering_data, thread_num);
        if (is_successful)
            num_legal_moves++;
#ifdef PACK_MULTITHREADED
        packing_multithreading_ctx.mu[new_locs[0].new_clb]->unlock();
        packing_multithreading_ctx.mu[new_locs[1].new_clb]->unlock();
#endif
    }

    pack_stats.mu.lock();
    pack_stats.good_moves += num_good_moves;
    pack_stats.legal_moves += num_legal_moves;
    pack_stats.mu.unlock();
}

#include <iostream>
#include <string>

void printProgressBar(double progress) {
    int barWidth = 70;

    VTR_LOG("[");
    int pos = barWidth * progress;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos)
            VTR_LOG("=");
        else if (i == pos)
            VTR_LOG(">");
        else
            VTR_LOG(" ");
    }
    VTR_LOG("] %zu %\n", int(progress * 100.0));
}
