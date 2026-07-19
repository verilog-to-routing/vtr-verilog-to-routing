#pragma once
/**
 * @file
 * @brief Declaration of ClusterPinCounter, the pin-counting state for a
 *        LegalizationCluster.
 *
 * This class is intended to replace the pin-counting fields previously
 * stored on t_pb_stats (input_pins_used, output_pins_used,
 * lookahead_input_pins_used, lookahead_output_pins_used).
 *
 * For every non-primitive t_pb in the cluster, two symmetric states are
 * tracked per input/output pin class:
 *   - committed: nets claimed by molecules already accepted into the cluster
 *   - lookahead: nets claimed by molecules currently under evaluation
 *                (rebuilt from scratch on each candidate check)
 *
 * On a successful commit, lookahead is copied into committed.
 * On a failed check, lookahead is discarded (reset).
 */

#include <unordered_map>
#include <vector>

#include "atom_netlist_fwd.h"

class t_pb;

class ClusterPinCounter {
  public:
    /**
     * @brief Per non-primitive pb pin-counting state.
     *
     * Each of the four vectors is indexed by pin class id at the associated pb,
     * matching the layout previously used on t_pb_stats. The inner vector
     * holds the AtomNetIds currently claiming a pin of that class.
     */
    struct PerPbState {
        std::vector<std::vector<AtomNetId>> committed_input_pin_class_nets;
        std::vector<std::vector<AtomNetId>> committed_output_pin_class_nets;
        std::vector<std::vector<AtomNetId>> lookahead_input_pin_class_nets;
        std::vector<std::vector<AtomNetId>> lookahead_output_pin_class_nets;
    };

    // ---------------- Allocation ----------------

    /**
     * @brief Allocate state for @p pb. Should be called once per each pb.
     *
     * Mirrors the lazy alloc_and_load_pb_stats pattern in cluster_legalizer.cpp.
     * Sizes the per-class vectors from
     *   pb->pb_graph_node->{input,output}_pin_class_sizes.
     */
    void allocate_pb_state(const t_pb* pb);

    /**
     * @brief Drop the state stored for @p pb, if any.
     *
     * Mirrors the free_pb_stats(t_pb*) pattern. Called when a pb is torn down
     * (e.g., during molecule revert) so that stale pb pointers do not remain
     * in the internal map.
     */
    void deallocate(const t_pb* pb);


    /**
     * @brief TODO
     */
    void deallocate_recursive(const t_pb* pb);

    // ---------------- Lookahead writes ----------------

    /**
     * @brief Add @p net to the lookahead input state of @p pb / @p class_id.
     *
     * Deduplicates — mirrors legacy input-side behavior in
     * compute_and_mark_lookahead_pins_used_for_pin.
     */
    void mark_lookahead_input(const t_pb* pb, size_t class_id, AtomNetId net);

    /**
     * @brief Add @p net to the lookahead output state of @p pb / @p class_id.
     *
     * No dedup — mirrors legacy output-side behavior (one driver per net, so
     * duplicates cannot occur within a single cluster).
     */
    void mark_lookahead_output(const t_pb* pb, size_t class_id, AtomNetId net);

    /**
     * @brief Clear lookahead state at every pb. Committed state is untouched.
     */
    void reset_lookahead();

    // ---------------- Commit ----------------

    /**
     * @brief Promote lookahead to committed at every pb.
     *
     * Called after a candidate molecule has been accepted into the cluster.
     */
    void commit_lookahead();

    // ---------------- Size queries ----------------

    size_t lookahead_input_size(const t_pb* pb, size_t class_id) const;
    size_t lookahead_output_size(const t_pb* pb, size_t class_id) const;
    size_t committed_input_size(const t_pb* pb, size_t class_id) const;
    size_t committed_output_size(const t_pb* pb, size_t class_id) const;

    // ---------------- Introspection ----------------

    /**
     * @brief Returns the state associated with @p pb, or nullptr if none has
     *        been allocated.
     *
     * Intended for step-1 verification asserts that compare the new state
     * against the legacy t_pb_stats fields.
     */
    const PerPbState* find(const t_pb* pb) const;

  private:
    /**
     * @brief One entry per non-primitive t_pb touched during clustering.
     *
     * Keyed by pb pointer to mirror the legacy per-t_pb layout, which lets
     * mirror-writes at existing legacy write sites remain point-for-point
     * symmetric.
     *
     * TODO: Each mark/size query does one std::unordered_map::find (average
     *       O(1), but with a hash + modulo + pointer chase, and heap-scattered
     *       buckets). If profiling shows this lookup is a bottleneck, migrate
     *       to a dense-index layout:
     *         1. At cluster construction, walk the pb tree and assign every
     *            non-primitive pb a dense PbLocalId (0..N-1).
     *         2. Replace this map with std::vector<PerPbState> indexed by
     *            PbLocalId; store a parallel map / lookup from t_pb* to
     *            PbLocalId if pointer-keyed access is still needed.
     *         3. find() becomes a direct array index — true O(1) and
     *            cache-friendly.
     *       The public API of this class is intentionally shaped around
     *       (pb, class_id) queries, so this swap is contained within the
     *       .cpp and does not affect callers.
     */
    std::unordered_map<const t_pb*, PerPbState> per_pb_state_;
};
