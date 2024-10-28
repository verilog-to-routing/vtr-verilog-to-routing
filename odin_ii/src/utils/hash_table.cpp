/*
 * Copyright 2023 CAS—Atlantic (University of New Brunswick, CASA)
 * 
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

#include <cstring>

#include "hash_table.h"
#include "odin_types.h"

#include "vtr_memory.h"

void hash_table::destroy_free_items() {
    for (auto kv : my_map)
        vtr::free(kv.second);
}

void hash_table::add(std::string key, void* item) { this->my_map.emplace(key, item); }

void* hash_table::remove(std::string key) {
    void* value = NULL;
    auto v = this->my_map.find(key);
    if (v != this->my_map.end()) {
        value = v->second;
        this->my_map.erase(v);
    }
    return value;
}

void* hash_table::get(std::string key) {
    void* value = NULL;
    auto v = this->my_map.find(key);
    if (v != this->my_map.end()) value = v->second;

    return value;
}

bool hash_table::is_empty() { return my_map.empty(); }
