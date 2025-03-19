/**
 * @file 
 * @author  Amir Poolad
 * @date    March 2025
 * @brief   The declaration of the AtomPBBimap class.
 * 
 * This file declares a class called AtomPBBimap that
 * contains a two way mapping between AtomBlockIds and pb types.
 */

#pragma once

#include "vpr_types.h"

// Forward declaration
class t_pb_graph_node;

/**
 * @brief Class that holds a bimap between atoms and pb types.
 *        This means that you can get a pb from an atom and the
 *        other way around.
 *        
 *        Used in the global AtomLookup context and in ClusterLegalizer
 */
class AtomPBBimap {
    public:
    AtomPBBimap() = default;
    AtomPBBimap(const vtr::bimap<AtomBlockId, const t_pb*, vtr::linear_map, std::unordered_map> &atom_to_pb);


    /**
     * @brief Returns the leaf pb associated with the atom blk_id
     * @note  this is the lowest level pb which corresponds directly to the atom block
     */
    const t_pb* atom_pb(const AtomBlockId blk_id) const;

    ///@brief Returns the atom block id associated with pb
    AtomBlockId pb_atom(const t_pb* pb) const;

    ///@brief Conveneince wrapper around atom_pb to access the associated graph node
    const t_pb_graph_node* atom_pb_graph_node(const AtomBlockId blk_id) const;

    /**
     * @brief Sets the bidirectional mapping between an atom and pb
     *
     * If either blk_id or pb are not valid any, existing mapping is removed
     */
    void set_atom_pb(const AtomBlockId blk_id, const t_pb* pb);

    /// @brief Sets the pb for all blocks in the netlist to nullptr.
    void reset_bimap();

    /// @brief Returns if the bimap is empty
    bool is_empty() const;

    private:
        /// @brief Two way map between AtomBlockIds and t_pb
        vtr::bimap<AtomBlockId, const t_pb*, vtr::linear_map, std::unordered_map> atom_to_pb_;
    };
