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

#include "scope_util.h"

#include "vtr_util.h"
#include "vtr_memory.h"

sc_scope* current_scope = NULL;
std::vector<sc_scope*> scope_stack;

sc_scope* new_sc_scope() {
    sc_scope* new_scope = (sc_scope*)vtr::calloc(1, sizeof(sc_scope));
    if (new_scope) {
        new_scope->defparam_sc = sc_new_string_cache();
        new_scope->param_sc = sc_new_string_cache();
    }
    return new_scope;
}

void move_sc_scope_items(sc_scope** source_ref, sc_scope* destination) {
    sc_scope* source = (*source_ref);
    if (source) {
        sc_merge_string_cache(&(source->defparam_sc), destination->defparam_sc);
        sc_merge_string_cache(&(source->param_sc), destination->param_sc);
        vtr::free(source);
        source = NULL;
    }
    (*source_ref) = source;
}

sc_scope* pop_scope() {
    sc_scope* to_return = NULL;
    if (!scope_stack.empty()) {
        to_return = scope_stack.back();
        scope_stack.pop_back();
    }

    /* current param table is the top */
    if (!scope_stack.empty()) {
        current_scope = scope_stack.back();
    } else {
        current_scope = NULL;
    }

    return to_return;
}

void merge_top_scopes() {
    if (scope_stack.size() > 1) {
        sc_scope* insertion = pop_scope();
        move_sc_scope_items(&insertion, scope_stack.back());
    }
}

void push_scope() {
    scope_stack.push_back(new_sc_scope());

    /* update current scope */
    current_scope = scope_stack.back();
}