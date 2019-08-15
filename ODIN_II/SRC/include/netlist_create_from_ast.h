#ifndef NETLIST_CREATE_FROM_AST_H
#define NETLIST_CREATE_FROM_AST_H

void create_netlist();
void connect_memory_and_alias(ast_node_t* hb_instance, char *instance_name_prefix, STRING_CACHE_LIST *local_string_cache_list);
void create_symbol_table_for_scope(ast_node_t* module_items, STRING_CACHE_LIST *local_string_cache_list);

#endif
