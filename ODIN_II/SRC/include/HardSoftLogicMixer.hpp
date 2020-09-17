/*
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef HARD_SOFT_LOGIC_MIXER_HPP
#define HARD_SOFT_LOGIC_MIXER_HPP

#include "odin_types.h" // netlist_t, config_t
#include "MixingOptimization.hpp"

class HardSoftLogicMixer {
  public:
    HardSoftLogicMixer();
    ~HardSoftLogicMixer();
    /*----------------------------------------------------------------------
     * Returns whether the specific node is a candidate for implementing
     * in hard block
     *---------------------------------------------------------------------
     */
    bool hardenable(nnode_t* node);

    /*----------------------------------------------------------------------
     * Function: map_deferred_blocksQueries if mixing optimization is enabled for this kind of
     * of hard blocks
     *---------------------------------------------------------------------
     */
    bool enabled(nnode_t* node);

    /*----------------------------------------------------------------------
     * Function: perform_optimizations 
     * For all  noted nodes, that were noted as candidates to be implemented 
     * on the hard blocks, launches corresponding procedure of chosing the
     * corresponding blocks
     * Parameters: netlist_t *
     *---------------------------------------------------------------------
     */
    void perform_optimizations(netlist_t* netlist);

    /*----------------------------------------------------------------------
     * Function: partial_map_node
     * High-level call to provide support for partial mapping layer
     * Parameters: 
     *      node_t * : pointer to node needs to perform mapping
     *      netlist_t : pointer to netlist
     *---------------------------------------------------------------------
     */
    void partial_map_node(nnode_t* node, short traverse_number, netlist_t*);

    /*----------------------------------------------------------------------
     * Function: note_candidate_node
     * Calculates number of available hard blocks by issuing a call,
     * traverses the netlist and statistics to figure out
     * which operation should be implemented on the hard block
     * Parameters: 
     *      node_t * : pointer to candidate node
     *---------------------------------------------------------------------
     */
    void note_candidate_node(nnode_t* node);

    // This is a container containing all optimization passes
    MixingOpt* _opts[operation_list_END];

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
    std::vector<nnode_t*> _nodes_by_opt[operation_list_END];
};

#endif
