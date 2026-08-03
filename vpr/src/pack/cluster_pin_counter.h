#pragma once
/**
 * @file
 * @author  Haydar Cakan
 * @date    July 2026
 * @brief   Tracks pin usage inside a cluster during packing for pin
 *          feasibility filter.
 *
 * Declares ClusterPinCounter, used by the packer's pin feasibility filter.
 * The filter checks that for each non-primitive pb in the cluster, the
 * demand of each pin class on that pb does not exceed the supply of that
 * pin class. The supply here is the number of pins on that pin class at
 * that pb level while the demand is the number of nets that need to leave
 * that pb using a pin of that pin class. Given a cluster and molecules, if
 * any pin class at any pb level has a demand greater than its supply, the
 * pin feasibility filter fails; otherwise it succeeds.
 *
 * See Section 4.3.2 of Jason Luu's PhD thesis for the pin feasibility filter
 * that this class is refactored from:
 *   http://hdl.handle.net/1807/68469
 */

#include <cstdint>
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

/**
 * @brief Owns pin usage state for one LegalizationCluster.
 *
 * A single "current" state is kept per pin class: a refcount of the atoms
 * routing each net through that class. .size() of the per class map is the
 * distinct-net count, i.e. the demand the pin feasibility filter compares
 * against the class's supply.
 *
 * Each candidate check mutates the current state and journals every
 * mutation. The caller frames each check with:
 *   snapshot_root_class_sizes(root)
 *   apply_molecule_delta(candidate_id, ...)   // production incremental path
 *   check_pins_used(root, max_ext_pin_util)   // incremental over touched (pb, class)
 * followed by either commit_check() on accept (clears the journal) or
 * rollback_check() on reject (replays the journal in reverse to restore
 * the pre-check state).
 *
 * apply_molecule_delta relies on the monotonicity invariant that within a
 * single cluster's construction, atom membership only grows. Adding a
 * molecule M can only *remove* marks from pre-existing atoms (via nets M
 * now drives or sinks that flip a previous is_reachable / net_exits_cluster
 * decision); it never adds marks to them. Only M's own atoms produce new
 * marks. This is why the delta touches only (a) M's own atoms and (b) the
 * pre-existing atoms on nets M touches, and why check_pins_used need only
 * look at classes the delta incremented: pre-delta state was feasible,
 * decrements cannot break feasibility, and the root clamp
 * (max(scaled_size, snapshot_size)) covers the seed-utilization transition.
 *
 * Processing order inside apply_molecule_delta matters: the re-evaluation
 * of pre-existing atoms (steps 1 and 2) runs before M's own atoms are
 * marked (step 3), and each pre-existing atom pin is first unmarked then
 * re-marked under the new membership. Blind decrement is not sound because
 * reachability is depth-dependent: a pin may mark at some ancestors but
 * not others, so unmarking is driven by a per atom pin record of which pbs
 * the pin actually marked.
 *
 * The root class-size snapshot is required because the pin feasibility
 * filter's root clamp compares against pre-check sizes; the collapsed
 * state no longer stores them separately.
 *
 * full_recompute_from_molecules is retained as the reference oracle used
 * by verify_against_full_recompute and by callers that need a from-scratch
 * rebuild. It is no longer on the production hot path.
 */
class ClusterPinCounter {
  public:
    /**
     * @brief Per non-primitive pb pin counting state.
     *
     * Indexed by pin class id at the associated pb. Inner map: net -> refcount.
     * A net's refcount is the number of atom pins contributing a mark to that
     * (pb, class); the map's .size() is the number of distinct nets, which
     * is what the pin feasibility filter compares against supply.
     */
    struct PerPbState {
        /// @brief Per input pin class: net -> refcount.
        std::vector<std::unordered_map<AtomNetId, uint16_t>> input_pin_class_net_counts;
        /// @brief Per output pin class: net -> refcount.
        std::vector<std::unordered_map<AtomNetId, uint16_t>> output_pin_class_net_counts;
    };

    /**
     * @brief Allocate the pin usage state for given pb.
     *
     * The two per class vectors are sized to the number of input/output pin
     * classes at given pb. Do not call on a pb whose state is already
     * allocated.
     */
    void allocate_pin_count_state(const t_pb* pb);

    /**
     * @brief Erase the pin usage state for given pb and every pb in its subtree.
     *
     * Must be called before the pb subtree is freed, otherwise the pb
     * pointers used for the recursion have been freed. Does not scrub per
     * atom pin mark records: any freed pb that is still referenced by an
     * unpurged mark record will be tolerated (skipped) by unmark_input_pin
     * / unmark_output_pin, and in practice never occurs because the pbs
     * freed on failure paths are only those newly allocated for the failing
     * candidate whose records are already rolled back by rollback_check.
     */
    void deallocate_pin_count_state_recursive(const t_pb* pb);

    /// @brief Number of distinct nets currently marked in the given input pin class of given pb.
    size_t input_size(const t_pb* pb, size_t class_id) const;
    /// @brief Number of distinct nets currently marked in the given output pin class of given pb.
    size_t output_size(const t_pb* pb, size_t class_id) const;

    /**
     * @brief Snapshot the current per class sizes at the cluster root.
     *
     * Must be called at the start of every candidate check, before any
     * mutation. The snapshot is read by check_pins_used to implement the
     * root clamp (which used to compare against the committed sizes; those
     * are gone after the committed/lookahead collapse).
     */
    void snapshot_root_class_sizes(const t_pb* root);

    /**
     * @brief Apply the delta introduced by the candidate molecule to the
     *        current state (production hot path).
     *
     * Implements the three-step delta from the spec:
     *   1. For every net M drives: re-evaluate every pre-existing sink of
     *      that net (unmark then re-mark its input pin under the new
     *      membership; may remove input marks that used to be needed but
     *      no longer are because the driver is now in this cluster).
     *   2. For every net M sinks: re-evaluate the driver (unmark then
     *      re-mark its output pin; may remove output marks that used to
     *      be needed because a sink outside the cluster made the net
     *      exit).
     *   3. Mark every pin of every atom in M itself.
     *
     * Steps 1 and 2 skip atoms in M (whose marks are added by step 3) and
     * cannot collide with each other (step 1 touches input pins, step 2
     * touches output pins). All mutations, including mutations to the per
     * atom pin mark records, are journaled so rollback_check can restore
     * the pre-delta state exactly.
     *
     * The caller must have snapshotted root sizes before calling this
     * (they are read by the subsequent check_pins_used).
     *
     * @param candidate_id  The molecule id currently under evaluation. Its
     *                      atoms must already be placed in their primitive
     *                      pbs (i.e. try_place_atom_block_rec has succeeded)
     *                      and their cluster membership recorded in
     *                      atom_cluster before this call.
     */
    void apply_molecule_delta(PackMoleculeId candidate_id,
                              const Prepacker& prepacker,
                              const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                              const AtomPBBimap& atom_to_pb);

    /**
     * @brief Full recompute of the current state over the given molecule list.
     *
     * Wipes every mark in the current state, then re-marks every atom of
     * every molecule. Every mutation is journaled so the change is undoable
     * via rollback_check.
     *
     * Retained as the reference oracle. verify_against_full_recompute uses
     * it against a scratch counter; production packing uses
     * apply_molecule_delta instead.
     */
    void full_recompute_from_molecules(const std::vector<PackMoleculeId>& molecules,
                                       const Prepacker& prepacker,
                                       const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                                       const AtomPBBimap& atom_to_pb);

    /**
     * @brief Check whether the current pin usage is feasible.
     *
     * Incremental: examines only the (pb, class) pairs that were
     * incremented during the current candidate check (as recorded by the
     * journal). This is sound because the pre-delta state was already
     * feasible and decrements cannot make a class exceed its supply; the
     * root clamp (max(scaled_size, snapshot_size)) covers the one edge
     * case where supply itself tightens between the seed molecule and
     * subsequent molecules.
     *
     * Under VTR_ASSERT_SAFE_ENABLED, additionally runs the full-tree
     * reference check and asserts both agree, catching bugs in the
     * touched-set collection or in the monotonicity assumption.
     *
     * @param cur_pb                 Root of the subtree to check. Also used
     *                               by the debug-only full-tree check.
     * @param max_external_pin_util  Scaling factors applied to root level pin
     *                               class supplies.
     * @return                       True if every touched pin class has demand
     *                               within supply, false otherwise.
     */
    bool check_pins_used(t_pb* cur_pb, t_ext_pin_util max_external_pin_util) const;

    /**
     * @brief Accept the current candidate check: discard the journals.
     *
     * The mutations recorded since the last commit/rollback become
     * permanent; the current state stands as the new accepted-only baseline
     * for the next check.
     */
    void commit_check();

    /**
     * @brief Reject the current candidate check: replay the journals in
     *        reverse, restoring the pre-check state.
     *
     * Must be called on every failure path that could have followed a
     * mutation (pin feasibility failure, intra-cluster routing failure,
     * etc.). Safe to call on an empty journal (no-op).
     *
     * Tolerant of pbs that have been erased from per_pb_state_ since the
     * journal entry was recorded: such entries are skipped. This defence
     * in depth allows the rollback to safely follow revert_place_atom_block
     * or cleanup_pb, though the caller should still order rollback FIRST
     * whenever possible to keep reasoning local.
     */
    void rollback_check();

    /**
     * @brief Debug-only equivalence check against a from-scratch recompute.
     *
     * Constructs a scratch ClusterPinCounter over the same pb set as this
     * counter, runs the full recompute path on it over the given molecule
     * list, and asserts that every (pb, class) has an identical net -> count
     * map. Comparison is over the full refcount, not just the distinct-net
     * set: a refcount discrepancy indicates a mark accounting bug that
     * would eventually break rollback.
     *
     * Callers must gate the call under VTR_ASSERT_SAFE_ENABLED (or an
     * equivalent debug flag): this method is expensive and must never run
     * in release builds.
     *
     * @param molecules     Molecule ids representing the intended cluster
     *                      state at the call site.
     * @param prepacker     Used to resolve each PackMoleculeId to its atom list.
     * @param atom_cluster  Maps atoms to the legalization cluster that owns them.
     * @param atom_to_pb    Maps atoms to their assigned primitive pb.
     */
    void verify_against_full_recompute(const std::vector<PackMoleculeId>& molecules,
                                       const Prepacker& prepacker,
                                       const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                                       const AtomPBBimap& atom_to_pb) const;

    /**
     * @brief Debug-only reachability check for the tracked pb set.
     *
     * Walks the pb subtree rooted at cluster_root and asserts that every pb
     * currently tracked in per_pb_state_ appears in that subtree. A tracked
     * pb that is unreachable indicates a lifetime bug: some code path freed
     * the pb without asking this counter to deallocate its state first,
     * leaving a dangling pointer as a live key.
     */
    void assert_all_pbs_reachable_from(const t_pb* cluster_root) const;

  private:
    /**
     * @brief A single journaled mutation to per class refcounts.
     *
     * Each add or remove of a mark emits one entry with change == +1 or -1.
     * Rollback replays entries in reverse, applying the negated change.
     */
    struct MarkDelta {
        const t_pb* pb;
        AtomNetId net;
        uint32_t class_id;
        bool is_input;
        int8_t change; // +1 or -1
    };

    /**
     * @brief A single journaled mutation to a per atom pin mark record.
     *
     * change == +1: pb was appended to mark_record[pin]; undo pops it.
     * change == -1: pb was removed from mark_record[pin] (via a bulk clear
     *   during unmark_*_pin); undo pushes it back.
     * On pop-to-empty during undo, the map entry is erased; on push into a
     * missing key, the entry is created.
     */
    struct MarkRecordDelta {
        AtomPinId pin;
        const t_pb* pb;
        bool is_input;
        int8_t change; // +1 or -1
    };

    /**
     * @brief Increment the refcount of net in the given (pb, class) map and
     *        journal the change.
     */
    void add_mark(const t_pb* pb, bool is_input, size_t class_id, AtomNetId net);

    /**
     * @brief Decrement the refcount of net in the given (pb, class) map,
     *        erasing the key if the count reaches zero, and journal the
     *        change.
     */
    void remove_mark(const t_pb* pb, bool is_input, size_t class_id, AtomNetId net);

    /**
     * @brief Iterate every entry in every (pb, class) map and issue a
     *        remove_mark per unit of refcount, leaving all maps empty and
     *        the journal populated with the corresponding decrements.
     *
     * Called by full_recompute_from_molecules. The production incremental
     * path apply_molecule_delta does not use this.
     */
    void wipe_all_marks_journaled();

    /**
     * @brief Undo every mark this input atom pin previously contributed and
     *        clear its record entry. Journals both the state decrements and
     *        the record deletions.
     */
    void unmark_input_pin(AtomPinId pin_id, AtomNetId net_id, const AtomPBBimap& atom_to_pb);

    /**
     * @brief Undo every mark this output atom pin previously contributed and
     *        clear its record entry. Journals both the state decrements and
     *        the record deletions.
     */
    void unmark_output_pin(AtomPinId pin_id, AtomNetId net_id, const AtomPBBimap& atom_to_pb);

    /**
     * @brief Add the given atom's pin usage contribution to the current
     *        state (via add_mark, so journaled). Also populates the per
     *        atom pin mark records for later unmark.
     */
    void compute_and_mark_pins_used(AtomBlockId blk_id,
                                    const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                                    const AtomPBBimap& atom_to_pb);

    /**
     * @brief Given an input pin and its assigned net, mark all pin classes
     *        that are affected and record the pbs marked into
     *        input_mark_record_[pin_id]. Check if connecting this pin to
     *        its driver pin will require entering a pb block starting from
     *        the parent pb block of the primitive till the root block. If
     *        entering a pb block is required, add this net to the input
     *        pin class.
     */
    void compute_and_mark_pins_used_for_input_pin(AtomPinId pin_id,
                                                  const t_pb_graph_pin* pb_graph_pin,
                                                  const t_pb* primitive_pb,
                                                  AtomNetId net_id,
                                                  const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                                                  const AtomPBBimap& atom_to_pb);

    /**
     * @brief Given an output pin and its assigned net, mark all pin classes
     *        that are affected and record the pbs marked into
     *        output_mark_record_[pin_id]. Check if connecting this pin to
     *        all its sink pins will require leaving a pb block starting
     *        from the parent pb block of the primitive till the root
     *        block. If leaving a pb block is required, add this net to
     *        the output pin class.
     */
    void compute_and_mark_pins_used_for_output_pin(AtomPinId pin_id,
                                                   const t_pb_graph_pin* pb_graph_pin,
                                                   const t_pb* primitive_pb,
                                                   AtomNetId net_id,
                                                   const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                                                   const AtomPBBimap& atom_to_pb);

    /**
     * @brief Append a pb to input_mark_record_[pin] (creating the key if
     *        necessary) and journal the change.
     */
    void record_input_mark(AtomPinId pin, const t_pb* pb);

    /**
     * @brief Append a pb to output_mark_record_[pin] (creating the key if
     *        necessary) and journal the change.
     */
    void record_output_mark(AtomPinId pin, const t_pb* pb);

    /**
     * @brief Full-tree reference check, retained as a debug oracle for
     *        check_pins_used. Walks every non-primitive pb in the subtree
     *        and tests every pin class against its supply.
     */
    bool check_pins_used_full_reference(t_pb* cur_pb, t_ext_pin_util max_external_pin_util) const;

    /**
     * @brief Pin usage state for every non-primitive pb visited during
     *        clustering, keyed by pb pointer.
     *
     * TODO: The per_pb_state_.find call on the mark/query hot path may be
     *       expensive. Can consider dense index storage.
     */
    std::unordered_map<const t_pb*, PerPbState> per_pb_state_;

    /**
     * @brief Per atom pin record of which pbs the pin's mark landed on.
     *
     * Required because reachability is depth-dependent: unmark cannot be
     * implemented by re-walking the marking logic under the new membership
     * (that would give the NEW marks, not the ones that need removing),
     * and it cannot be implemented by blind decrement at every ancestor
     * (that would corrupt classes the pin never marked). The record makes
     * unmark exactly reverse the pin's original marking walk.
     *
     * Class id and net id are recoverable at unmark time from the pin's
     * pb_graph_pin (parent_pin_class[pb->depth]) and the pin's net, so
     * the record only needs to store the pbs.
     */
    std::unordered_map<AtomPinId, std::vector<const t_pb*>> input_mark_record_;
    std::unordered_map<AtomPinId, std::vector<const t_pb*>> output_mark_record_;

    /**
     * @brief Journal of mutations performed during the current candidate
     *        check. Cleared on commit_check; replayed in reverse on
     *        rollback_check.
     */
    std::vector<MarkDelta> journal_;

    /**
     * @brief Journal of mutations to the per atom pin mark records during
     *        the current candidate check. Cleared on commit_check;
     *        replayed in reverse on rollback_check.
     */
    std::vector<MarkRecordDelta> record_journal_;

    /**
     * @brief Snapshot of the root pb's per class sizes taken at the start
     *        of the current candidate check. Used by check_pins_used to
     *        implement the root clamp against pre-check (accepted-only)
     *        sizes.
     */
    std::vector<size_t> root_input_class_size_snapshot_;
    std::vector<size_t> root_output_class_size_snapshot_;
};
