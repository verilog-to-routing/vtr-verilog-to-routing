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
#ifndef _ODIN_ERROR_H_
#define _ODIN_ERROR_H_

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

struct loc_t {
    int file = -1;
    int line = -1;
    int col = -1;
};

enum odin_error {
    UTIL,
    PARSE_ARGS,
    AST,
    RESOLVE,
    NETLIST,
    PARSE_BLIF,
};

extern const char *odin_error_STR[];
extern std::vector<std::pair<std::string, int>> include_file_names;
extern int delayed_errors;
extern const loc_t unknown_location;

// causes an interrupt in GDB
[[noreturn]] void _verbose_abort(const char *condition_str, const char *odin_file_name, int odin_line_number, const char *odin_function_name);

#define oassert(condition)                                                                                                                           \
    if (!bool(condition))                                                                                                                            \
    _verbose_abort(#condition, __FILE__, __LINE__, __func__)

void _log_message(odin_error error_type, loc_t loc, bool soft_error, const char *function_file_name, int function_line, const char *function_name,
                  const char *message, ...);

#define error_message(error_type, loc, message, ...)                                                                                                 \
    _log_message(error_type, loc, true, __FILE__, __LINE__, __PRETTY_FUNCTION__, message, __VA_ARGS__)

#define warning_message(error_type, loc, message, ...)                                                                                               \
    _log_message(error_type, loc, false, __FILE__, __LINE__, __PRETTY_FUNCTION__, message, __VA_ARGS__)

#define possible_error_message(error_type, loc, message, ...)                                                                                        \
    _log_message(error_type, loc, !global_args.permissive.value(), __FILE__, __LINE__, __PRETTY_FUNCTION__, message, __VA_ARGS__)

#define delayed_error_message(error_type, loc, message, ...)                                                                                         \
    {                                                                                                                                                \
        _log_message(error_type, loc, false, __FILE__, __LINE__, __PRETTY_FUNCTION__, message, __VA_ARGS__);                                         \
        delayed_errors += 1;                                                                                                                         \
    }

void verify_delayed_error(odin_error error_type);

#endif // _ODIN_ERROR_H_
