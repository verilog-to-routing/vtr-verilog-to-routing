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
#include <cstdarg>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "odin_util.h"
#include "vtr_memory.h"
#include "vtr_path.h"
#include "vtr_util.h"
#include <regex>
#include <stdbool.h>

// for mkdir
#ifdef WIN32
#include <direct.h>
#define getcwd _getcwd
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

long shift_left_value_with_overflow_check(long input_value, long shift_by, loc_t loc)
{
    if (shift_by < 0)
        error_message(NETLIST, loc, "requesting a shift left that is negative [%ld]\n", shift_by);
    else if (shift_by >= (long)ODIN_STD_BITWIDTH - 1)
        warning_message(NETLIST, loc, "requesting a shift left that will overflow the maximum size of %ld [%ld]\n", shift_by, ODIN_STD_BITWIDTH - 1);

    return input_value << shift_by;
}

/*---------------------------------------------------------------------------------------------
 * (function: name_based_on_op)
 * 	Get the string version of an operation
 *-------------------------------------------------------------------------------------------*/
const char *name_based_on_op(operation_list op)
{
    oassert(op < operation_list_END && "OUT OF BOUND operation_list!");

    return operation_list_STR[op][ODIN_STRING_TYPE];
}

/*---------------------------------------------------------------------------------------------
 * (function: node_name_based_on_op)
 * 	Get the string version of a node
 *-------------------------------------------------------------------------------------------*/
const char *node_name_based_on_op(nnode_t *node) { return name_based_on_op(node->type); }

/*---------------------------------------------------------------------------------------------
 * (function: make_full_ref_name)
 * // {previous_string}.instance_name
 * // {previous_string}.instance_name^signal_name
 * // {previous_string}.instance_name^signal_name~bit
 *-------------------------------------------------------------------------------------------*/
char *make_full_ref_name(const char *previous, const char * /*module_name*/, const char *module_instance_name, const char *signal_name, long bit)
{
    std::stringstream return_string;
    if (previous)
        return_string << previous;

    if (module_instance_name)
        return_string << "." << module_instance_name;

    if (signal_name && (previous || module_instance_name))
        return_string << "^";

    if (signal_name)
        return_string << signal_name;

    if (bit != -1) {
        oassert(signal_name);
        return_string << "~" << std::dec << bit;
    }
    return vtr::strdup(return_string.str().c_str());
}

/*---------------------------------------------------------------------------------------------
 *  (function: make_simple_name )
 *-------------------------------------------------------------------------------------------*/
std::string make_simple_name(char *input, const char *flatten_string, char flatten_char)
{
    oassert(input);
    oassert(flatten_string);

    std::string input_str = input;
    std::string flatten_str = flatten_string;

    for (size_t i = 0; i < flatten_str.length(); i++)
        std::replace(input_str.begin(), input_str.end(), flatten_str[i], flatten_char);

    return input_str;
}

/*-----------------------------------------------------------------------
 * (function: my_malloc_struct )
 *-----------------------------------------------------------------*/
void *my_malloc_struct(long bytes_to_alloc)
{
    void *allocated = vtr::calloc(1, bytes_to_alloc);
    static long int m_id = 0;

    // ways to stop the execution at the point when a specific structure is built...note it needs to be m_id - 1 ... it's unique_id in most data
    // structures
    // oassert(m_id != 193);

    if (allocated == NULL) {
        fprintf(stderr, "MEMORY FAILURE\n");
        oassert(0);
    }

    /* mark the unique_id */
    *((long int *)allocated) = m_id++;

    return allocated;
}

/*
 * Returns a new string consisting of the original string
 * plus the appendage. Leaves the original string
 * intact.
 *
 * Handles format strings as well.
 */
char *append_string(const char *string, const char *appendage, ...)
{
    char buffer[vtr::bufsize];

    va_list ap;

    va_start(ap, appendage);
    vsnprintf(buffer, vtr::bufsize * sizeof(char), appendage, ap);
    va_end(ap);

    std::string new_string = std::string(string) + std::string(buffer);
    return vtr::strdup(new_string.c_str());
}

void passed_verify_i_o_availabilty(nnode_t *node, int expected_input_size, int expected_output_size, const char *current_src, int line_src)
{
    if (!node)
        error_message(UTIL, unknown_location, "node unavailable @%s::%d", current_src, line_src);

    std::stringstream err_message;
    int error = 0;

    if (expected_input_size != -1 && node->num_input_pins != expected_input_size) {
        err_message << " input size is " << std::to_string(node->num_input_pins) << " expected 3:\n";
        for (int i = 0; i < node->num_input_pins; i++)
            err_message << "\t" << node->input_pins[0]->name << "\n";

        error = 1;
    }

    if (expected_output_size != -1 && node->num_output_pins != expected_output_size) {
        err_message << " output size is " << std::to_string(node->num_output_pins) << " expected 1:\n";
        for (int i = 0; i < node->num_output_pins; i++)
            err_message << "\t" << node->output_pins[0]->name << "\n";

        error = 1;
    }

    if (error)
        error_message(UTIL, node->loc, "failed for %s:%s %s\n", node_name_based_on_op(node), node->name, err_message.str().c_str());
}

/*
 * Gets the current time in seconds.
 */
double wall_time()
{
    auto time_point = std::chrono::system_clock::now();
    std::chrono::duration<double> time_since_epoch = time_point.time_since_epoch();

    return time_since_epoch.count();
}

/**
 * This overrides default sprintf since odin uses sprintf to concatenate strings
 * sprintf has undefined behavior for such and this prevents string overriding if
 * it is also given as an input
 */
int odin_sprintf(char *s, const char *format, ...)
{
    va_list args, args_copy;
    va_start(args, format);
    va_copy(args_copy, args);

    const auto sz = std::vsnprintf(nullptr, 0, format, args) + 1;

    try {
        std::string temp(sz, ' ');
        std::vsnprintf(&temp.front(), sz, format, args_copy);
        va_end(args_copy);
        va_end(args);

        s = strncpy(s, temp.c_str(), temp.length());

        return temp.length();

    } catch (const std::bad_alloc &) {
        va_end(args_copy);
        va_end(args);
        return -1;
    }
}
