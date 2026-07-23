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
 *   https://www.eecg.toronto.edu/~jayar/pubs/theses/Luu/JasonLuuPhD.pdf
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

/**
 * @brief Owns pin usage state for one LegalizationCluster.
 *
 * Two states are kept per pin class:
 *   - committed: nets claimed by molecules already accepted into the cluster.
 *   - lookahead: nets claimed by molecules currently under evaluation,
 *                rebuilt from scratch on each candidate check.
 *
 * When a candidate check is accepted, lookahead is promoted into committed;
 * the next check's reset then drops the lookahead side. When a check is
 * rejected, no promotion happens and the next reset drops lookahead.
 */
class ClusterPinCounter {
  public:
    /**
     * @brief Per non-primitive pb pin counting state.
     *
     * Each of the four vectors is indexed by pin class id at the associated pb.
     * The inner vector holds the AtomNetIds currently claiming a pin of that class.
     * Using vector for inner container because per class net counts are expected to be small.
     */
    struct PerPbState {
        /// @brief Nets currently claiming an input pin of each input pin class, contributed by accepted molecules.
        std::vector<std::vector<AtomNetId>> committed_input_pin_class_nets;
        /// @brief Nets currently claiming an output pin of each output pin class, contributed by accepted molecules.
        std::vector<std::vector<AtomNetId>> committed_output_pin_class_nets;
        /// @brief Nets speculatively claiming an input pin of each input pin class during a candidate check.
        std::vector<std::vector<AtomNetId>> lookahead_input_pin_class_nets;
        /// @brief Nets speculatively claiming an output pin of each output pin class during a candidate check.
        std::vector<std::vector<AtomNetId>> lookahead_output_pin_class_nets;
    };

    /**
     * @brief Allocate the pin usage state for given pb if not already allocated.
     *
     * The four per-class vectors are sized to the number of input/output pin
     * classes at given pb.
     *
     * @param pb  The pb to allocate state for.
     */
    void allocate_pb_state(const t_pb* pb);

    /**
     * @brief Erase the pin usage state stored for given pb, if any.
     *
     * Only the current pb is affected; descendants are not touched.
     * Prefer deallocate_recursive when tearing down a pb subtree.
     *
     * @param pb  The pb to erase state for.
     */
    void deallocate(const t_pb* pb);

    /**
     * @brief Erase the pin usage state for given pb and every pb in its subtree.
     *
     * Must be called before the pb subtree is freed, otherwise the pb
     * pointers used for the recursion have been freed.
     *
     * @param pb  Root of the subtree to erase state for.
     */
    void deallocate_recursive(const t_pb* pb);

    /**
     * @brief Add given net to the lookahead input state of given pin class of
     *        given pb.
     *
     * No-op if the net is already recorded in the class (deduplicated).
     *
     * @param pb        The pb whose lookahead input state is updated.
     * @param class_id  Input pin class id at pb to add net to.
     * @param net       The net to add.
     */
    void mark_lookahead_input(const t_pb* pb, size_t class_id, AtomNetId net);

    /**
     * @brief Add given net to the lookahead output state of given pin class of
     *        given pb.
     *
     * No dedup: a net has a single driver, so duplicates cannot occur within
     * a single cluster.
     *
     * @param pb        The pb whose lookahead output state is updated.
     * @param class_id  Output pin class id at pb to add net to.
     * @param net       The net to add.
     */
    void mark_lookahead_output(const t_pb* pb, size_t class_id, AtomNetId net);

    /**
     * @brief Clear lookahead state at every pb.
     */
    void reset_lookahead();

    /**
     * @brief Promote the lookahead state to committed at every pb.
     *
     * Called after a candidate molecule has been accepted into the cluster.
     */
    void commit_lookahead();

    /// @brief Number of nets in the lookahead input state of the given pin class of the given pb.
    size_t lookahead_input_size(const t_pb* pb, size_t class_id) const;
    /// @brief Number of nets in the lookahead output state of the given pin class of the given pb.
    size_t lookahead_output_size(const t_pb* pb, size_t class_id) const;
    /// @brief Number of nets in the committed input state of the given pin class of the given pb.
    size_t committed_input_size(const t_pb* pb, size_t class_id) const;
    /// @brief Number of nets in the committed output state of the given pin class of the given pb.
    size_t committed_output_size(const t_pb* pb, size_t class_id) const;

    /**
     * @brief Recompute the lookahead state from scratch for every atom of every
     *        molecule in the given molecule list.
     *
     * Called on each candidate check, after reset_lookahead and before
     * check_lookahead_pins_used. The molecule list should contain every molecule
     * currently in the cluster: already committed molecules plus the candidate
     * being tested.
     *
     * @param molecules     Molecule ids currently under evaluation in the cluster.
     * @param prepacker     Used to resolve each PackMoleculeId to its atom list.
     * @param atom_cluster  Maps atoms to the legalization cluster that owns them.
     * @param atom_to_pb    Maps atoms to their assigned primitive pb.
     */
    void try_update_lookahead_pins_used(const std::vector<PackMoleculeId>& molecules,
                                        const Prepacker& prepacker,
                                        const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                                        const AtomPBBimap& atom_to_pb);

    /**
     * @brief Check whether the lookahead pin usage is feasible at every
     *        non-primitive pb in the subtree rooted at the given pb.
     *
     * The demand of each pin class is compared against its supply. At the
     * cluster root, the supply is scaled by max_external_pin_util. If the
     * current committed usage already exceeds this scaled supply, the supply
     * is raised to that committed level so already committed pins are not
     * rejected. At non-root pbs, the raw pin class size is used as the supply.
     *
     * @param cur_pb                 Root of the subtree to check.
     * @param max_external_pin_util  Scaling factors applied to root level pin
     *                               class supplies.
     * @return                       True if every pin class has demand within
     *                               supply, false otherwise.
     */
    bool check_lookahead_pins_used(t_pb* cur_pb, t_ext_pin_util max_external_pin_util);

  private:
    /**
     * @brief Add the given atom's pin usage contribution to the lookahead state.
     *
     * For each pin of the atom, walks from the atom's primitive pb up to the
     * root and marks the pin class at each level that this pin's net would
     * claim.
     *
     * @param blk_id        The atom whose pins are being marked.
     * @param atom_cluster  Maps atoms to the legalization cluster that owns them.
     * @param atom_to_pb    Maps atoms to their assigned primitive pb.
     */
    void compute_and_mark_lookahead_pins_used(AtomBlockId blk_id,
                                              const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                                              const AtomPBBimap& atom_to_pb);

    /**
     * @brief Given an input pin and its assigned net, mark all pin classes that are
     *        affected. Check if connecting this pin to its driver pin will require
     *        entering a pb block starting from the parent pb block of the primitive
     *        till the root block (depth = 0). If entering a pb block is required,
     *        add this net to the input pin class (to increment the number of used
     *        pins from this class) that should be used to enter the pb block.
     *
     * @param pb_graph_pin  The input pin being marked.
     * @param primitive_pb  The primitive pb that owns pb_graph_pin.
     * @param net_id        The net connected to pb_graph_pin.
     * @param atom_cluster  Maps atoms to the legalization cluster that owns them.
     * @param atom_to_pb    Maps atoms to their assigned primitive pb.
     */
    void compute_and_mark_lookahead_pins_used_for_input_pin(const t_pb_graph_pin* pb_graph_pin,
                                                            const t_pb* primitive_pb,
                                                            AtomNetId net_id,
                                                            const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                                                            const AtomPBBimap& atom_to_pb);

    /**
     * @brief Given an output pin and its assigned net, mark all pin classes that are
     *        affected. Check if connecting this pin to all its sink pins will require
     *        leaving a pb block starting from the parent pb block of the primitive
     *        till the root block (depth = 0). If leaving a pb block is required,
     *        add this net to the output pin class (to increment the number of used
     *        pins from this class) that should be used to leave the pb block.
     *
     * @param pb_graph_pin  The output pin being marked.
     * @param primitive_pb  The primitive pb that owns pb_graph_pin.
     * @param net_id        The net driven by pb_graph_pin.
     * @param atom_cluster  Maps atoms to the legalization cluster that owns them.
     * @param atom_to_pb    Maps atoms to their assigned primitive pb.
     */
    void compute_and_mark_lookahead_pins_used_for_output_pin(const t_pb_graph_pin* pb_graph_pin,
                                                            const t_pb* primitive_pb,
                                                            AtomNetId net_id,
                                                            const vtr::vector_map<AtomBlockId, LegalizationClusterId>& atom_cluster,
                                                            const AtomPBBimap& atom_to_pb);



    /**
     * @brief Pin usage state for every non-primitive pb visited during
     *        clustering, keyed by pb pointer.
     *
     * TODO: The per_pb_state_.find call on the mark/query hot path may be
     *       expensive. Can consider dense index storage.
     */
    std::unordered_map<const t_pb*, PerPbState> per_pb_state_;
};
