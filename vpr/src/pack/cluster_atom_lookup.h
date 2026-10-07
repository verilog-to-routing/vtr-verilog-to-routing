#pragma once
/**
 * @file
 * @brief Sparse atom to primitive mappings owned by an individual cluster.
 */
#include <unordered_map>
#include "atom_netlist_fwd.h"
#include "vtr_bimap.h"
#include "vpr_types.h"

/** @brief Cluster-local bimap whose storage scales with the number of placed atoms. */
class ClusterAtomPBBimap {
  public:
    /** @brief Look up a locally placed primitive, or null for an external atom. */
    const t_pb* get_atom_pb(AtomBlockId atom) const {
        auto it = atom_to_pb_.find(atom);
        return it == atom_to_pb_.end() ? nullptr : it->second;
    }

    /** @brief Look up the atom occupying a primitive, or invalid for an empty pb. */
    AtomBlockId get_pb_atom(const t_pb* pb) const {
        auto it = atom_to_pb_.find(pb);
        return it == atom_to_pb_.inverse_end() ? AtomBlockId::INVALID() : it->second;
    }

    /** @brief Look up the graph node of a locally placed atom. */
    const t_pb_graph_node* get_atom_pb_graph_node(AtomBlockId atom) const {
        const t_pb* pb = get_atom_pb(atom);
        return pb ? pb->pb_graph_node : nullptr;
    }

    /** @brief Record an atom's primitive placement in both mapping directions. */
    void update(AtomBlockId atom, const t_pb* pb) {
        atom_to_pb_.update(atom, pb);
    }

    /** @brief Remove an atom and its inverse primitive mapping. */
    void erase(AtomBlockId atom) {
        atom_to_pb_.erase(atom);
    }

    /** @brief Remove a primitive and its inverse atom mapping. */
    void erase(const t_pb* pb) {
        atom_to_pb_.erase(pb);
    }

  private:
    vtr::bimap<AtomBlockId, const t_pb*, std::unordered_map, std::unordered_map> atom_to_pb_; ///< Sparse bidirectional placement mapping.
};
