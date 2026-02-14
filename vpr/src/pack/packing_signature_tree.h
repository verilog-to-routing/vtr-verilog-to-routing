#pragma once
/**
 * @file
 * @author  Milo Liebster
 * @date    January 2026
 * @brief   The declaration of the PackingSignatureTree class.
 *
 * A structure for memoizing the results of legality checks during
 * packing to significantly improve packing runtimes for complex
 * logic block architectures.
 */

#include <map>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "atom_netlist.h"
#include "physical_types.h"
#include "vtr_strong_id.h"

// Define to avoid circular dependancy from including cluster_legalizer.h
typedef vtr::StrongId<struct legalization_cluster_id_tag, size_t> LegalizationClusterId;

/// @brief Type alias for primitive_num from t_pb_graph_node.
typedef int t_primitive_num;

/// @brief Unique cluster-relative pin identifier assembled from accessible VPR
///        types.
///
/// The packing signature tree requires cluster-relative pin IDs, but the VPR
/// packing datatypes only implement global pin identifiers. As a workaround:
/// the primitive_num, port name, and bit index of a pin uniquely identify
/// that pin for any cluster.
///
/// This is quite a large tuple (>32 bytes), and pin identifier copies make up
/// most of the packing signature tree's memory footprint. To save on memory
/// this type gets computed and then used as a key to look up a derived 4-byte
/// pin ID (PstPinPd) that gets stored in the tree instead.
typedef std::tuple<t_primitive_num, std::string, BitIndex> PstPin;

/// @brief Hash function definition for PstPin.
struct PstPinHash {
    size_t operator()(PstPin pin) const noexcept {
        return std::hash<t_primitive_num>{}(std::get<0>(pin)) ^ std::hash<std::string>{}(std::get<1>(pin)) ^ std::hash<int>{}((int)std::get<2>(pin));
    }
};

/// @brief Derived 4-byte cluster-relative pin identifier.
typedef vtr::StrongId<struct pst_pin_id_tag> PstPinId;

/// @brief Pair of pins representing a connection in the packing signature tree.
struct PstConnection {
    PstPinId source;
    PstPinId sink;

    bool operator==(PstConnection const& rhs) const {
        return (this->source == rhs.source && this->sink == rhs.sink);
    }

    // Magnitude comparison of connections does not have a literal meaning, but
    // allows for connections to be consistently sorted so that element-wise
    // equality tests can be performed between lists of connections.
    bool operator<(PstConnection const& rhs) const {
        return (this->source != rhs.source) ? this->source < rhs.source : this->sink < rhs.sink;
    }
};

/// @brief Legality status of packing signature that gets stored in the
///        external connectivity nodes.
///
/// Every full or partial packing signature that has been evaluated for
/// legality is terminated by an external connectivity node (ECN) that has an
/// e_ecn_legality field of LEGAL or ILLEGAL. UNKNOWN is returned by
/// PackingSignatureTree::check_legality() for unrecognised cluster states.
enum e_ecn_legality {
    UNKNOWN,
    LEGAL,
    ILLEGAL
};

/// @brief Leaf node type for PackingSignatureTree. Encodes pins that have
///        signals entering/exiting the cluster, as well as the legality of the
///        packing signature that this node terminates.
struct ExternalConnectivityNode {
    /// @brief Pins that are driven by a source outside the cluster.
    ///
    /// Pins that share a source external to the cluster are grouped together.
    std::vector<std::vector<PstPinId>> cluster_inputs;

    /// @brief Pins that drive one or more sinks outside the cluster.
    std::vector<PstPinId> cluster_outputs;

    /// @brief Legality of the packing signature that this node terminates
    ///        (whether a routing solution exist for this configuration).
    e_ecn_legality legality;

    ExternalConnectivityNode(e_ecn_legality legal)
        : legality(legal) {}

    // Equality test ignores the legality field.
    bool operator==(ExternalConnectivityNode const& rhs) const {
        if (this->cluster_inputs != rhs.cluster_inputs) return false;
        return (this->cluster_outputs == rhs.cluster_outputs);
    }
};

/// @brief Primary node type for PackingSignatureTree. Encodes cluster-relative
///        atom placement locations and connections between the atom
///        represented by this node with other atoms that have been packed up
///        until this point in the signature.
struct LocationAndConnectivityNode {
    /// @brief Unique identifier of the cluster-relative location where the
    ///        atom represented by this node was placed.
    t_primitive_num primitive_num;

    /// @brief List of input connections to the atom represented by this node
    ///        that are driven by atoms that have been packed up until this
    ///        point.
    std::vector<PstConnection> intracluster_sources_to_primitive_inputs;

    /// @brief List of output connections from the atom represented by this node
    ///        that drive atoms that have been packed up until this point.
    std::vector<PstConnection> intracluster_sinks_of_primitive_outputs;

    /// @brief Child nodes representing atoms packed after the one represented
    ///        by this node.
    std::vector<LocationAndConnectivityNode*> child_lcn;

    /// @brief Child leaf nodes that encode distinct external connection
    ///        variations for the packing signature that is described by the
    ///        LCNs that have been traversed (from the root, up to and including
    ///        this one).
    std::vector<ExternalConnectivityNode*> child_ecn;

    LocationAndConnectivityNode()
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
///
/// Theory:
/// Because the VPR packer is greedy and deterministic, repeated structures that
/// show up in circuits will tend to be processed identically by the packer and
/// produce matching clusters. Tracking seen cluster patterns in a decision tree
/// lets us memoize and reuse cluster legalization outcomes.
///
/// The PackingSignatureTree (PST) is made up of LocationAndConnectivityNodes
/// (LCNs) and ExternalConnectivityNodes (ECNs), as shown here:
///
///   PST:                  Contents of LCNs and ECNs:
///            [ROOT]       *--------------------------------------------*
///             |  |        | LCN: { atom_loc, {sources}, {sinks} }      |
///         [LCN]  [LCN]    | ECN: { legal, {ext_sources}, {ext_sinks} } |
///          | |      |     *--------------------------------------------*
///      [LCN] [ECN]  [LCN]
///       |            | |
///   [ECN]        [ECN] [ECN]
///
/// Any path from the ROOT to an ECN corresponds to a unique cluster packing
/// pattern (packing signature) that has been tested for legality previously.
/// If a series of identical packing decisions are made to a previously packed
/// cluster, then the tree will be traversed from the root to the same ECN as
/// before; in this iteration, only a final routing needs to be performed since
/// any intermediate legality check results have been precomputed and are
/// available in the ECNs that are encountered while constructing the cluster.
/// Each logical block type gets an isolated branch in PackingSignatureTree.
///
/// LCNs include the cluster-relative location of an atom that has been packed
/// as well as PstConnections that describe incoming/outgoing connections to
/// other atoms that have already been packed in this cluster. Because it is
/// not certain whether external connections might become internal as more
/// atoms are added to the cluster, the PackingSignatureTree class does extra
/// bookkeeping for what external connections are made. When a current cluster
/// configuration needs to be fully defined so that a legality outcome can be
/// recorded, an ECN is generated to describe all the connections entering and
/// exiting the cluster. The result of the legality check also gets stored in
/// the ECN.
///
/// Although an LCN is made for each atom added to a cluster, ECNs are only
/// made when the packer performs a legality check. During speculative packing,
/// this is only once the cluster is finalized; for detailed packing, this
/// means there is one ECN for the last atom of every molecule inserted. The
/// PackingSignatureTree class also records what changes have been made to its
/// external connectivity bookkeeping structures since the previous legal
/// molecule was inserted so that it can rollback the state of its bookkeeping
/// if the current molecule is illegal and needs to be unpacked. The LCN(s)
/// and ECN added for the illegal molecule stay in the PST so that future
/// clusters can recognize the illegal state and skip performing legalization
/// by routing.
class PackingSignatureTree {
  public:
    PackingSignatureTree() = delete;
    PackingSignatureTree(bool enabled)
        : routed(false)
        , enabled_(enabled)
        , at_lcn_(nullptr) {}

    ~PackingSignatureTree() {
        for (auto lcn : branches_) {
            delete lcn;
        }
    }

    /// @brief Begin a new packing signature for the provided logical block type.
    void start_packing_signature(const t_logical_block_type* cluster_logical_block_type);

    /// @brief Save current bookkeeping state in case rollback needs to occur.
    void set_checkpoint();

    /// @brief Reset bookkeeping state to checkpoint.
    void rollback_to_checkpoint();

    /// @brief Record the location and connections of a packed atom in the PST.
    void add_lcn(const t_pb_graph_node* primitive_pb_graph_node, const AtomBlockId atom_block_id);

    /// @brief Record the external connections and legality outcome of a
    ///        cluster packing pattern in the PST.
    void add_ecn(e_ecn_legality legality);

    /// @brief Get legality status of current cluster packing pattern, if known.
    e_ecn_legality check_legality();

  private:
    /// @brief Generate an LCN from the current PST state and provided arguments.
    LocationAndConnectivityNode* create_lcn(const t_pb_graph_node* primitive_pb_graph_node, const AtomBlockId atom_block_id);

    /// @brief Generate an ECN from the current PST state and record its legality,
    ///        if provided.
    ExternalConnectivityNode* create_ecn(e_ecn_legality legality = e_ecn_legality::UNKNOWN);

  public:
    /// @brief Used for the packer to track whether it has a valid routing for
    ///        a finalized cluster.
    ///
    /// With the PST, it is possible for the packer to reach a final cluster
    /// packing without ever routing the cluster. If this value is false when
    /// the packer is finalizing a cluster, it will run the cluster router so
    /// that a routing exists for later VPR stages.
    bool routed;

  private:
    /// @brief If false, then class methods exit immediately and no PST is
    ///        constructed.
    const bool enabled_;

    /// @brief Main branches of the PST.
    ///
    /// Each logical block type gets its own branch, where the logical block
    /// type corresponding to each index can be found at the same index in
    /// branch_logical_block_types_.
    std::vector<LocationAndConnectivityNode*> branches_;

    /// @brief List of logical block types where the indices correspond to the
    ///        respective branch in branches_ for that logical block type.
    std::vector<const t_logical_block_type*> branch_logical_block_types_;

    /// @brief Current active node for the signature being constructed.
    LocationAndConnectivityNode* at_lcn_;

    /// @brief Mappings from atom IDs of primitives that have been added to
    ///        the open cluster to their primitive_num.
    std::map<AtomBlockId, t_primitive_num> packed_atoms_;

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
        PstPinId pin;
        size_t external_sinks_count;
    };

    /// @brief Bookkeeping of all nets that drive pins within the open cluster
    ///        as well as the pins that the nets drive. Used for generating
    ///        the cluster_inputs field in ECNs.
    ///
    /// If a net in this map is also present in output_nets_, then it is known
    /// to be driven internally and is excluded from generated ECNs.
    std::map<AtomNetId, std::vector<PstPinId>> input_nets_;

    /// @brief Bookkeeping of all nets that are driven by pins within the open
    ///        cluster as well as a count of sinks that the net drives outside
    ///        of the cluster.
    ///
    /// The external sinks count gets initialized to the total number of sinks
    /// driven by this net and decremented for every pin this net drives inside
    /// the cluster. If this count is zero when an ECN is generated, then this
    /// net has no external sinks and is excluded from the ECN.
    std::map<AtomNetId, ExternalSinksRecord> output_nets_;

    /// @brief Value of at_lcn_ for most recent checkpoint.
    LocationAndConnectivityNode* checkpoint_lcn_;

    /// @brief Atoms added to the cluster since the most recent checkpoint.
    std::vector<AtomBlockId> checkpoint_new_atoms_;

    /// @brief Nets added to input_nets_ since the most recent checkpoint.
    std::map<AtomNetId, size_t> checkpoint_input_nets_;

    /// @brief Nets added to output_nets_ since the most recent checkpoint.
    std::vector<AtomNetId> checkpoint_new_output_nets_;

    /// @brief Changes made to external sink counts in output_nets_ since the
    ///        most recent checkpoint.
    std::map<AtomNetId, size_t> checkpoint_decremented_output_nets_;
};
