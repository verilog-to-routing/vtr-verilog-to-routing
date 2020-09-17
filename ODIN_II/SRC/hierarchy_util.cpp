#include "vtr_util.h"
#include "vtr_memory.h"
#include "ast_util.h"

STRING_CACHE* copy_param_table_sc(STRING_CACHE* to_copy);

ast_node_t* resolve_hierarchical_name_reference_by_path_search(sc_hierarchy* local_ref, std::string identifier);
ast_node_t* resolve_hierarchical_name_reference_by_upward_search(sc_hierarchy* local_ref, std::string identifier);

sc_hierarchy* init_sc_hierarchy() {
    sc_hierarchy* hierarchy = (sc_hierarchy*)vtr::calloc(1, sizeof(sc_hierarchy));
    hierarchy->parent = NULL;

    hierarchy->module_children = NULL;
    hierarchy->function_children = NULL;
    hierarchy->task_children = NULL;
    hierarchy->block_children = NULL;

    hierarchy->num_module_children = 0;
    hierarchy->num_function_children = 0;
    hierarchy->num_task_children = 0;
    hierarchy->num_block_children = 0;

    hierarchy->num_unnamed_genblks = 0;

    hierarchy->instance_name_prefix = NULL;
    hierarchy->scope_id = NULL;

    hierarchy->top_node = NULL;

    hierarchy->local_param_table_sc = NULL;
    hierarchy->local_defparam_table_sc = NULL;
    hierarchy->local_symbol_table_sc = NULL;

    hierarchy->local_symbol_table = NULL;
    hierarchy->num_local_symbol_table = 0;

    return hierarchy;
}

/*---------------------------------------------------------------------------
 * (function: copy_sc_hierarchy)
 *-------------------------------------------------------------------------*/
sc_hierarchy* copy_sc_hierarchy(sc_hierarchy* to_copy) {
    sc_hierarchy* hierarchy = (sc_hierarchy*)vtr::calloc(1, sizeof(sc_hierarchy));
    hierarchy->scope_id = NULL;
    hierarchy->instance_name_prefix = NULL;
    hierarchy->top_node = NULL;

    hierarchy->num_local_symbol_table = to_copy->num_local_symbol_table;
    hierarchy->local_symbol_table = (ast_node_t**)vtr::calloc(hierarchy->num_local_symbol_table, sizeof(ast_node_t*));
    hierarchy->local_symbol_table_sc = sc_new_string_cache();

    for (int i = 0; i < hierarchy->num_local_symbol_table; i++) {
        hierarchy->local_symbol_table[i] = ast_node_deep_copy(to_copy->local_symbol_table[i]);

        char* identifier = hierarchy->local_symbol_table[i]->identifier_node->types.identifier;
        long sc_spot = sc_add_string(hierarchy->local_symbol_table_sc, identifier);
        hierarchy->local_symbol_table_sc->data[sc_spot] = (void*)hierarchy->local_symbol_table[i];
    }

    if (to_copy->local_param_table_sc) {
        hierarchy->local_param_table_sc = copy_param_table_sc(to_copy->local_param_table_sc);
    }

    return hierarchy;
}

/*---------------------------------------------------------------------------
 * (function: copy_param_table_sc)
 *-------------------------------------------------------------------------*/
STRING_CACHE* copy_param_table_sc(STRING_CACHE* to_copy) {
    STRING_CACHE* sc;

    sc = sc_new_string_cache();

    for (long i = 0; i < to_copy->free; i++) {
        long sc_spot = sc_add_string(sc, to_copy->string[i]);
        sc->data[sc_spot] = (void*)ast_node_deep_copy((ast_node_t*)to_copy->data[i]);
    }

    return sc;
}

/*---------------------------------------------------------------------------
 * (function: free_sc_hierarchy)
 *-------------------------------------------------------------------------*/
void free_sc_hierarchy(sc_hierarchy* to_free) {
    int i;

    for (i = 0; i < to_free->num_module_children; i++) {
        free_sc_hierarchy(to_free->module_children[i]);
    }
    to_free->module_children = (sc_hierarchy**)vtr::free(to_free->module_children);

    for (i = 0; i < to_free->num_function_children; i++) {
        free_sc_hierarchy(to_free->function_children[i]);
    }
    to_free->function_children = (sc_hierarchy**)vtr::free(to_free->function_children);

    for (i = 0; i < to_free->num_task_children; i++) {
        free_sc_hierarchy(to_free->task_children[i]);
    }
    to_free->task_children = (sc_hierarchy**)vtr::free(to_free->task_children);

    for (i = 0; i < to_free->num_block_children; i++) {
        free_sc_hierarchy(to_free->block_children[i]);
    }
    to_free->block_children = (sc_hierarchy**)vtr::free(to_free->block_children);

    if (to_free->local_param_table_sc) {
        for (i = 0; i < to_free->local_param_table_sc->free; i++) {
            free_whole_tree((ast_node_t*)to_free->local_param_table_sc->data[i]);
        }
        to_free->local_param_table_sc = sc_free_string_cache(to_free->local_param_table_sc);
    }

    for (i = 0; i < to_free->num_local_symbol_table; i++) {
        free_whole_tree(to_free->local_symbol_table[i]);
    }
    to_free->num_local_symbol_table = 0;
    to_free->local_symbol_table = (ast_node_t**)vtr::free(to_free->local_symbol_table);
    to_free->local_symbol_table_sc = sc_free_string_cache(to_free->local_symbol_table_sc);

    to_free->instance_name_prefix = (char*)vtr::free(to_free->instance_name_prefix);
    to_free->scope_id = (char*)vtr::free(to_free->scope_id);

    to_free->top_node = NULL;

    to_free->num_module_children = 0;
    to_free->num_function_children = 0;
    to_free->num_task_children = 0;
    to_free->num_unnamed_genblks = 0;

    vtr::free(to_free);
}

/*---------------------------------------------------------------------------
 * (function: resolve_hierarchical_name_reference)
 *-------------------------------------------------------------------------*/
ast_node_t* resolve_hierarchical_name_reference(sc_hierarchy* local_ref, char* identifier) {
    // upward search: item identifier only (e.g. my_item)
    // path search: series of scopes followed by item identifier (e.g. some_prefix.some_other_prefix.my_item)

    if (identifier) {
        ast_node_t* var_declare = NULL;
        std::string id(identifier);

        size_t pos = id.find_first_of('.', 0);
        if (pos == std::string::npos) {
            var_declare = resolve_hierarchical_name_reference_by_upward_search(local_ref, id);
        } else {
            // find parent scope with this name and pass down the rest of the path
            std::string scope_id = id.substr(0, pos + 1);
            id = id.erase(0, pos + 1);

            sc_hierarchy* this_ref = local_ref;
            bool found = false;

            while (!found) {
                if (this_ref->scope_id == scope_id) {
                    found = true;
                } else {
                    /* check for downward search in this scope */
                    for (int i = 0; !found && i < this_ref->num_module_children; i++) {
                        if (this_ref->module_children[i]->scope_id == scope_id) {
                            this_ref = this_ref->module_children[i];
                            found = true;
                        }
                    }
                    for (int i = 0; !found && i < this_ref->num_function_children; i++) {
                        if (this_ref->function_children[i]->scope_id == scope_id) {
                            this_ref = this_ref->function_children[i];
                            found = true;
                        }
                    }
                    for (int i = 0; !found && i < this_ref->num_task_children; i++) {
                        if (this_ref->task_children[i]->scope_id == scope_id) {
                            this_ref = this_ref->task_children[i];
                            found = true;
                        }
                    }
                    for (int i = 0; !found && i < this_ref->num_block_children; i++) {
                        if (this_ref->block_children[i]->scope_id == scope_id) {
                            this_ref = this_ref->block_children[i];
                            found = true;
                        }
                    }
                }

                /* if not found, check upward */
                if (!found && this_ref->parent != NULL) {
                    this_ref = this_ref->parent;
                }
            }

            if (found) {
                var_declare = resolve_hierarchical_name_reference_by_path_search(this_ref, id);
            }
        }

        return var_declare;
    }

    return NULL;
}

/*---------------------------------------------------------------------------
 * (function: resolve_hierarchical_name_reference_by_path_search)
 *-------------------------------------------------------------------------*/
ast_node_t* resolve_hierarchical_name_reference_by_path_search(sc_hierarchy* local_ref, std::string identifier) {
    if (!identifier.empty()) {
        ast_node_t* var_declare = NULL;

        size_t pos = identifier.find_first_of('.', 0);
        if (pos == std::string::npos) {
            // this is the item we're searching for; look for item in current scope
            long sc_spot;
            bool found = false;

            STRING_CACHE* local_symbol_table_sc = local_ref->local_symbol_table_sc;

            if ((sc_spot = sc_lookup_string(local_symbol_table_sc, identifier.c_str())) != -1) {
                var_declare = (ast_node_t*)local_symbol_table_sc->data[sc_spot];
                found = true;
            }

            if (!found) {
                STRING_CACHE* local_param_table_sc = local_ref->local_param_table_sc;
                if ((sc_spot = sc_lookup_string(local_param_table_sc, identifier.c_str())) != -1) {
                    var_declare = (ast_node_t*)local_param_table_sc->data[sc_spot];
                    found = true;
                }
            }

            if (!found) {
                // TODO check scopes???
            }
        } else {
            // find scope with this name and pass down the rest of the path
            std::string scope_id = identifier.substr(0, pos + 1);
            identifier = identifier.erase(0, pos + 1);

            for (int i = 0; !var_declare && i < local_ref->num_module_children; i++) {
                if (local_ref->module_children[i]->scope_id == scope_id) {
                    var_declare = resolve_hierarchical_name_reference_by_path_search(local_ref->module_children[i], identifier);
                }
            }

            for (int i = 0; !var_declare && i < local_ref->num_function_children; i++) {
                if (local_ref->function_children[i]->scope_id == scope_id) {
                    var_declare = resolve_hierarchical_name_reference_by_path_search(local_ref->function_children[i], identifier);
                }
            }

            for (int i = 0; !var_declare && i < local_ref->num_task_children; i++) {
                if (local_ref->task_children[i]->scope_id == scope_id) {
                    var_declare = resolve_hierarchical_name_reference_by_path_search(local_ref->task_children[i], identifier);
                }
            }

            for (int i = 0; !var_declare && i < local_ref->num_block_children; i++) {
                if (local_ref->block_children[i]->scope_id == scope_id) {
                    var_declare = resolve_hierarchical_name_reference_by_path_search(local_ref->block_children[i], identifier);
                }
            }
        }

        return var_declare;
    }

    return NULL;
}

/*---------------------------------------------------------------------------
 * (function: resolve_hierarchical_name_reference_by_upward_search)
 *-------------------------------------------------------------------------*/
ast_node_t* resolve_hierarchical_name_reference_by_upward_search(sc_hierarchy* local_ref, std::string identifier) {
    if (!identifier.empty()) {
        ast_node_t* var_declare = NULL;

        long sc_spot;
        bool found = false;

        STRING_CACHE* local_symbol_table_sc = local_ref->local_symbol_table_sc;

        if ((sc_spot = sc_lookup_string(local_symbol_table_sc, identifier.c_str())) != -1) {
            var_declare = (ast_node_t*)local_symbol_table_sc->data[sc_spot];
            found = true;
        }

        if (!found) {
            STRING_CACHE* local_param_table_sc = local_ref->local_param_table_sc;
            if ((sc_spot = sc_lookup_string(local_param_table_sc, identifier.c_str())) != -1) {
                var_declare = (ast_node_t*)local_param_table_sc->data[sc_spot];
                found = true;
            }
        }

        if (!found && local_ref->parent != NULL) {
            // TODO check scopes???
            var_declare = resolve_hierarchical_name_reference_by_upward_search(local_ref->parent, identifier);
        }

        return var_declare;
    }

    return NULL;
}