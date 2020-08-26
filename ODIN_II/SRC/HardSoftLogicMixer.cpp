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
#include "HardSoftLogicMixer.hpp"

#include <stdint.h> // INT_MAX
#include <vector>

#include "multipliers.h" // instantiate_simple_soft_multiplier
#include "odin_error.h"  // error_message

HardSoftLogicMixer::HardSoftLogicMixer() {
    for (int i = 0; i < operation_list_END; i++) {
        if (i == MULTIPLY) {
            this->_opts[i] = new MultsOpt();
        } else {
            this->_opts[i] = new MixingOpt();
        }
    }
}

HardSoftLogicMixer::~HardSoftLogicMixer() {
    for (int i = 0; i < operation_list_END; i++) {
        delete this->_opts[i];
    }
}
void HardSoftLogicMixer::note_candidate_node(nnode_t* opNode) {
    _nodes_by_opt[opNode->type].push_back(opNode);
}

bool HardSoftLogicMixer::hardenable(nnode_t* node) {
    return this->_opts[node->type]->hardenable(node);
}

bool HardSoftLogicMixer::enabled(nnode_t* node) {
    return this->_opts[node->type]->enabled();
}

int HardSoftLogicMixer::hard_blocks_needed(operation_list opt) {
    return _nodes_by_opt[opt].size();
}

void HardSoftLogicMixer::partial_map_node(nnode_t* node, short traverse_number, netlist_t* netlist) {
    _opts[node->type]->partial_map_node(node, traverse_number, netlist, this);
}

void HardSoftLogicMixer::perform_optimizations(netlist_t* netlist) {
    if (_opts[MULTIPLY]->enabled()) {
        int blocks_needed = this->hard_blocks_needed(MULTIPLY);
        _opts[MULTIPLY]->set_blocks_needed(blocks_needed);
        _opts[MULTIPLY]->assign_weights(netlist, _nodes_by_opt[MULTIPLY]);
        _opts[MULTIPLY]->perform(netlist, _nodes_by_opt[MULTIPLY]);
        _opts[MULTIPLY]->instantiate_soft_logic(netlist, _nodes_by_opt[MULTIPLY]);
    }
}
