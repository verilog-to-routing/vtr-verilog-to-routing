#ifndef __SCOPE_H__
#define __SCOPE_H__

#include "string_cache.h"

#include <vector>

struct sc_scope {
    STRING_CACHE* defparam_sc;
    STRING_CACHE* param_sc;
};

sc_scope* new_sc_scope();
void move_sc_scope_items(sc_scope** source, sc_scope* destination);
void free_sc_scope(sc_scope** to_free_ref, STRING_CACHE** param, STRING_CACHE** defparam);

extern sc_scope* current_scope;
extern std::vector<sc_scope*> scope_stack;

sc_scope* pop_scope();
void merge_top_scopes();
void push_scope();

#endif
