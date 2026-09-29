#pragma once
/**
 * @file parallel_anneal_engine.h
 * @brief Declares ParallelAnnealEngine, which evaluates several proposed swaps
 * in parallel and applies at most one of them.
 *
 * ## Algorithm
 *
 * Most swap attempts in simulated annealing are rejected, especially when it
 * starts from a good initial placement (e.g. an analytical placement), and a
 * rejected attempt leaves the placement state unchanged. The engine proposes a
 * batch of `W` swaps against the current committed placement state and
 * evaluates them concurrently. Each attempt has an id. Outcomes
 * are resolved in id order:
 *
 *  - The lowest-id accepted attempt wins and is committed. It and every
 *    lower-id attempt retire, so statistics and the RL agent are updated
 *    for them in id order.
 *  - Higher ids were evaluated against a state the winner invalidated. They
 *    are discarded and their ids reissued in the next batch. Still-running
 *    higher ids cancel early once an acceptance is known.
 *  - If nothing accepts, all W attempts retire as rejections.
 *
 * ## Determinism
 *
 * Each attempt seeds its RNG from (seed, id), and replica RL-agents are synced
 * from the master generator every batch. An attempt's outcome therefore does
 * not depend on which worker runs it. Runs with the same seed and worker count
 * produce identical results.
 *
 * ## Threading model
 *
 * Each worker owns a private replica of the state that swap evaluation mutates
 * (block locations, net costs, proposed timing data), plus a private RNG and
 * move generators. Everything else is shared and read-only during a batch.
 * Workers propose and evaluate their share of a batch on their own replica.
 * The coordinator is worker 0 and also resolves outcomes and commits. A winner
 * is applied to the master state and every replica, keeping all copies identical.
 *
 * Supported configurations: CRITICALITY_TIMING_PLACE and BOUNDING_BOX_PLACE
 * with cube bounding boxes, without congestion modeling, interposer cost terms,
 * NoC optimization, manual moves, or per-move logging. This class does not
 * check these conditions. PlacementAnnealer only creates the engine when they
 * are satisfied, and runs its sequential inner loop otherwise.
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
 *
 * @note Adjacent slots of a batch are written concurrently by different workers.
 * The cache-line alignment keeps them from false-sharing a line at slot boundaries.
 */
struct alignas(64) t_speculative_swap {
    /// Moved blocks and their from/to locations, captured only for accepted attempts.
    std::vector<t_pl_moved_block> moved_blocks;
    /// Move/block type chosen by the move generator.
    t_propose_action proposed_action;
    /// Whether the move generator produced a valid move.
    e_create_move create_outcome = e_create_move::ABORT;
    /// The RL-agent action behind this proposal.
    size_t agent_action = 0;
    /// Accept/reject decision made during evaluation (ABORTED when create_outcome != VALID).
    e_move_result move_result = e_move_result::ABORTED;
    /// Cost deltas computed during evaluation.
    t_swap_cost_deltas deltas;
    /// Committed values to replay on the master and replicas without
    /// re-evaluation, captured only for accepted attempts.
    t_swap_commit_record commit_record;

    /// @brief Resets the attempt for reuse. Keeps the buffers of moved_blocks and
    /// commit_record.
    void reset() {
        moved_blocks.clear();
        proposed_action = t_propose_action();
        create_outcome = e_create_move::ABORT;
        agent_action = 0;
        move_result = e_move_result::ABORTED;
        deltas = t_swap_cost_deltas();
    }
};

/// @brief Result of running one speculative batch.
struct t_batch_outcome {
    /// Number of attempts that retired (index of the winner + 1, or the full
    /// batch size if no attempt was accepted).
    int num_retired_attempts = 0;
    /// True when the last retired attempt was accepted and has been committed.
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
     * @param num_workers Number of parallel evaluators. The coordinator thread
     * is one of them, so num_workers - 1 worker threads are spawned.
     * @param placer_opts Placement options (algorithm, seed, cost factors).
     * @param place_macros Placement macros, needed to construct the replicas' move generators.
     * @param costs Master placement costs, updated when a winner is committed.
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
     * @brief Copies the master state into every replica. Must be called
     * before the first batch of each outer loop iteration.
     *
     * @note Only the very first call copies the full master state. Winning
     * moves keep the replicas identical to the master, so later calls only
     * copy the timing data the outer loop rewrites.
     */
    void sync_replicas();

    /**
     * @brief Copies the master's committed connection delays and timing costs
     * into every replica.
     *
     * Needed after a timing update or a cost recompute, which rewrite that data
     * on the master.
     */
    void sync_replicas_timing();

    /**
     * @brief Proposes, evaluates, and resolves one speculative batch.
     *
     * Workers propose and evaluate their share of the batch on their private
     * replicas. The lowest-id accepted attempt, if any, is committed to the
     * master state before returning. Replica commits may still be running
     * when this returns and are waited on before the engine touches any
     * replica or attempt state again.
     *
     * @param batch_size Number of attempts to issue (>= 1).
     * @param master_move_generator The annealer's move generator for this inner
     * loop. It never proposes here and only serves as the sync source for the
     * replica generators' agent state.
     * @param use_second_generator True when `master_move_generator` is the
     * annealer's second move generator, so replicas propose with theirs too.
     * @param temperature Current annealing temperature.
     * @param rlim Current move range limit.
     * @return How many attempts retired and whether a move was committed.
     */
    t_batch_outcome run_batch(int batch_size,
                              MoveGenerator& master_move_generator,
                              bool use_second_generator,
                              float temperature,
                              float rlim);

    /**
     * @brief Returns the attempts of the most recent batch. Only the retired
     * prefix reported by run_batch() is meaningful. Later entries were discarded
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

        /// Private copy of block locations and timing data.
        PlacerState placer_state;
        /// Private copy of net bounding boxes and costs.
        std::unique_ptr<NetCostHandler> net_cost_handler;
        /// Always empty, since the engine only runs without interposer cost
        /// terms. Exists only because SwapEvaluator takes one by reference.
        std::optional<InterposerCostHandler> interposer_cost_handler;
        /// Private scratch for the blocks and pins touched by the attempt being evaluated.
        t_pl_blocks_to_be_moved blocks_affected;
        /// Private RNG, reseeded from (seed, attempt id) for every attempt this
        /// worker proposes and evaluates.
        vtr::RngContainer rng;
        /// Private copies of the annealer's two move generators. Their RL agent
        /// state is synced from the master generator every batch.
        std::unique_ptr<MoveGenerator> move_generator_1;
        std::unique_ptr<MoveGenerator> move_generator_2;
        /// Swap evaluator that operates on this replica's state.
        std::unique_ptr<SwapEvaluator> evaluator;
    };

    // ---- Worker pool ----
    //
    // The constructor spawns num_workers_ - 1 worker threads, one per replica
    // other than replica 0, which belongs to the coordinator (the annealer's
    // thread). The threads live for the lifetime of the engine and are joined
    // by the destructor after an EXIT job.
    //
    // Jobs are dispatched one at a time. The coordinator writes the job and its
    // payload, then bumps job_epoch_. Worker threads spin on job_epoch_, run the
    // job on their own replica, and increment workers_done_. The coordinator
    // runs its own share on replica 0 and then waits for workers_done_ to reach
    // the worker thread count before publishing the next job. Spinning backs
    // off to yield and then sleep during long serial phases.

    /// @brief Commands executed by the worker pool.
    enum class e_worker_job : uint8_t {
        NONE,
        EVALUATE,      ///< Evaluate this worker's share of the current batch
        COMMIT_WINNER, ///< Apply the winning move to this worker's replica
        SYNC_FULL,     ///< Synchronize this worker's replica with the full master state
        SYNC_TIMING,   ///< Synchronize this worker's replica timing data with the master state
        EXIT           ///< Terminate the worker thread
    };

    /// @brief Worker thread loop. Waits for each job and runs it on its replica until EXIT.
    void worker_main_(int worker_id);

    /// @brief Runs the current job on the given worker's replica. Worker 0 is the coordinator.
    void execute_worker_job_(int worker_id);

    /// @brief Waits for the previous job to finish, then publishes a new one without waiting for it.
    void begin_worker_job_(e_worker_job job);

    /// @brief Blocks until all worker threads have finished the current job.
    void wait_worker_job_();

    /// @brief Publishes a job, runs the coordinator's share, and waits for the workers.
    void run_worker_job_(e_worker_job job);

    /// @brief Proposes, evaluates, and reverts one attempt on the given replica.
    /// Accepted attempts also capture their move and commit record.
    void propose_and_evaluate_attempt_(EvalReplica& replica, MoveGenerator& move_generator, int slot_index);

    /// @brief Commits the winning attempt on `replica` by replaying its recorded
    /// committed values (no re-evaluation).
    void apply_winner_to_replica_(EvalReplica& replica, const t_speculative_swap& winner);

    /// @brief Commits the winning attempt on the master state by replaying its
    /// recorded committed values, and updates the placement costs from the
    /// recorded deltas.
    void commit_winner_on_master_(const t_speculative_swap& winner);

    /// @brief Copies the full master state into the given replica.
    void sync_replica_full_(EvalReplica& replica);

    /// @brief Copies the master's committed timing data into the given replica.
    void sync_replica_timing_(EvalReplica& replica);

    /// @brief Derives the deterministic RNG seed of a swap attempt from the
    /// placement seed and the attempt id.
    int attempt_seed_(uint64_t attempt_id) const;

    /// @brief Same acceptance test as PlacementAnnealer::assess_swap_(), but the
    /// uniform random number is passed in rather than drawn here, so the
    /// decision does not depend on evaluation order.
    static e_move_result assess_speculative_swap_(double delta_c, float t, float accept_rand);

  private:
    /// Number of parallel evaluators, including the coordinator.
    const int num_workers_;
    const t_placer_opts& placer_opts_;
    /// Master placement costs, updated when a winner is committed.
    t_placer_costs& costs_;
    PlacerState& master_state_;
    NetCostHandler& master_net_cost_handler_;
    const PlaceDelayModel* delay_model_;
    const PlacerCriticalities* criticalities_;
    NetPinTimingInvalidator* pin_timing_invalidator_;
    SwapEvaluator& master_evaluator_;
    /// The annealer's scratch move record, used to commit winners on the master.
    t_pl_blocks_to_be_moved& master_blocks_affected_;

    /// True for CRITICALITY_TIMING_PLACE.
    const bool timing_driven_;
    /// Base seed that per-attempt RNG seeds are derived from.
    const uint64_t seed_;
    /// Number of attempts retired so far. Discarded attempts do not count.
    uint64_t attempt_counter_ = 0;
    /// True once the replicas have received their first full copy of the master state.
    bool replicas_fully_synced_ = false;

    /// One replica per worker. Replica 0 belongs to the coordinator.
    std::vector<std::unique_ptr<EvalReplica>> replicas_;
    /// Attempts of the most recent batch.
    std::vector<t_speculative_swap> attempts_;

    // ---- Worker pool state ----

    /// The num_workers_ - 1 worker threads. The coordinator is worker 0.
    std::vector<std::thread> workers_;
    /// Incremented to publish a new job. Worker threads spin on it.
    alignas(64) std::atomic<uint64_t> job_epoch_{0};
    /// Current job, written before job_epoch_ is bumped.
    e_worker_job job_ = e_worker_job::NONE;
    /// EVALUATE payload: whether replicas propose with their second generator.
    bool batch_use_gen2_ = false;
    /// EVALUATE payload: number of attempts in the current batch.
    int batch_size_ = 0;
    /// EVALUATE payload: current annealing temperature.
    float batch_temperature_ = 0.f;
    /// EVALUATE payload: current move range limit.
    float batch_rlim_ = 0.f;
    /// EVALUATE payload: sync source for the replica generators' agent state.
    const MoveGenerator* batch_master_generator_ = nullptr;
    /// COMMIT_WINNER payload: the winning attempt.
    const t_speculative_swap* winner_attempt_ = nullptr;
    /// Number of worker threads that finished the current job. Written while
    /// a job runs, so it gets its own cache line.
    alignas(64) std::atomic<int> workers_done_{0};
    /// Lowest accepted attempt id in the current batch, or INT_MAX if none yet.
    /// Attempts with a higher id cancel early. Written while a job runs, so it
    /// gets its own cache line.
    alignas(64) std::atomic<int> first_accepted_id_{std::numeric_limits<int>::max()};
};
