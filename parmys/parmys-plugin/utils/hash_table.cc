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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash_table.h"
#include "odin_types.h"
#include "vtr_memory.h"

void Hashtable::destroy_free_items()
{
    for (auto kv : my_map)
        vtr::free(kv.second);
}

void Hashtable::add(std::string key, void *item) { this->my_map.emplace(key, item); }

void *Hashtable::remove(std::string key)
{
    void *value = NULL;
    auto v = this->my_map.find(key);
    if (v != this->my_map.end()) {
        value = v->second;
        this->my_map.erase(v);
    }
    return value;
}

void *Hashtable::get(std::string key)
{
    void *value = NULL;
    auto v = this->my_map.find(key);
    if (v != this->my_map.end())
        value = v->second;

    return value;
}

bool Hashtable::is_empty() { return my_map.empty(); }
