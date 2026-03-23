#pragma once
/**
 * @file
 * @author  Milo Liebster
 * @date    January 2026
 * @brief   The declaration of the PackingSignatureTree class.
 *
 * Overview
 * ========
 * A data structure for tracking and recognizing cluster packing patterns that
 * have been seen previously during the packing stage. This enables the
 * memoization of redundant legality checks done during detailed packing which
 * can dominate VPR runtime for complex logic block architectures.
 *
 * TODO Cite "Deja-Vu Packing" FCCM paper following publication
 *
 * Theory of Operation
 * -------------------
 * Because the VPR packer is greedy and deterministic, repeated structures that
 * show up in circuits will tend to be processed identically by the packer and
 * produce matching clusters. Tracking seen cluster patterns in a decision tree
 * lets us memoize and reuse cluster legalization outcomes.
 *
 * Definitions
 * ===========
 *
 * Packing Pattern
 * ---------------
 * An arrangement of netlist atoms and their connections that have been mapped
 * to specific architecture primitives within a pb during clustering. This
 * excludes the physical routing interconnects used to realize the described
 * connections between primitive pins (packing patterns may represent desired
 * clusters that cannot be legally routed). There may be multiple cluster
 * instances with a given packing pattern in a final packing solution.
 *
 * Packing Signature
 * -----------------
 * An ordered encoding of a packing pattern. It is necessary to impose an
 * ordering on packing patterns (which are fundamentally graphs) in order to
 * compare them for memoization in a reasonable amount of time. To illustrate
 * this, consider a simple graph with nodes labeled '*', '@', and '$' (in no
 * particular order):
 *
 *     +->[*]--+->[@]  +---------------------------------+
 *     |       |       | NODES: {*, @, $}                |
 *     |       |       | EDGES: {(*, @), (*, $), ($, *)} |
 *     +--[$]<-+       +---------------------------------+
 *
 * There are N! * E! orderings (N = # nodes, E = # edges) in which this graph
 * could be described, making comparison for memoization difficult. However, if
 * the nodes are ordered 'A', 'B', 'C', we can describe the graph in the
 * following manner, where incoming and outgoing edges are only described once
 * all the nodes they connect have been described:
 *
 *     +->[A]--+->[C]  +-------------------------------------+
 *     |       |       | 1. A: {in: { },      out: { }}      |
 *     |       |       | 2. B: {in: {(A, B)}, out: {(B, A)}} |
 *     +--[B]<-+       | 3. C: {in: {(A, C)}, out: { }}      |
 *                     +-------------------------------------+
 *
 * This representation makes element-wise comparison for memoization simple and
 * this principle can be extended to encode packing patterns. When forming
 * clusters, primitives are added sequentially in a predictable order
 * determined by the packing algorithm. Every atom added results in a new
 * possible cluster packing pattern; thus, subsets of packing signatures (e.g.
 * lines 1 + 2, or just line 1 on its own, in this simplified case) are
 * themselves packing signatures.
 *
 * For the PackingSignatureTree class, each of lines 1, 2, and 3 are encoded as
 * nodes in a search tree that can be traversed to lookup a desired value (the
 * routing legality) associated with a particular packing pattern, if it has
 * been seen before.
 *
 * Location and Connectivity Node (LCN)
 * ------------------------------------
 * A structure encoding a pb primitive location where a netlist atom has been
 * mapped by the clusterer, as well as connections to other netlist atoms that
 * have been added to the cluster previously. Conceptually, an LCN corresponds
 * to one of the lines in the ordered graph encoding provided above. For netlist
 * atoms, which might have multiple incoming and/or outgoing connections, the
 * LCN representation gets extended to look like the following:
 *
 *           +----------+        LCN
 *     (X)-->|A         |        +---------------------------------------------+
 *           |   locN  C|-->(Z)  | locN: {in: {(X, A), (Y, B)}, out: {(C, Z)}} |
 *     (Y)-->|B         |        +---------------------------------------------+
 *           +----------+
 *
 * Where locN is a unique identifier for the pb primitive that the netlist atom
 * was mapped to, and X, Y, and Z represent connected pins belonging to netlist
 * atoms that were previously added to this cluster. Note that the pb primitive
 * at locN may have more pins than those shown here, but only pins connected to
 * previously placed clusters are included in the LCN.
 *
 * External Connectivity Node (ECN)
 * --------------------------------
 * Not shown in the examples above is that a packing signature must also
 * describe signals entering and exiting the cluster. At any given packing step,
 * it is impossible to know whether a connected atom outside of the cluster
 * might be added to the cluster in a later packing step, making a currently
 * external net entirely internal. Thus, in order to avoid needing to backtrack
 * when forming a packing signature, external connections are only described in
 * the terminating node of the signature (the ECN):
 *
 *     --+->[A loc0 B]------------->  ECN
 *       |                            +-------------------------+
 *       +->[C loc1 D]->[E loc2 F]->  | incoming: {{A, C}, {G}} |
 *                                    | outgoing: {B, F, H}     |
 *     ---->[G loc0 H]------------->  +-------------------------+
 *
 * The incoming signals are described as a set of sets where inner-sets contain
 * sink pins that share an external source (e.g. pins A and C, whereas G has
 * its own source). The outgoing signals are a list of pins that drive an
 * external sink.
 *
 * ECNs have a higher memory cost than LCNs, so they are only created when
 * there is a value to be saved for lookup (i.e. when a routing legality
 * check has been run).
 *
 * Packing Signature Tree (PST)
 * ----------------------------
 * A search tree comprised of LCNs and ECNs:
 *
 *     PST:                  LCNs and ECNs:
 *              [ROOT]       +-----------------------------------------+
 *               |  |        | LCN: { locN, {in}, {out} }              |
 *           [LCN]  [LCN]    | ECN: { legal?, {incoming}, {outgoing} } |
 *            | |      |     +-----------------------------------------+
 *        [LCN] [ECN]  [LCN]
 *         |            | |
 *     [ECN]        [ECN] [ECN]
 *
 * Any path from the ROOT to an ECN is a unique cluster packing signature that
 * has been tested for legality previously. During the packing of a new cluster,
 * a cursor begins at the root of the PST and tracks the LCN corresponding to
 * the atom most recently added to the cluster. If an LCN for a new atom is not
 * already present from a previous cluster, then a new one is created and added
 * as a child to the LCN at the cursor. In addition to the cursor, the
 * PackingSignatureTree class also tracks the state of external connections as
 * atoms get added to the cluster, so that it can always generate an accurate
 * ECN for the active packing signature.
 *
 * When the packer needs to know whether a legal routing exists for the packing
 * pattern it has created, it will first query the PST to generate an ECN that
 * gets compared with the list of child ECNs for the LCN at the cursor. If a
 * match is found, then the PST will report the previously determined legality
 * status found in the ECN. Otherwise, the packer will run the usual routing
 * legality check, and a new ECN containing the computed legality will be added
 * to the PST.
 *
 * When the packer inserts a molecule into a cluster that makes routing
 * impossible, the PST must add nodes to encode this illegal packing pattern,
 * and subsequently rollback its state to before the molecule was added (to
 * match the packer which unclusters the molecule). To facilitate this, the PST
 * will set "checkpoints" before it adds any atoms belonging to a new molecule.
 * The checkpoint saves the cursor to checkpoint_cursor_, as well as clears the
 * other checkpoint_* structures that record changes made to the structures
 * that track external connectivity state. When a rollback occurs, the cursor
 * is restored to the checkpoint, and modifications to external connectivity
 * structures are undone.
 *
 * Because routing gets skipped for familiar packing signatures when the PST is
 * enabled, the packer must keep track of whether its cluster routing structures
 * are up-to-date before committing a final cluster. It must make sure routing
 * is run at least once for clusters with repeat final packing signatures.
 *
 * General Usage
 * =============
 * This section provides pseudo-code demonstrating the context where PST
 * methods should be called during the packing flow for correct behaviour.
 *
 * New Cluster
 * -----------
 * start_packing_signature must be run after a new cluster is opened for packing:
 *
 *     PackMoleculeId seed = select_seed_molecule(netlist);
 *     t_logical_block_type* cluster_logical_block_type = start_cluster(seed);
 *     pst.start_packing_signature(cluster_logical_block_type);
 *
 * Molecule Packing
 * ----------------
 * The general steps for updating the PST while packing a molecule are as follows:
 *
 *     // Insert atoms from molecule into cluster
 *     atom_pb_mappings = cluster.pack_atoms(molecule);
 *
 *     // Save state before adding nodes to PST in case of rollback
 *     pst.set_checkpoint();
 *
 *     // Add LCNs to PST for each atom in molecule
 *     for (AtomBlockId atom : molecule) {
 *         pst.add_lcn(atom_pb_mappings(atom), atom);
 *     }
 *
 *     // Set fallthrough value so the packer knows the cluster is not routed
 *     // if the routing legalization step gets skipped.
 *     routed = false;
 *
 *     // Query PST to see if the legality for this packing pattern is known
 *     e_ecn_legality legality = pst.check_legality();
 *
 *     // If this is a new packing pattern, routing legality check must be run
 *     if (legality == e_ecn_legality::UNKNOWN) {
 *         routed = cluster.run_routing_legality_check();
 *         legality = (routed) ? e_ecn_legality::LEGAL : e_ecn_legality::ILLEGAL;
 *
 *         // Create an ECN and store the legality check result
 *         pst.add_ecn(legality);
 *     }
 *
 *     if (legality == e_ecn_legality::LEGAL) {
 *         cluster.commit_molecule(molecule);
 *     } else {
 *         cluster.remove_molecule(molecule);
 *
 *         // Rollback to state before the illegal molecule was added
 *         pst.rollback_to_checkpoint();
 *     }
 */

#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "atom_netlist.h"
#include "cluster_legalizer_fwd.h"
#include "physical_types.h"
#include "vtr_strong_id.h"

/// @brief Type alias for primitive_num from t_pb_graph_node.
typedef int t_primitive_num;

/// @brief Unique cluster-relative pin identifier assembled from accessible VPR
///        types.
///
/// The packing signature tree requires cluster-relative pin IDs (i.e. IDs that
/// represent architectural pins present in a pb, independent of the location
/// of a specific instance of that pb), but the VPR packing datatypes only
/// implement global pin identifiers. As a workaround: the primitive_num (unique
/// number encoding the location of an architecture primitive within a pb),
/// port name, and bit index of a pin uniquely identify that pin for any cluster.
///
/// This is quite a large tuple (>32 bytes), and pin identifier copies make up
/// most of the packing signature tree's memory footprint. To save on memory
/// this type gets computed and then used as a key to look up a derived 4-byte
/// pin ID (PstPinPd) that gets stored in the tree instead.
typedef std::tuple<t_primitive_num, std::string, BitIndex> PstPin;

/// @brief Hash function definition for PstPin.
struct PstPinHash {
    size_t operator()(PstPin pin) const noexcept {
        size_t hash = 0;
        vtr::hash_combine(hash, std::get<0>(pin));
        vtr::hash_combine(hash, std::get<1>(pin));
        vtr::hash_combine(hash, static_cast<int>(std::get<2>(pin)));
        return hash;
    }
};

/// @brief Derived 4-byte cluster-relative pin identifier with a unique
///        correspondence to a PstPin tuple.
typedef vtr::StrongId<struct pst_pin_id_tag> PstPinId;

/// @brief Pair of pin IDs representing a connection between two architecture
///        pins from a pb that are required to be routable for a cluster
///        packing pattern to be legal.
struct PstConnection {
    /// @brief ID of the architectural pin driving the connection.
    PstPinId source_pin;

    /// @brief ID of the architectural pin driven by the connection.
    PstPinId sink_pin;

    bool operator==(PstConnection const& rhs) const {
        return (this->source_pin == rhs.source_pin && this->sink_pin == rhs.sink_pin);
    }

    // Magnitude comparison of connections does not have a literal meaning, but
    // allows for connections to be consistently sorted so that element-wise
    // equality tests can be performed between lists of connections.
    bool operator<(PstConnection const& rhs) const {
        return (this->source_pin != rhs.source_pin) ? this->source_pin < rhs.source_pin : this->sink_pin < rhs.sink_pin;
    }
};

/// @brief Legality status of packing signature that gets stored in the ECN.
///
/// Every full or partial packing signature that has been evaluated for
/// legality is terminated by an ECN that has an e_ecn_legality field of LEGAL
/// or ILLEGAL. UNKNOWN is returned by PackingSignatureTree::check_legality()
/// for unrecognised cluster states.
enum e_ecn_legality {
    UNKNOWN,
    LEGAL,
    ILLEGAL
};

/// @brief External Connectivity Nodes (ECNs) are the leaf node type for
///        PackingSignatureTree. They encode pb pins that have signals
///        entering/exiting the cluster, as well as the legality of the
///        packing signature that this node terminates.
struct ExternalConnectivityNode {
    /// @brief IDs of architectural pins that are driven by a source outside
    ///        the cluster.
    ///
    /// Pins that share a source external to the cluster are grouped together.
    std::vector<std::vector<PstPinId>> cluster_inputs;

    /// @brief IDs of architectural pins that drive one or more sinks outside
    ///        the cluster.
    std::vector<PstPinId> cluster_outputs;

    /// @brief Legality of the packing signature that this node terminates
    ///        (whether a routing solution exists for this configuration).
    e_ecn_legality legality;

    ExternalConnectivityNode(e_ecn_legality legal)
        : legality(legal) {}

    // Equality test ignores the legality field.
    bool operator==(ExternalConnectivityNode const& rhs) const {
        if (this->cluster_inputs != rhs.cluster_inputs) return false;
        return (this->cluster_outputs == rhs.cluster_outputs);
    }
};

/// @brief Location and Connectivity nodes (LCNs) are the primary node type
///        for PackingSignatureTree. They encode cluster-relative atom
///        placement locations and connections between the atom represented
///        by this node with other atoms that have been packed previously in
///        the current cluster.
struct LocationAndConnectivityNode {
    /// @brief Unique identifier of the pb primitive location where the atom
    ///        represented by this node was placed.
    t_primitive_num primitive_num;

    /// @brief List of input connections to the atom represented by this node
    ///        that are driven by atoms that were packed previous to this one.
    std::vector<PstConnection> intracluster_sources_to_primitive_inputs;

    /// @brief List of output connections from the atom represented by this node
    ///        that drive atoms that were packed previous to this one.
    std::vector<PstConnection> intracluster_sinks_of_primitive_outputs;

    /// @brief Child nodes representing atoms packed after the one represented
    ///        by this node.
    std::vector<LocationAndConnectivityNode*> child_lcn;

    /// @brief Child leaf nodes that encode distinct external connection
    ///        variations for the packing signature with internal connections
    ///        described by the LCNs up to and including this one.
    std::vector<ExternalConnectivityNode*> child_ecn;

    LocationAndConnectivityNode()
        // primitive_nums are assigned contiguously starting from 0, so -1 is
        // used as a sentinel value.
        : primitive_num(-1) {}

    ~LocationAndConnectivityNode() {
        for (auto lcn : child_lcn) {
            delete lcn;
        }
        for (auto ecn : child_ecn) {
            delete ecn;
        }
    }

    bool operator==(LocationAndConnectivityNode const& rhs) const {
        if (this->primitive_num != rhs.primitive_num) return false;
        if (this->intracluster_sources_to_primitive_inputs != rhs.intracluster_sources_to_primitive_inputs) return false;
        if (this->intracluster_sinks_of_primitive_outputs != rhs.intracluster_sinks_of_primitive_outputs) return false;
        return true;
    }
};

/// @brief A data structure for tracking and recognizing cluster packing
///        patterns that have been seen previously during the packing stage.
///        This enables the memoization of redundant legality checks done
///        during detailed packing which can dominate VPR runtime for complex
///        logic block architectures.
class PackingSignatureTree {
  public:
    PackingSignatureTree()
        : cursor_(nullptr)
        , checkpoint_cursor_(nullptr) {}

    ~PackingSignatureTree() {
        for (auto lcn : branches_) {
            delete lcn;
        }
    }

    /// @brief Begin a new packing signature for the provided logical block type.
    ///
    /// A separate PST branch is created for each logical block type.
    ///
    ///   @param cluster_logical_block_type The logical block type of the new cluster
    void start_packing_signature(const t_logical_block_type* cluster_logical_block_type);

    /// @brief Save current bookkeeping state in case rollback needs to occur.
    void set_checkpoint();

    /// @brief Reset bookkeeping state to checkpoint.
    void rollback_to_checkpoint();

    /// @brief Record the location and connections of a packed atom in the PST.
    ///
    ///   @param primitive_pb_graph_node The architectural pb primitive node
    ///                                  that the atom was packed to
    ///   @param atom_block_id           The netlist atom that was packed
    void add_lcn(const t_pb_graph_node* primitive_pb_graph_node, const AtomBlockId atom_block_id);

    /// @brief Record the external connections and legality outcome of a
    ///        cluster packing pattern in the PST.
    ///
    ///   @param legality The computed legality of the packing signature
    ///                   that is terminated by this ECN
    void add_ecn(e_ecn_legality legality);

    /// @brief Get legality status of current cluster packing pattern, if known.
    e_ecn_legality check_legality();

  private:
    /// @brief Generate an LCN from the current PST state and provided arguments.
    ///
    ///   @param primitive_pb_graph_node The architectural pb primitive node
    ///                                  that the atom was packed to
    ///   @param atom_block_id           The netlist atom that was packed
    LocationAndConnectivityNode* create_lcn(const t_pb_graph_node* primitive_pb_graph_node, const AtomBlockId atom_block_id);

    /// @brief Generate an ECN from the current PST state and record its legality,
    ///        if provided.
    ///
    ///   @param ecn Pointer to the new ECN instance that will be populated with
    ///              external connectivity state for the current cluster
    void populate_ecn(ExternalConnectivityNode* ecn);

  private:
    /// @brief Main branches of the PST.
    ///
    /// Each logical block type gets its own branch, where the logical block
    /// type corresponding to each index can be found at the same index in
    /// branch_logical_block_types_.
    std::vector<LocationAndConnectivityNode*> branches_;

    /// @brief List of logical block types where the indices correspond to the
    ///        respective branch in branches_ for that logical block type.
    std::vector<const t_logical_block_type*> branch_logical_block_types_;

    /// @brief Cursor to most recent node for the signature being constructed.
    LocationAndConnectivityNode* cursor_;

    /// @brief Mappings from atom IDs of primitives that have been added to
    ///        the open cluster to their primitive_num.
    std::unordered_map<AtomBlockId, t_primitive_num> packed_atoms_;

    /// @brief Lookup table for 4-byte PstPinId keyed by >32-byte PstPin tuple.
    ///
    /// This is to reduce the memory footprint of encoding pins in the PST.
    std::unordered_map<PstPin, PstPinId, PstPinHash> pin_mappings_;

    /// @brief Find (or create) and return PstPinId for provided PstPin.
    inline PstPinId get_pin_mapping(PstPin pin) {
        auto got = pin_mappings_.find(pin);
        if (got == pin_mappings_.end()) {
            PstPinId pin_id = PstPinId(pin_mappings_.size());
            pin_mappings_[pin] = pin_id;
            return pin_id;
        }
        return got->second;
    }

    /// @brief Tracks current number of external sinks for a given source pin.
    struct ExternalSinksRecord {
        /// @brief Source pin belonging to an atom that has been packed for
        ///        which the number of external sinks is beting tracked.
        PstPinId source_pin;

        /// @brief Count of external sinks belonging to source_pin. If this
        ///        count reaches zero, then source_pin has no external sinks
        ///        and should be omitted from ECNs.
        size_t external_sinks_count;
    };

    /// @brief Bookkeeping of all nets that drive pins within the open cluster
    ///        as well as the pins that the nets drive. Used for generating
    ///        the cluster_inputs field in ECNs.
    ///
    /// If a net in this map is also present in output_nets_, then it is known
    /// to be driven internally and is excluded from generated ECNs.
    std::unordered_map<AtomNetId, std::vector<PstPinId>> input_nets_;

    /// @brief Bookkeeping of all nets that are driven by pins within the open
    ///        cluster as well as a count of sinks that the net drives outside
    ///        of the cluster.
    ///
    /// The external sinks count gets initialized to the total number of sinks
    /// driven by this net and decremented for every pin this net drives inside
    /// the cluster. If this count is zero when an ECN is generated, then this
    /// net has no external sinks and is excluded from the ECN.
    std::unordered_map<AtomNetId, ExternalSinksRecord> output_nets_;

    /// @brief Value of cursor_ for most recent checkpoint.
    LocationAndConnectivityNode* checkpoint_cursor_;

    /// @brief Atoms added to the cluster since the most recent checkpoint.
    std::vector<AtomBlockId> checkpoint_new_atoms_;

    /// @brief Nets added to input_nets_ since the most recent checkpoint.
    std::unordered_map<AtomNetId, size_t> checkpoint_input_nets_;

    /// @brief Nets added to output_nets_ since the most recent checkpoint.
    std::vector<AtomNetId> checkpoint_new_output_nets_;

    /// @brief Changes made to external sink counts in output_nets_ since the
    ///        most recent checkpoint.
    std::unordered_map<AtomNetId, size_t> checkpoint_decremented_output_nets_;
};
