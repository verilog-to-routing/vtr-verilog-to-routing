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
#include "hard_soft_logic_mixer.h"

#include <stdint.h> // INT_MAX
#include <vector>

#include "multiplier.h" // instantiate_simple_soft_multiplier
#include "odin_error.h" // error_message

HardSoftLogicMixer::HardSoftLogicMixer()
{
    for (int i = 0; i < operation_list_END; i++) {
        if (i == MULTIPLY) {
            this->_opts[i] = new MultsOpt();
        } else {
            this->_opts[i] = new MixingOpt();
        }
    }
}

HardSoftLogicMixer::~HardSoftLogicMixer()
{
    for (int i = 0; i < operation_list_END; i++) {
        delete this->_opts[i];
    }
}
void HardSoftLogicMixer::note_candidate_node(nnode_t *opNode) { _nodes_by_opt[opNode->type].push_back(opNode); }

bool HardSoftLogicMixer::hardenable(nnode_t *node) { return this->_opts[node->type]->hardenable(node); }

bool HardSoftLogicMixer::enabled(nnode_t *node) { return this->_opts[node->type]->enabled(); }

int HardSoftLogicMixer::hard_blocks_needed(operation_list opt) { return _nodes_by_opt[opt].size(); }

void HardSoftLogicMixer::partial_map_node(nnode_t *node, short traverse_number, netlist_t *netlist)
{
    _opts[node->type]->partial_map_node(node, traverse_number, netlist, this);
}

void HardSoftLogicMixer::perform_optimizations(netlist_t *netlist)
{
    if (_opts[MULTIPLY]->enabled()) {
        int blocks_needed = this->hard_blocks_needed(MULTIPLY);
        _opts[MULTIPLY]->set_blocks_needed(blocks_needed);
        _opts[MULTIPLY]->assign_weights(netlist, _nodes_by_opt[MULTIPLY]);
        _opts[MULTIPLY]->perform(netlist, _nodes_by_opt[MULTIPLY]);
        _opts[MULTIPLY]->instantiate_soft_logic(netlist, _nodes_by_opt[MULTIPLY]);
    }
}
