#pragma once
/**
 * @file parallel_anneal_engine.h
 * @brief Declares ParallelAnnealEngine, which speculatively evaluates several
 * proposed swaps in parallel while keeping the annealing trajectory
 * deterministic.
 *
 * ## Algorithm
 *
 * Most swap attempts in simulated annealing are rejected, and a rejected attempt
 * leaves the placement state unchanged. The engine exploits this: it proposes a
 * *batch* of `W` swaps against the current committed placement state and
 * evaluates them concurrently on worker threads. The annealer sets W to the
 * inverse of the previous temperature's acceptance rate, so we expect
 * one of the W attempts to be accepted (W == the number of active workers,
 * capped by --place_swap_eval_num_workers). Each attempt has a logical id;
 * outcomes are resolved in id order:
 *
 *  - The winner is the lowest-id accepted attempt. It is committed, and every
 *    attempt with the same or lower id "really happened" (statistics and the RL
 *    agent are updated in id order).
 *  - Higher ids were evaluated against a state the winner just invalidated, so
 *    they never logically happened and are re-proposed in the next batch. Once
 *    an acceptance is known, still-running higher ids cancel early: they can
 *    neither win nor be bookkept.
 *  - If nothing accepts, all W attempts are real rejections.
 *
 * ## Determinism
 *
 * Each attempt id derives its own RNG stream from (seed, id), and replica agents
 * are synced from the master generator every batch. An attempt's proposal and
 * acceptance test therefore do not depend on which worker runs it or when. The
 * trajectory is a pure function of (seed, W), so the result is identical for any
 * worker count.
 *
 * ## Threading model
 *
 * Each worker owns a private replica of the state that swap evaluation mutates:
 * block locations, a NetCostHandler, the proposed connection delay/timing cost
 * arrays (inside the replica's PlacerState), plus a private RNG and private move
 * generators. Everything else (the clustered netlist, criticalities, the delay
 * model) is shared and read-only during a batch. Workers both propose and
 * evaluate their share of a batch against their own replica; the coordinator
 * only resolves outcomes and commits. When a winner is committed, the master
 * state and every replica apply the same move, keeping all copies bit-identical.
 *
 * Supported configurations: CRITICALITY_TIMING_PLACE and BOUNDING_BOX_PLACE
 * with cube bounding boxes, without congestion modeling, interposer cost terms,
 * NoC optimization, manual moves, or per-move logging. The annealer falls back
 * to the sequential inner loop otherwise (see PlacementAnnealer::should_use_parallel_inner_loop_()).
 */

#include "interposer_cost_handler.h"
#include "move_generator.h"
#include "move_transactions.h"
#include "placer_state.h"
#include "swap_evaluator.h"
#include "vtr_random.h"

#include <atomic>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

class NetPinTimingInvalidator;
class PlaceDelayModel;
class PlaceMacros;
class PlacerCriticalities;
class PlacerState;
class t_placer_costs;
struct t_placer_opts;

/**
 * @brief One speculative swap attempt. All fields are filled by the worker that
 * proposes and evaluates the attempt; the coordinator only reads them when
 * resolving and committing the batch.
 */
struct t_speculative_swap {
    /// The proposed move, captured only for accepted attempts.
    std::vector<t_pl_moved_block> moved_blocks;
    /// Move/block type chosen by the move generator.
    t_propose_action proposed_action;
    /// Whether the move generator produced a valid move.
    e_create_move create_outcome = e_create_move::ABORT;
    /// The RL-agent action behind this proposal.
    size_t agent_action = 0;
    /// Pre-drawn uniform random number for the acceptance test, from this attempt's RNG stream.
    float accept_rand = 0.f;
    /// Accept/reject decision made during evaluation (ABORTED when create_outcome != VALID).
    e_move_result move_result = e_move_result::ABORTED;
    /// Cost deltas computed during evaluation.
    t_swap_cost_deltas deltas;
    /// Replayable committed values, captured during evaluation for accepted
    /// attempts only. Lets the winner be committed everywhere with
    /// O(affected nets + pins) copies instead of a re-evaluation.
    t_swap_commit_record commit_record;
};

/// @brief Result of running one speculative batch.
struct t_batch_outcome {
    /// Number of attempts that logically happened (index of the winner + 1, or
    /// the full batch size if no attempt was accepted).
    int logical_attempts = 0;
    /// True when the last logical attempt was accepted and has been committed.
    bool committed = false;
};

/**
 * @class ParallelAnnealEngine
 * @brief Worker pool and batch scheduler for speculative parallel swap evaluation.
 */
class ParallelAnnealEngine {
  public:
    ParallelAnnealEngine() = delete;
    ParallelAnnealEngine(const ParallelAnnealEngine&) = delete;
    ParallelAnnealEngine& operator=(const ParallelAnnealEngine&) = delete;

    /**
     * @param num_workers Number of worker threads.
     * @param placer_opts Placement options (algorithm, seed, cost factors).
     * @param place_macros Placement macros, needed to construct the replicas' move generators.
     * @param costs Master placement costs; updated when a winner is committed.
     * @param master_state The master placement state.
     * @param master_net_cost_handler Net cost handler bound to the master state.
     * @param move_lim Number of moves per temperature, needed to construct the
     * replicas' move generators.
     * @param noc_centroid_weight NoC-biased centroid move weight, needed to
     * construct the replicas' move generators.
     * @param delay_model Placement delay model (nullptr for non-timing-driven placement).
     * @param criticalities Connection criticalities (nullptr for non-timing-driven placement).
     * @param pin_timing_invalidator Invalidator for incremental STA (nullptr for non-timing-driven placement).
     * @param master_evaluator Swap evaluator bound to the master state.
     * @param master_blocks_affected The annealer's scratch move record, used for
     * committing winners on the master state.
     */
    ParallelAnnealEngine(int num_workers,
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
                         t_pl_blocks_to_be_moved& master_blocks_affected);

    ~ParallelAnnealEngine();

    /**
     * @brief Brings every replica up to date with the master state. Must be
     * called before the first batch of every inner loop.
     *
     * The first call copies the full committed master state (block locations,
     * net cost data, timing data). Winner commits keep the replicas' committed
     * state bit-identical to the master's from then on, so subsequent calls only
     * copy the committed timing data — in the configurations the engine supports
     * (no congestion modeling, no interposer cost terms), that is the only state
     * the outer loop rewrites between inner loops
     * (perform_full_timing_update()).
     */
    void sync_replicas_for_inner_loop();

    /**
     * @brief Synchronizes only the committed timing data (connection delays and
     * timing costs) of every replica with the master state.
     *
     * Sufficient after a timing update or cost recompute inside the inner loop,
     * which rewrites timing data but leaves block locations and net bounding
     * boxes untouched.
     */
    void sync_replicas_timing();

    /**
     * @brief Proposes, evaluates, and resolves one speculative batch.
     *
     * Workers propose and evaluate their share of the batch on their private
     * replicas, with per-attempt RNG streams and replica move generators synced
     * from `master_move_generator`'s agent state. The lowest-id accepted attempt
     * (if any) is committed to the master state and all replicas before
     * returning.
     *
     * @param batch_size Number of attempts to issue (>= 1).
     * @param master_move_generator The master move generator selected by the
     * annealer for this inner loop. Never proposes here; it is the sync source
     * for the replica generators' agent state, and stays canonical because the
     * annealer replays every logical outcome to it (and only it).
     * @param use_second_generator True when `master_move_generator` is the
     * annealer's second move generator; replicas then propose with their second
     * generator as well.
     * @param temperature Current annealing temperature.
     * @param rlim Current move range limit.
     * @return How many attempts logically happened and whether a move was committed.
     */
    t_batch_outcome run_batch(int batch_size,
                              MoveGenerator& master_move_generator,
                              bool use_second_generator,
                              float temperature,
                              float rlim);

    /**
     * @brief Returns the attempts of the most recent batch. Only the logical
     * prefix reported by run_batch() is meaningful; later entries were discarded
     * as stale speculation.
     */
    const std::vector<t_speculative_swap>& attempts() const { return attempts_; }

  private:
    /// @brief Worker-private replica of the placement state mutated by swap
    /// evaluation, plus the RNG and move generators this worker proposes with.
    struct EvalReplica {
        EvalReplica(const t_placer_opts& placer_opts,
                    const PlaceMacros& place_macros,
                    const t_placer_costs& costs,
                    const PlacerState& master_state,
                    const NetCostHandler& master_net_cost_handler,
                    int move_lim,
                    float noc_centroid_weight,
                    const PlaceDelayModel* delay_model,
                    const PlacerCriticalities* criticalities);

        /// Private block locations + proposed timing arrays. grid_blocks and the
        /// committed timing arrays inside are kept in sync via winner commits.
        PlacerState placer_state;
        /// Private net cost scratch + committed net cost data.
        std::unique_ptr<NetCostHandler> net_cost_handler;
        /// Never engaged: the engine only runs when interposer cost terms are
        /// inactive. Exists because SwapEvaluator takes an optional handler by
        /// reference (the sequential path shares that interface).
        std::optional<InterposerCostHandler> interposer_cost_handler;
        /// Private move record scratch.
        t_pl_blocks_to_be_moved blocks_affected;
        /// Private RNG, reseeded from (seed, attempt id) for every attempt this
        /// worker proposes and evaluates.
        vtr::RngContainer rng;
        /// Private move generators mirroring the annealer's pair; their evolving
        /// agent state is synced from the master generator every batch.
        std::unique_ptr<MoveGenerator> move_generator_1;
        std::unique_ptr<MoveGenerator> move_generator_2;
        /// Evaluator bound to this replica.
        std::unique_ptr<SwapEvaluator> evaluator;
    };

    /// @brief Commands executed by the worker pool.
    enum class e_worker_job : uint8_t {
        NONE,
        EVALUATE,      ///< Evaluate this worker's share of the current batch
        COMMIT_WINNER, ///< Apply the winning move to this worker's replica
        SYNC_FULL,     ///< Synchronize this worker's replica with the full master state
        SYNC_TIMING,   ///< Synchronize this worker's replica timing data with the master state
        EXIT           ///< Terminate the worker thread
    };

    /// @brief Worker thread entry point: waits for jobs and executes them on its replica.
    void worker_main_(int worker_id);

    /// @brief Publishes a job to the workers (non-blocking).
    void begin_worker_job_(e_worker_job job);

    /// @brief Blocks until all workers have finished the current job.
    void wait_worker_job_();

    /// @brief Publishes a job and waits for its completion.
    void run_worker_job_(e_worker_job job);

    /// @brief Proposes and evaluates the attempt in slot `slot_index` of the
    /// current batch on `replica`, filling the attempt's fields. For accepted
    /// attempts, the move and its replayable commit record are captured before
    /// the speculative evaluation is reverted.
    void propose_and_evaluate_attempt_(EvalReplica& replica, MoveGenerator& move_generator, int slot_index);

    /// @brief Commits the winning attempt on `replica` by replaying its recorded
    /// committed values (no re-evaluation), keeping the replica in sync with the
    /// master state.
    void apply_winner_to_replica_(EvalReplica& replica, const t_speculative_swap& winner);

    /// @brief Commits the winning attempt on the master state by replaying its
    /// recorded committed values, and updates the placement costs from the
    /// recorded deltas.
    void commit_winner_on_master_(const t_speculative_swap& winner);

    void sync_replica_full_(EvalReplica& replica);
    void sync_replica_timing_(EvalReplica& replica);

    /**
     * @brief Derives the deterministic RNG seed of a swap attempt from the
     * placement seed and the attempt id (splitmix64 mix).
     */
    int attempt_seed_(uint64_t attempt_id) const;

    /**
     * @brief Acceptance test with a pre-drawn random number. Mirrors
     * PlacementAnnealer::assess_swap_(), but takes the uniform random number as
     * an argument so the decision is independent of evaluation order.
     */
    static e_move_result assess_speculative_swap_(double delta_c, float t, float accept_rand);

  private:
    const int num_workers_;
    const t_placer_opts& placer_opts_;
    /// Master placement costs; written only by commit_winner_on_master_().
    t_placer_costs& costs_;
    PlacerState& master_state_;
    NetCostHandler& master_net_cost_handler_;
    const PlaceDelayModel* delay_model_;
    const PlacerCriticalities* criticalities_;
    NetPinTimingInvalidator* pin_timing_invalidator_;
    SwapEvaluator& master_evaluator_;
    t_pl_blocks_to_be_moved& master_blocks_affected_;

    /// True for CRITICALITY_TIMING_PLACE (commit/revert proposed timing data).
    const bool timing_driven_;
    /// Base seed for per-attempt RNG stream derivation.
    const uint64_t seed_;
    /// Monotonic logical attempt counter; discarded speculation does not advance it.
    uint64_t attempt_counter_ = 0;
    /// True once the replicas have received their initial full state copy; from
    /// then on winner commits plus the per-inner-loop timing sync keep them
    /// identical to the master, and no further full copies are needed.
    bool replicas_fully_synced_ = false;

    /// Worker replicas, one per worker thread.
    std::vector<std::unique_ptr<EvalReplica>> replicas_;
    /// Attempts of the current/most recent batch.
    std::vector<t_speculative_swap> attempts_;

    // ---- Worker pool state ----
    std::vector<std::thread> workers_;
    /// Incremented to publish a new job; workers spin on it.
    std::atomic<uint64_t> job_epoch_{0};
    /// Count of workers that finished the current job.
    std::atomic<int> workers_done_{0};
    /// Current job; written by the coordinator before bumping job_epoch_.
    e_worker_job job_ = e_worker_job::NONE;
    /// EVALUATE payload: number of attempts in the current batch.
    int batch_size_ = 0;
    /// EVALUATE payload: current annealing temperature.
    float batch_temperature_ = 0.f;
    /// EVALUATE payload: current move range limit.
    float batch_rlim_ = 0.f;
    /// EVALUATE payload: whether replicas propose with their second generator.
    bool batch_use_gen2_ = false;
    /// EVALUATE payload: sync source for the replica generators' agent state.
    const MoveGenerator* batch_master_generator_ = nullptr;
    /// Lowest attempt id known to be accepted in the current batch (INT_MAX when
    /// none yet). Accepting workers CAS-min it; workers running attempts with a
    /// higher id cancel early, since those attempts can neither win nor be
    /// bookkept. It monotonically decreases to the batch winner's id, so a stale
    /// read only delays a cancellation — it can never cancel a logical attempt.
    std::atomic<int> first_accepted_id_{std::numeric_limits<int>::max()};
    /// COMMIT_WINNER payload: the winning attempt (with its commit record).
    const t_speculative_swap* winner_attempt_ = nullptr;
};
