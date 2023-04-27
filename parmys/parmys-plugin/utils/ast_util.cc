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
#include "odin_globals.h"
#include "odin_types.h"
#include <algorithm>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast_util.h"
#include "odin_util.h"
#include "vtr_memory.h"
#include "vtr_util.h"

/*---------------------------------------------------------------------------
 * (function: create_node_w_type)
 *-------------------------------------------------------------------------*/
ast_node_t *create_node_w_type(ids id, loc_t loc)
{
    oassert(id != NO_ID);

    static long unique_count = 0;

    ast_node_t *new_node;

    new_node = (ast_node_t *)vtr::calloc(1, sizeof(ast_node_t));
    oassert(new_node != NULL);

    new_node->type = id;
    new_node->children = NULL;
    new_node->num_children = 0;
    new_node->unique_count = unique_count++; //++count_id;
    new_node->loc = loc;
    new_node->far_tag = 0;
    new_node->high_number = 0;
    new_node->hb_port = 0;
    new_node->net_node = 0;
    new_node->types.vnumber = nullptr;
    new_node->types.identifier = NULL;
    new_node->chunk_size = 1;
    new_node->identifier_node = NULL;
    /* init value */
    new_node->types.variable.initial_value = nullptr;
    /* reset flags */
    new_node->types.variable.is_parameter = false;
    new_node->types.variable.is_string = false;
    new_node->types.variable.is_localparam = false;
    new_node->types.variable.is_defparam = false;
    new_node->types.variable.is_port = false;
    new_node->types.variable.is_input = false;
    new_node->types.variable.is_output = false;
    new_node->types.variable.is_inout = false;
    new_node->types.variable.is_wire = false;
    new_node->types.variable.is_reg = false;
    new_node->types.variable.is_genvar = false;
    new_node->types.variable.is_memory = false;
    new_node->types.variable.signedness = UNSIGNED;

    new_node->types.concat.num_bit_strings = 0;
    new_node->types.concat.bit_strings = NULL;

    return new_node;
}

/*---------------------------------------------------------------------------------------------
 * (function: create_tree_node_id)
 *-------------------------------------------------------------------------------------------*/
ast_node_t *create_tree_node_id(char *string, loc_t loc)
{
    ast_node_t *new_node = create_node_w_type(IDENTIFIERS, loc);
    new_node->types.identifier = string;

    return new_node;
}
