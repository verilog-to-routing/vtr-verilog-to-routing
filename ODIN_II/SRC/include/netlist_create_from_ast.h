#ifndef NETLIST_CREATE_FROM_AST_H
#define NETLIST_CREATE_FROM_AST_H

void create_netlist();
void connect_memory_and_alias(ast_node_t* hb_instance, char *instance_name_prefix, sc_hierarchy *local_ref);
void create_symbol_table_for_scope(ast_node_t* module_items, sc_hierarchy *local_ref);

#endif
