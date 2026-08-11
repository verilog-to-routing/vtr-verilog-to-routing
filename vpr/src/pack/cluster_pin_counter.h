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
 * @brief Tracks pin usage inside one LegalizationCluster during packing,
 *        supporting incremental candidate feasibility checks.
 *
 * The pin feasibility filter answers, for each candidate molecule, whether
 * adding it keeps every pin class within its supply. This class maintains
 * the pin state incrementally instead of recomputing it from scratch on
 * every candidate, which would be expensive. Only atoms affected by the
 * candidate are touched, and every mutation is journaled so a failed check
 * can be reverted.
 *
 * State stored per cluster:
 *   - per_pb_state_: for every non-primitive pb in the cluster, per pin
 *     class, a map from net to refcount. The refcount is the number of atom
 *     pins currently marking (pb, class, net); the map's size() is the
 *     distinct net count the filter compares against the class's supply.
 *   - input_mark_record_ / output_mark_record_: per atom pin, the list of
 *     pbs the pin's mark landed on. Needed to undo a pin's marks because
 *     reachability is depth dependent and cannot be recomputed from current
 *     pb membership alone.
 *   - per_pb_state_journal_ / mark_record_journal_: mutations recorded
 *     during a candidate check (to per_pb_state_ refcounts and to
 *     input_mark_record_ / output_mark_record_ lists, respectively),
 *     replayed in reverse on failure.
 *
 * Usage per cluster: call allocate_pin_count_state(pb) on every pb during
 * setup. Then for each candidate molecule, run snapshot_root_class_sizes(root),
 * apply_molecule_delta(candidate), and check_pins_used(root, max_ext_pin_util).
 * On accept, call commit_check() to discard the journals. On reject, call
 * rollback_check() to replay the journals in reverse. When the cluster's
 * pin counter is no longer needed, call deallocate_pin_count_state_recursive(root).
 * Both destroy_cluster and clean_cluster do this; destroy_cluster additionally
 * frees the pbs, so the deallocation must happen before that.
 *
 * How the incremental algorithm works (apply_molecule_delta): adding a
 * candidate molecule can only change three groups of marks in the pin state.
 *   1. When the candidate drives a net that has pre-existing sinks in the
 *      cluster, those sinks may lose their input mark because the driver
 *      is now in cluster too.
 *   2. When the candidate sinks a net whose driver is already in the
 *      cluster, that driver may lose its output mark because a sink is
 *      now in cluster too.
 *   3. The candidate's own atoms produce new marks.
 * apply_molecule_delta handles these three cases and journals every mark
 * change; all other atoms in the cluster are unaffected.
 *
 * Debug only consistency checks: verify_against_full_recompute rebuilds the
 * state from scratch (via full_recompute_from_molecules) and asserts it
 * matches the incremental result. assert_all_pbs_reachable_from catches
 * lifetime bugs where a pb was freed without informing the counter. Both are
 * expensive and are guarded by VTR_ASSERT_SAFE_ENABLED at their call sites.
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
     *
     * @param pb The pb to allocate state for.
     */
    void allocate_pin_count_state(const t_pb* pb);

    /**
     * @brief Erase the pin usage state for given pb and every pb in its subtree.
     *
     * Must be called before the pb subtree is freed, otherwise the pb
     * pointers used for the recursion have been freed.
     *
     * @param pb  Root of the subtree to erase state for.
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
     * mutation. check_pins_used reads the snapshot to implement the root
     * clamp. The clamp raises the effective supply of each root class to at
     * least the snapshotted size. This is needed because the seed molecule
     * is packed with FULL_EXTERNAL_PIN_UTIL and can already exceed the
     * scaled supply. Without the clamp, every subsequent check would fail.
     */
    void snapshot_root_class_sizes(const t_pb* root);

    /**
     * @brief Apply the delta introduced by the candidate molecule to the
     *        current state.
     *
     * Adding a candidate molecule can only change three groups of marks in
     * the pin state:
     *   1. When the candidate drives a net that has pre-existing sinks in
     *      the cluster, those sinks may lose their input mark because the
     *      driver is now in cluster too.
     *   2. When the candidate sinks a net whose driver is already in the
     *      cluster, that driver may lose its output mark because a sink is
     *      now in cluster too.
     *   3. The candidate's own atoms produce new marks.
     * All mutations are journaled so rollback_check can restore the pre-delta
     * state exactly.
     *
     * Note: snapshot_root_class_sizes must have been called for this check,
     * and the candidate's atoms must already be placed in their primitive pbs
     * and recorded in atom_cluster (i.e. try_place_atom_block_rec has succeeded).
     *
     * @param candidate_id  The molecule id currently under evaluation.
     */
    void apply_molecule_delta(PackMoleculeId candidate_id,
                              const Prepacker& prepacker,
                              const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                              const AtomPBBimap& atom_to_pb);

    /**
     * @brief Check whether the current pin usage is feasible.
     *
     * Incremental: examines only the (pb, class) pairs incremented during
     * this candidate check.
     *
     * @param cur_pb                 Root of the subtree to check.
     * @param max_external_pin_util  Scaling factors applied to root class supplies.
     * @return                       True if every touched pin class is within supply.
     */
    bool check_pins_used(t_pb* cur_pb, t_ext_pin_util max_external_pin_util) const;

    /**
     * @brief Accept the current candidate check: discard the journals,
     *        leaving the current state as the new baseline.
     */
    void commit_check();

    /**
     * @brief Reject the current candidate check: replay the journals in
     *        reverse to restore the state before check.
     *
     * Must be called on every failure path that could have followed a
     * mutation (pin feasibility failure, intra-cluster routing failure,
     * etc.). Safe to call on an empty journal (no-op).
     */
    void rollback_check();

    // =========================================================================
    // Debug only helpers
    // =========================================================================

    /**
     * @brief Recompute the current state from scratch over the given
     *        molecule list.
     */
    void full_recompute_from_molecules(const std::vector<PackMoleculeId>& molecules,
                                       const Prepacker& prepacker,
                                       const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                                       const AtomPBBimap& atom_to_pb);

    /**
     * @brief Build a scratch counter via full_recompute_from_molecules over
     *        the given molecule list, and assert every (pb, class) net
     *        refcount matches this counter.
     */
    void verify_against_full_recompute(const std::vector<PackMoleculeId>& molecules,
                                       const Prepacker& prepacker,
                                       const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                                       const AtomPBBimap& atom_to_pb) const;

    /**
     * @brief Assert every pb tracked in per_pb_state_ is reachable from
     *        cluster_root. Catches lifetime bugs where a pb was freed
     *        without calling deallocate_pin_count_state_recursive.
     */
    void assert_all_pbs_reachable_from(const t_pb* cluster_root) const;

  private:
    /**
     * @brief One journaled mutation to per_pb_state_ refcounts. Rollback
     *        replays entries in reverse.
     *
     * TODO: The remaining fixed-width types (uint32_t class_id here, uint16_t
     *       refcount in PerPbState) could be replaced with plain int / size_t
     *       for VPR-idiomatic consistency. Kept for now because they affect
     *       struct/map-entry size and warrant a perf check before changing.
     */
    struct PerPbStateDelta {
        const t_pb* pb;    ///< Which pb the mutated refcount belongs to.
        AtomNetId net;     ///< Which net's refcount changed.
        uint32_t class_id; ///< Which pin class at pb.
        bool is_input;     ///< True: input pin class. False: output.
        int change;        ///< +1 on add_mark, -1 on remove_mark.
    };

    /**
     * @brief One journaled mutation to input_mark_record_ / output_mark_record_.
     *        Rollback replays entries in reverse.
     */
    struct MarkRecordDelta {
        AtomPinId pin;  ///< Which pin's mark record was mutated.
        const t_pb* pb; ///< The pb added to or removed from that pin's record.
        bool is_input;  ///< True: input_mark_record_. False: output_mark_record_.
        int change;     ///< +1 on append (pb added to the record), -1 on removal.
    };

    /// @brief Increment the refcount of net_id in the given (pb, class) map. Journals the change.
    void add_mark(const t_pb* pb, bool is_input, size_t class_id, AtomNetId net_id);

    /// @brief Decrement the refcount of net_id in the given (pb, class) map, erasing the key at zero. Journals the change.
    void remove_mark(const t_pb* pb, bool is_input, size_t class_id, AtomNetId net_id);

    /// @brief Append pb to input_mark_record_[pin_id] (creating the entry if missing). Journals the append.
    void record_input_mark(AtomPinId pin_id, const t_pb* pb);

    /// @brief Append pb to output_mark_record_[pin_id] (creating the entry if missing). Journals the append.
    void record_output_mark(AtomPinId pin_id, const t_pb* pb);

    /// @brief Remove every mark this input pin has contributed, and clear its entry in input_mark_record_. Undoable via rollback_check.
    void remove_input_pin_marks(AtomPinId pin_id, AtomNetId net_id, const AtomPBBimap& atom_to_pb);

    /// @brief Remove every mark this output pin has contributed, and clear its entry in output_mark_record_. Undoable via rollback_check.
    void remove_output_pin_marks(AtomPinId pin_id, AtomNetId net_id, const AtomPBBimap& atom_to_pb);

    /// @brief Wipe all pin state, journaled so it can be rolled back. Used only by full_recompute_from_molecules.
    void wipe_all_marks_journaled();

    /**
     * @brief Add the given atom's pin marks: for each pin, mark the classes
     *        at every pb the pin's net must cross on the walk from
     *        primitive to root.
     */
    void compute_and_mark_pins_used(AtomBlockId blk_id,
                                    const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                                    const AtomPBBimap& atom_to_pb);

    /**
     * @brief Walk from primitive_pb up to root. At each pb where the pin's
     *        net must enter through an input pin (i.e. the driver is not
     *        reachable inside the pb), mark the input pin class and record
     *        the pb into input_mark_record_[pin_id] so remove_input_pin_marks
     *        can undo.
     */
    void compute_and_mark_pins_used_for_input_pin(AtomPinId pin_id,
                                                  const t_pb_graph_pin* pb_graph_pin,
                                                  const t_pb* primitive_pb,
                                                  AtomNetId net_id,
                                                  const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                                                  const AtomPBBimap& atom_to_pb);

    /**
     * @brief Walk from primitive_pb up to root. At each pb where the pin's
     *        net must exit through an output pin (i.e. some sink is outside
     *        the pb), mark the output pin class and record the pb into
     *        output_mark_record_[pin_id] so remove_output_pin_marks can undo.
     */
    void compute_and_mark_pins_used_for_output_pin(AtomPinId pin_id,
                                                   const t_pb_graph_pin* pb_graph_pin,
                                                   const t_pb* primitive_pb,
                                                   AtomNetId net_id,
                                                   const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                                                   const AtomPBBimap& atom_to_pb);

    /**
     * @brief Debug reference: walk every non-primitive pb in the subtree and
     *        test every pin class against its supply. Used to cross check
     *        check_pins_used under VTR_ASSERT_SAFE_ENABLED.
     */
    bool check_pins_used_full_reference(t_pb* cur_pb, t_ext_pin_util max_external_pin_util) const;

    /**
     * @brief Pin usage state for every non-primitive pb visited during
     *        clustering, keyed by pb pointer.
     *
     * TODO: Consider dense index storage; per_pb_state_.find is on the hot path.
     */
    std::unordered_map<const t_pb*, PerPbState> per_pb_state_;

    /**
     * @brief Per atom pin record of which pbs the pin's mark landed on. Read by
     *        remove_input_pin_marks / remove_output_pin_marks to undo those
     *        marks exactly.
     */
    std::unordered_map<AtomPinId, std::vector<const t_pb*>> input_mark_record_;
    std::unordered_map<AtomPinId, std::vector<const t_pb*>> output_mark_record_;

    /// @brief Journal of per_pb_state_ mutations for the current candidate check.
    std::vector<PerPbStateDelta> per_pb_state_journal_;

    /// @brief Journal of input_mark_record_ / output_mark_record_ mutations for the current candidate check.
    std::vector<MarkRecordDelta> mark_record_journal_;

    /// @brief Root pb class sizes captured by snapshot_root_class_sizes at the start of the check. Read by check_pins_used for the root clamp.
    std::vector<size_t> root_input_class_size_snapshot_;
    std::vector<size_t> root_output_class_size_snapshot_;
};
