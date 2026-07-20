#pragma once
/**
 * @file
 * @brief Declaration of ClusterPinCounter, the pin-counting state for a
 *        LegalizationCluster.
 *
 * For every non-primitive t_pb in the cluster, two symmetric states are
 * tracked per input/output pin class:
 *   - committed: nets claimed by molecules already accepted into the cluster
 *   - lookahead: nets claimed by molecules currently under evaluation
 *                (rebuilt from scratch on each candidate check)
 *
 * On a successful commit, lookahead is promoted into committed.
 * On a failed check, lookahead is discarded (reset).
 */

#include <unordered_map>
#include <vector>

#include "atom_netlist_fwd.h"
#include "cluster_legalizer_fwd.h"
#include "prepack.h"
#include "vpr_types.h"
#include "vtr_vector_map.h"

class AtomPBBimap;
class t_pb;
struct t_pb_graph_pin;

class ClusterPinCounter {
  public:
    /**
     * @brief Per non-primitive pb pin-counting state.
     *
     * Each of the four vectors is indexed by pin class id at the associated pb.
     * The inner vector holds the AtomNetIds currently claiming a pin of that
     * class.
     */
    struct PerPbState {
        std::vector<std::vector<AtomNetId>> committed_input_pin_class_nets;
        std::vector<std::vector<AtomNetId>> committed_output_pin_class_nets;
        std::vector<std::vector<AtomNetId>> lookahead_input_pin_class_nets;
        std::vector<std::vector<AtomNetId>> lookahead_output_pin_class_nets;
    };

    // ---------------- Allocation ----------------

    /**
     * @brief Allocate state for @p pb.
     *
     * Idempotent — safe to call multiple times on the same pb; subsequent
     * calls are no-ops. Mirrors the lazy alloc_and_load_pb_stats pattern in
     * cluster_legalizer.cpp. Sizes the per-class vectors from
     *   pb->pb_graph_node->{input,output}_pin_class_sizes.
     */
    void allocate_pb_state(const t_pb* pb);

    /**
     * @brief Drop the state stored for @p pb, if any.
     *
     * Erases only @p pb's own entry; does not touch descendants. Prefer
     * deallocate_recursive when tearing down a pb subtree. Safe to call on
     * a pb that has no state (no-op).
     */
    void deallocate(const t_pb* pb);

    /**
     * @brief Erase state for @p pb and every pb in its subtree.
     *
     * Mirrors the shape of free_pb / free_pb_stats_recursive in
     * cluster_legalizer.cpp. Must be called BEFORE the corresponding
     * legacy free — otherwise the pb pointers used for the recursion
     * have been delete[]-freed and traversing them is UB.
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

    // ---------------- Recompute / check ----------------

    /**
     * @brief Recompute speculative lookahead pin usage for every atom currently
     *        assigned to the cluster.
     */
    void try_update_lookahead_pins_used(const std::vector<PackMoleculeId>& molecules,
                                        const Prepacker& prepacker,
                                        const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                                        const AtomPBBimap& atom_to_pb);

    /**
     * @brief Check if the number of available inputs/outputs for a pin class
     *        is sufficient for speculatively packed blocks.
     */
    bool check_lookahead_pins_used(t_pb* cur_pb, t_ext_pin_util max_external_pin_util);

  private:
    /**
     * @brief Determine if pins of speculatively packed pb are legal.
     */
    void compute_and_mark_lookahead_pins_used(AtomBlockId blk_id,
                                              const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                                              const AtomPBBimap& atom_to_pb);

    /**
     * @brief Given a pin and its assigned net, mark all pin classes that are
     *        affected. Check if connecting this pin to its driver pin or to
     *        all sink pins will require leaving a pb_block starting from the
     *        parent pb_block of the primitive till the root block (depth = 0).
     *        If leaving a pb_block is required add this net to the pin class
     *        (to increment the number of used pins from this class) that
     *        should be used to leave the pb_block.
     */
    void compute_and_mark_lookahead_pins_used_for_pin(const t_pb_graph_pin* pb_graph_pin,
                                                      const t_pb* primitive_pb,
                                                      AtomNetId net_id,
                                                      const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                                                      const AtomPBBimap& atom_to_pb);


    /**
     * @brief One entry per non-primitive t_pb touched during clustering,
     *        keyed by pb pointer.
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
