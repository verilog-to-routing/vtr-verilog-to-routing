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
#ifndef _ODIN_UTIL_H_
#define _ODIN_UTIL_H_

#include <string>

#include "odin_types.h"

long shift_left_value_with_overflow_check(long input_value, long shift_by, loc_t loc);

const char *name_based_on_op(operation_list op);
const char *node_name_based_on_op(nnode_t *node);

char *make_full_ref_name(const char *previous, const char *module_name, const char *module_instance_name, const char *signal_name, long bit);

std::string make_simple_name(char *input, const char *flatten_string, char flatten_char);

void *my_malloc_struct(long bytes_to_alloc);

char *append_string(const char *string, const char *appendage, ...);

double wall_time();

int odin_sprintf(char *s, const char *format, ...);

void passed_verify_i_o_availabilty(nnode_t *node, int expected_input_size, int expected_output_size, const char *current_src, int line_src);

#endif // _ODIN_UTIL_H_
