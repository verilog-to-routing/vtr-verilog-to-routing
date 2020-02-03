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

void free_sc_scope(sc_scope** to_free_ref, STRING_CACHE** param, STRING_CACHE** defparam) {
    sc_scope* to_free = (*to_free_ref);
    if (to_free) {
        *defparam = to_free->defparam_sc;
        *param = to_free->param_sc;

        to_free->defparam_sc = NULL;
        to_free->param_sc = NULL;

        vtr::free(to_free);
        to_free = NULL;
    }

    (*to_free_ref) = to_free;
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