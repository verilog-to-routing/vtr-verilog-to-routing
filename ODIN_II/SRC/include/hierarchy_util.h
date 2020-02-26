#ifndef __HIERARCHY_H__
#define __HIERARCHY_H__

#include "string_cache.h"
#include "odin_types.h"

struct sc_hierarchy {
    char* scope_id;
    char* instance_name_prefix;

    struct ast_node_t* top_node;

    STRING_CACHE* local_defparam_table_sc;
    STRING_CACHE* local_param_table_sc;
    STRING_CACHE* local_symbol_table_sc;

    struct ast_node_t** local_symbol_table;
    int num_local_symbol_table;

    sc_hierarchy* parent;

    sc_hierarchy** module_children;
    sc_hierarchy** function_children;
    sc_hierarchy** task_children;
    sc_hierarchy** block_children;

    int num_module_children;
    int num_function_children;
    int num_task_children;
    int num_block_children;

    int num_unnamed_genblks;
};

sc_hierarchy* init_sc_hierarchy();
sc_hierarchy* copy_sc_hierarchy(sc_hierarchy* to_copy);
void free_sc_hierarchy(sc_hierarchy* to_free);
ast_node_t* resolve_hierarchical_name_reference(sc_hierarchy* local_ref, char* identifier);

#endif