/*
 * Copyright 2022 CAS—Atlantic (University of New Brunswick, CASA)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _HARD_SOFT_LOGIC_MIXER_HPP_
#define _HARD_SOFT_LOGIC_MIXER_HPP_

#include "mixing_optimization.h"
#include "odin_types.h" // netlist_t, config_t

class HardSoftLogicMixer
{
  public:
    HardSoftLogicMixer();
    ~HardSoftLogicMixer();
    /*----------------------------------------------------------------------
     * Returns whether the specific node is a candidate for implementing
     * in hard block
     *---------------------------------------------------------------------
     */
    bool hardenable(nnode_t *node);

    /*----------------------------------------------------------------------
     * Function: map_deferred_blocksQueries if mixing optimization is enabled for this kind of
     * of hard blocks
     *---------------------------------------------------------------------
     */
    bool enabled(nnode_t *node);

    /*----------------------------------------------------------------------
     * Function: perform_optimizations
     * For all  noted nodes, that were noted as candidates to be implemented
     * on the hard blocks, launches corresponding procedure of chosing the
     * corresponding blocks
     * Parameters: netlist_t *
     *---------------------------------------------------------------------
     */
    void perform_optimizations(netlist_t *netlist);

    /*----------------------------------------------------------------------
     * Function: partial_map_node
     * High-level call to provide support for partial mapping layer
     * Parameters:
     *      node_t * : pointer to node needs to perform mapping
     *      netlist_t : pointer to netlist
     *---------------------------------------------------------------------
     */
    void partial_map_node(nnode_t *node, short traverse_number, netlist_t *);

    /*----------------------------------------------------------------------
     * Function: note_candidate_node
     * Calculates number of available hard blocks by issuing a call,
     * traverses the netlist and statistics to figure out
     * which operation should be implemented on the hard block
     * Parameters:
     *      node_t * : pointer to candidate node
     *---------------------------------------------------------------------
     */
    void note_candidate_node(nnode_t *node);

    // This is a container containing all optimization passes
    MixingOpt *_opts[operation_list_END];

  private:
    /*----------------------------------------------------------------------
     * Function: hard_blocks_needed
     * Returns cached value calculated from netlist, for a specific optimiza
     * tion kind
     *---------------------------------------------------------------------
     */
    int hard_blocks_needed(operation_list);

    // This array is composed of vectors, that store nodes that
    // are potential candidates for performing mixing optimization
    std::vector<nnode_t *> _nodes_by_opt[operation_list_END];
};

#endif
