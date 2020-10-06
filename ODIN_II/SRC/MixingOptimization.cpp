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

#include "MixingOptimization.hpp"

#include <stdint.h> // INT_MAX
#include <vector>

#include "netlist_statistic.h"    // mixing_optimization_stats
#include "multipliers.h"          // instantiate_simple_soft_multiplier
#include "odin_error.h"           // error_message
#include "adders.h"               // hard_adders
#include "HardSoftLogicMixer.hpp" // HardSoftLogicMixer

void MixingOpt::scale_counts() {
    if (this->_blocks_count < 0 || this->_blocks_count == INT_MAX || this->_ratio < 0.0 || this->_ratio > 1.0) {
        error_message(NETLIST, unknown_location, "The parameters for optimization kind:%i are configured incorrectly : count %i, ratio %f\n", this->_kind, this->_blocks_count, this->_ratio);
        exit(0);
    }
    this->_blocks_count = this->_blocks_count * this->_ratio;
}

void MixingOpt::assign_weights(netlist_t* /*netlist*/, std::vector<nnode_t*> /*nodes*/) {
    // compute weights for all noted nodes
    error_message(NETLIST, unknown_location, "Assign_weights mixing optimization was called for optimization without specification provided, for kind  %i\n", this->_kind);
    exit(0);
}

void MixingOpt::perform(netlist_t*, std::vector<nnode_t*>&) {
    error_message(NETLIST, unknown_location, "Performing mixing optimization was called for optimization without method provided, for kind  %i\n", this->_kind);
    exit(0);
}

MultsOpt::MultsOpt(int _exact)
    : MixingOpt(1.0, MULTIPLY) {
    this->_blocks_count = _exact;
    this->_enabled = true;
}

MultsOpt::MultsOpt(float ratio)
    : MixingOpt(ratio, MULTIPLY) {
    if (ratio < 0.0 || ratio > 1.0) {
        error_message(NETLIST, unknown_location, "Miltipliers mixing optimization is started with wrong ratio %f\n", ratio);
        exit(0);
    }

    //Explicitly set all hard block multipliers to max
    this->_blocks_count = INT_MAX;
    this->_enabled = true;
}

bool MultsOpt::hardenable(nnode_t* node) {
    int mult_size = std::max<int>(node->input_port_sizes[0], node->input_port_sizes[1]);
    return (hard_multipliers && (mult_size > min_mult));
}

void MultsOpt::assign_weights(netlist_t* netlist, std::vector<nnode_t*> nodes) {
    // compute weights for all noted nodes
    for (size_t i = 0; i < nodes.size(); i++) {
        mixing_optimization_stats(nodes[i], netlist);
    }
}

void MultsOpt::perform(netlist_t* netlist, std::vector<nnode_t*>& weighted_nodes) {
    size_t nodes_count = weighted_nodes.size();

    // per optimization, instantiate hard logic
    for (int i = 0; i < this->_blocks_count; i++) {
        int maximal_cost = -1;
        int index = -1;
        for (size_t j = 0; j < nodes_count; j++) {
            // if found a new maximal cost that is higher than a current maximum AND is not restricted by input
            // params for minimal "hardenable" multiplier width
            if (maximal_cost < weighted_nodes[j]->weight && this->hardenable(weighted_nodes[j])) {
                maximal_cost = weighted_nodes[j]->weight;
                index = j;
            }
        }

        // if there are no suitable nodes left, leave the loop to
        // implement remaining nodes in soft logic
        if (index < 0)
            break;

        // indicate for future iterations the node was hardened
        weighted_nodes[index]->weight = -1;

        if (hard_multipliers) {
            instantiate_hard_multiplier(weighted_nodes[index], this->cached_traverse_value, netlist);
        }
    }

    // From the end of the vector, remove all nodes that were implemented in hard logic. The remaining
    // nodes will be instantiated in soft_map_remaining_nodes
    for (int i = nodes_count - 1; i >= 0; i--) {
        if (weighted_nodes[i]->weight == -1) {
            weighted_nodes.erase(weighted_nodes.begin() + i);
        }
    }
}

void MixingOpt::set_blocks_needed(int new_count) {
    this->_blocks_count = new_count;
}

void MultsOpt::set_blocks_needed(int new_count) {
    // with development for fixed_layout, this value will change
    int availableHardBlocks = INT_MAX;
    int hardBlocksNeeded = new_count;
    int hardBlocksCount = availableHardBlocks;

    if (hardBlocksCount > hardBlocksNeeded) {
        hardBlocksCount = hardBlocksNeeded;
    }

    if (hardBlocksCount < this->_blocks_count) {
        this->_blocks_count = hardBlocksCount;
    }

    this->scale_counts();
}
void MixingOpt::instantiate_soft_logic(netlist_t* /*netlist*/, std::vector<nnode_t*> /* nodes*/) {
    error_message(NETLIST, unknown_location, "Performing instantiate_soft_logic was called for optimization without method provided, for kind  %i\n", this->_kind);
    exit(0);
}

void MixingOpt::partial_map_node(nnode_t* /*node*/, short /*traverse_value*/, netlist_t*, /*netlist*/ HardSoftLogicMixer* /*mixer*/) {
    error_message(NETLIST, unknown_location, "Performing partial_map_node was called for optimization without method provided, for kind  %i\n", this->_kind);
    exit(0);
}

void MultsOpt::partial_map_node(nnode_t* node, short traverse_value, netlist_t* netlist, HardSoftLogicMixer* mixer) {
    if (mixer->enabled(node) && mixer->hardenable(node)) {
        mixer->note_candidate_node(node);
    } else if (mixer->hardenable(node)) {
        instantiate_hard_multiplier(node, traverse_value, netlist);
    } else if (!hard_adders) {
        instantiate_simple_soft_multiplier(node, traverse_value, netlist);
    }
    this->cached_traverse_value = traverse_value;
}

void MultsOpt::instantiate_soft_logic(netlist_t* netlist, std::vector<nnode_t*> nodes) {
    unsigned int size = nodes.size();
    for (unsigned int j = 0; j < size; j++) {
        instantiate_simple_soft_multiplier(nodes[j], this->cached_traverse_value, netlist);
    }
    for (int i = size - 1; i >= 0; i--) {
        nodes[i] = free_nnode(nodes[i]);
        nodes.erase(nodes.begin() + i);
    }
}
