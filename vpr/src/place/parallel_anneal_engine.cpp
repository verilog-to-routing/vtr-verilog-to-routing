#include "parallel_anneal_engine.h"

#include "NetPinTimingInvalidator.h"
#include "RL_agent_util.h"
#include "globals.h"
#include "place_util.h"
#include "placer_state.h"
#include "vpr_types.h"
#include "vtr_assert.h"

#include <chrono>
#include <cmath>
#include <limits>

namespace {

/**
 * @brief Backoff step for busy-wait loops: spin briefly, then yield, then sleep.
 *
 * Batches are microsecond-scale, so workers usually pick up the next job while
 * still spinning; the sleep tier only engages during long serial phases (e.g.
 * timing updates in the outer loop) to avoid burning CPU.
 */
inline void spin_backoff(unsigned& spin_count) {
    ++spin_count;
    if (spin_count < 1024) {
        // busy spin
    } else if (spin_count < 65536) {
        std::this_thread::yield();
    } else {
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
}

} // namespace

ParallelAnnealEngine::EvalReplica::EvalReplica(const t_placer_opts& placer_opts,
                                               const PlaceMacros& place_macros,
                                               const t_placer_costs& costs,
                                               const PlacerState& master_state,
                                               const NetCostHandler& master_net_cost_handler,
                                               int move_lim,
                                               float noc_centroid_weight,
                                               const PlaceDelayModel* delay_model,
                                               const PlacerCriticalities* criticalities)
    : placer_state(placer_opts.place_algorithm.is_timing_driven())
    , blocks_affected(g_vpr_ctx.clustering().clb_nlist.blocks().size())
    , rng(placer_opts.seed) {
    // The move generator constructors below query the replica's block location
    // registry (e.g. movable block counts per type), so it must be populated
    // before they are created — the first full sync comes too late for that.
    placer_state.mutable_blk_loc_registry() = master_state.blk_loc_registry();

    net_cost_handler = std::make_unique<NetCostHandler>(placer_state,
                                                        master_net_cost_handler.cube_bb(),
                                                        placer_opts.place_algorithm,
                                                        master_net_cost_handler.congestion_chan_util_threshold());

    // The replica generators propose against the replica's state with the
    // replica's RNG. Their evolving agent state is re-synced from the master
    // generator before every batch (see worker_main_()), so the construction
    // arguments only need to match the master generators' configuration.
    std::tie(move_generator_1, move_generator_2) = create_move_generators(placer_state, place_macros,
                                                                          *net_cost_handler, placer_opts,
                                                                          move_lim, noc_centroid_weight, rng);

    evaluator = std::make_unique<SwapEvaluator>(placer_opts, costs, placer_state,
                                                *net_cost_handler, interposer_cost_handler,
                                                delay_model, criticalities);
}

ParallelAnnealEngine::ParallelAnnealEngine(int num_workers,
                                           const t_placer_opts& placer_opts,
                                           const PlaceMacros& place_macros,
                                           t_placer_costs& costs,
                                           PlacerState& master_state,
                                           NetCostHandler& master_net_cost_handler,
                                           int move_lim,
                                           float noc_centroid_weight,
                                           const PlaceDelayModel* delay_model,
                                           const PlacerCriticalities* criticalities,
                                           NetPinTimingInvalidator* pin_timing_invalidator,
                                           SwapEvaluator& master_evaluator,
                                           t_pl_blocks_to_be_moved& master_blocks_affected)
    : num_workers_(num_workers)
    , placer_opts_(placer_opts)
    , costs_(costs)
    , master_state_(master_state)
    , master_net_cost_handler_(master_net_cost_handler)
    , delay_model_(delay_model)
    , criticalities_(criticalities)
    , pin_timing_invalidator_(pin_timing_invalidator)
    , master_evaluator_(master_evaluator)
    , master_blocks_affected_(master_blocks_affected)
    , timing_driven_(placer_opts.place_algorithm.is_timing_driven())
    , seed_(static_cast<uint64_t>(static_cast<uint32_t>(placer_opts.seed))) {
    VTR_ASSERT(num_workers_ >= 1);
    VTR_ASSERT_MSG(placer_opts.place_algorithm == e_place_algorithm::CRITICALITY_TIMING_PLACE
                       || placer_opts.place_algorithm == e_place_algorithm::BOUNDING_BOX_PLACE,
                   "Parallel swap evaluation only supports the criticality-timing and bounding-box algorithms.");
    VTR_ASSERT_MSG(master_net_cost_handler.cube_bb(),
                   "Parallel swap evaluation only supports cube bounding boxes.");
    VTR_ASSERT_MSG(placer_opts.congestion_factor == 0.,
                   "Parallel swap evaluation does not support congestion modeling.");

    replicas_.reserve(num_workers_);
    for (int i = 0; i < num_workers_; ++i) {
        replicas_.emplace_back(std::make_unique<EvalReplica>(placer_opts_, place_macros, costs_,
                                                             master_state_, master_net_cost_handler_,
                                                             move_lim, noc_centroid_weight,
                                                             delay_model_, criticalities_));
    }

    // Matches the "no job in flight" state so waits before the first job
    // return immediately (see wait_worker_job_()).
    workers_done_.store(num_workers_ - 1, std::memory_order_relaxed);

    // The coordinator evaluates replica 0's share itself, so only
    // num_workers_ - 1 threads are needed.
    workers_.reserve(num_workers_ - 1);
    for (int i = 1; i < num_workers_; ++i) {
        workers_.emplace_back(&ParallelAnnealEngine::worker_main_, this, i);
    }
}

ParallelAnnealEngine::~ParallelAnnealEngine() {
    begin_worker_job_(e_worker_job::EXIT);
    for (std::thread& worker : workers_) {
        worker.join();
    }
}

void ParallelAnnealEngine::worker_main_(int worker_id) {
    uint64_t last_epoch = 0;

    while (true) {
        unsigned spin_count = 0;
        while (job_epoch_.load(std::memory_order_acquire) == last_epoch) {
            spin_backoff(spin_count);
        }
        // The coordinator never publishes a new job before all workers finished
        // the previous one, so exactly one new epoch can be pending here.
        last_epoch = job_epoch_.load(std::memory_order_acquire);

        if (job_ == e_worker_job::EXIT) {
            return;
        }

        execute_worker_job_(worker_id);

        workers_done_.fetch_add(1, std::memory_order_release);
    }
}

void ParallelAnnealEngine::execute_worker_job_(int worker_id) {
    EvalReplica& replica = *replicas_[worker_id];

    switch (job_) {
        case e_worker_job::EVALUATE: {
            MoveGenerator& generator = batch_use_gen2_ ? *replica.move_generator_2 : *replica.move_generator_1;
            // Copy the master generator's agent state; it is the only one
            // that receives outcomes. All workers read it, no one writes it.
            generator.sync_state_from(*batch_master_generator_);
            for (int k = worker_id; k < batch_size_; k += num_workers_) {
                propose_and_evaluate_attempt_(replica, generator, k);
            }
            break;
        }
        case e_worker_job::COMMIT_WINNER:
            apply_winner_to_replica_(replica, *winner_attempt_);
            break;
        case e_worker_job::SYNC_FULL:
            sync_replica_full_(replica);
            break;
        case e_worker_job::SYNC_TIMING:
            sync_replica_timing_(replica);
            break;
        default:
            break;
    }
}

void ParallelAnnealEngine::begin_worker_job_(e_worker_job job) {
    // The previous job may still be in flight: run_batch() leaves the replica
    // COMMIT_WINNER running so it overlaps the annealer's batch bookkeeping.
    wait_worker_job_();
    workers_done_.store(0, std::memory_order_relaxed);
    job_ = job;
    job_epoch_.fetch_add(1, std::memory_order_release);
}

void ParallelAnnealEngine::wait_worker_job_() {
    const int num_worker_threads = num_workers_ - 1;
    unsigned spin_count = 0;
    while (workers_done_.load(std::memory_order_acquire) != num_worker_threads) {
        spin_backoff(spin_count);
    }
}

void ParallelAnnealEngine::run_worker_job_(e_worker_job job) {
    begin_worker_job_(job);
    execute_worker_job_(/*worker_id=*/0);
    wait_worker_job_();
}

void ParallelAnnealEngine::sync_replicas() {
    if (!replicas_fully_synced_) {
        run_worker_job_(e_worker_job::SYNC_FULL);
        replicas_fully_synced_ = true;
    } else {
        // Winner commits keep the replicas' committed state identical to the
        // master's; in the configurations the engine supports, the outer loop
        // only rewrites timing data between inner loops.
        run_worker_job_(e_worker_job::SYNC_TIMING);
    }
}

void ParallelAnnealEngine::sync_replicas_timing() {
    run_worker_job_(e_worker_job::SYNC_TIMING);
}

void ParallelAnnealEngine::sync_replica_full_(EvalReplica& replica) {
    replica.placer_state.mutable_blk_loc_registry() = master_state_.blk_loc_registry();

    sync_replica_timing_(replica);

    replica.net_cost_handler->copy_committed_state_from(master_net_cost_handler_);
}

void ParallelAnnealEngine::sync_replica_timing_(EvalReplica& replica) {
    if (!timing_driven_) {
        return;
    }

    PlacerTimingContext& replica_timing = replica.placer_state.mutable_timing();
    const PlacerTimingContext& master_timing = master_state_.timing();

    replica_timing.connection_delay = master_timing.connection_delay;
    replica_timing.connection_timing_cost = master_timing.connection_timing_cost;
    replica_timing.net_timing_cost = master_timing.net_timing_cost;
}

int ParallelAnnealEngine::attempt_seed_(uint64_t attempt_id) const {
    // splitmix64-style mix of (seed, attempt id) into an independent stream seed.
    uint64_t z = seed_ + 0x9E3779B97F4A7C15ull * (attempt_id + 1);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    z ^= (z >> 31);
    return static_cast<int>(z & 0x7FFFFFFFull);
}

e_move_result ParallelAnnealEngine::assess_speculative_swap_(double delta_c, float t, float accept_rand) {
    if (delta_c <= 0) {
        return e_move_result::ACCEPTED;
    }

    if (t == 0.) {
        return e_move_result::REJECTED;
    }

    float prob_fac = std::exp(-delta_c / t);
    if (prob_fac > accept_rand) {
        return e_move_result::ACCEPTED;
    }
    return e_move_result::REJECTED;
}

void ParallelAnnealEngine::propose_and_evaluate_attempt_(EvalReplica& replica,
                                                         MoveGenerator& move_generator,
                                                         int slot_index) {
    t_speculative_swap& attempt = attempts_[slot_index];

    // Reset by the evaluating worker, not the coordinator, so the slot's cache
    // lines are not bounced through the coordinator every batch.
    attempt.reset();

    // Early cancellation: once a lower-id attempt is known to be accepted, this
    // one can neither win nor retire, so its remaining work is skipped. Nothing at
    // or below the eventual winner id is cancelled, so the trajectory is unaffected.
    const t_swap_cancel_token cancel_token{&first_accepted_id_, slot_index};

    if (cancel_token.cancelled()) {
        return;
    }

    // Reseed this attempt's private RNG stream, so its proposal and pre-drawn
    // acceptance number depend only on the attempt id and the committed state.
    replica.rng.srandom(attempt_seed_(attempt_counter_ + slot_index));

    // Allow some fraction of moves to not be restricted by rlim,
    // in the hopes of better escaping local minima.
    float attempt_rlim = batch_rlim_;
    if (placer_opts_.rlim_escape_fraction > 0. && replica.rng.frand() < placer_opts_.rlim_escape_fraction) {
        attempt_rlim = std::numeric_limits<float>::infinity();
    }

    attempt.create_outcome = move_generator.propose_move(replica.blocks_affected, attempt.proposed_action,
                                                         attempt_rlim, placer_opts_, criticalities_);
    const float accept_rand = replica.rng.frand();
    attempt.agent_action = move_generator.get_last_action();

    if (attempt.create_outcome != e_create_move::VALID) {
        attempt.move_result = e_move_result::ABORTED;
        replica.blocks_affected.clear_move_blocks();
        return;
    }

    // The cancellation token is also polled inside the evaluation, so a long
    // evaluation of a stale attempt is abandoned partway.
    attempt.deltas = replica.evaluator->apply_and_evaluate(replica.blocks_affected, placer_opts_.place_algorithm,
                                                           cancel_token);
    if (attempt.deltas.cancelled) {
        // The blocks have already been moved and some affected nets already
        // have proposed costs, so both must be undone.
        replica.evaluator->revert(replica.blocks_affected, timing_driven_);
        replica.blocks_affected.clear_move_blocks();
        return;
    }

    attempt.move_result = assess_speculative_swap_(attempt.deltas.delta_c, batch_temperature_, accept_rand);

    // Capture the move and the committed values it would write while they are
    // still in this replica's scratch, so a win can be committed everywhere
    // without re-evaluating, then publish this id for early cancellation.
    // Nothing is captured once a lower id is accepted, which never happens to
    // the eventual winner: it is the minimum accepted id.
    if (attempt.move_result == e_move_result::ACCEPTED && !cancel_token.cancelled()) {
        VTR_ASSERT_SAFE(!attempt.deltas.update_interposer_costs);
        attempt.moved_blocks = replica.blocks_affected.moved_blocks;
        replica.evaluator->extract_commit_record(replica.blocks_affected,
                                                 attempt.commit_record);

        // Atomic min: lower first_accepted_id_ to this id, retrying if another
        // worker changed it concurrently and stopping if it installed a lower id.
        int current = first_accepted_id_.load(std::memory_order_relaxed);
        while (slot_index < current
               && !first_accepted_id_.compare_exchange_weak(current, slot_index, std::memory_order_relaxed)) {
        }
    }

    // Evaluation is always speculative: leave the replica untouched. The winner
    // is committed through its recorded values after the batch resolves.
    replica.evaluator->revert(replica.blocks_affected, timing_driven_);
    replica.blocks_affected.clear_move_blocks();
}

void ParallelAnnealEngine::apply_winner_to_replica_(EvalReplica& replica, const t_speculative_swap& winner) {
    replica.blocks_affected.set_moved_blocks(winner.moved_blocks);
    replica.evaluator->apply_commit_record(replica.blocks_affected, winner.commit_record);
}

void ParallelAnnealEngine::commit_winner_on_master_(const t_speculative_swap& winner) {
    // The winner has the minimum accepted id, so its move was always captured.
    VTR_ASSERT_SAFE(!winner.moved_blocks.empty());

    VTR_ASSERT_SAFE(!winner.deltas.update_interposer_costs);
    costs_.cost += winner.deltas.delta_c;
    costs_.bb_cost += winner.deltas.cost_terms_delta.bb_cost;

    master_blocks_affected_.set_moved_blocks(winner.moved_blocks);

    if (timing_driven_) {
        costs_.timing_cost += winner.deltas.timing_delta_c;

        // The invalidator reads affected_pins, which only the winner's replica
        // computed, so copy them to the master before invalidating.
        master_blocks_affected_.affected_pins = winner.commit_record.affected_pins;
        pin_timing_invalidator_->invalidate_affected_connections(master_blocks_affected_);
    }

    master_evaluator_.apply_commit_record(master_blocks_affected_, winner.commit_record);
}

t_batch_outcome ParallelAnnealEngine::run_batch(int batch_size,
                                                MoveGenerator& master_move_generator,
                                                bool use_second_generator,
                                                float temperature,
                                                float rlim) {
    VTR_ASSERT_SAFE(batch_size >= 1);

    // The previous batch's replica commits may still be in flight, and their
    // winner lives in attempts_: wait before the resize below can reallocate it.
    // The slots themselves are reset by the workers that evaluate them (see
    // propose_and_evaluate_attempt_()).
    wait_worker_job_();

    if (attempts_.size() < static_cast<size_t>(batch_size)) {
        attempts_.resize(batch_size);
    }

    // --- Parallel propose + evaluate phase ---
    //
    // Each worker proposes and evaluates its share of the batch on its private
    // replica; the coordinator evaluates replica 0's share itself.
    first_accepted_id_.store(std::numeric_limits<int>::max(), std::memory_order_relaxed);
    batch_size_ = batch_size;
    batch_temperature_ = temperature;
    batch_rlim_ = rlim;
    batch_use_gen2_ = use_second_generator;
    batch_master_generator_ = &master_move_generator;
    run_worker_job_(e_worker_job::EVALUATE);

    // --- Resolve in attempt-id order ---
    t_batch_outcome outcome;
    outcome.num_retired_attempts = batch_size;

    int winner = -1;
    for (int k = 0; k < batch_size; ++k) {
        if (attempts_[k].create_outcome == e_create_move::VALID
            && attempts_[k].move_result == e_move_result::ACCEPTED) {
            winner = k;
            outcome.num_retired_attempts = k + 1;
            outcome.committed = true;
            break;
        }
    }

    // --- Commit the winner (replicas in parallel with the master) ---
    if (winner >= 0) {
        winner_attempt_ = &attempts_[winner];
        begin_worker_job_(e_worker_job::COMMIT_WINNER);
        commit_winner_on_master_(attempts_[winner]);
        execute_worker_job_(/*worker_id=*/0);
        // The worker threads' commits are deliberately not waited on: they
        // overlap the annealer's bookkeeping for this batch, and the next
        // job waits for them before touching any replica or attempt state.
    }

    attempt_counter_ += outcome.num_retired_attempts;
    return outcome;
}
