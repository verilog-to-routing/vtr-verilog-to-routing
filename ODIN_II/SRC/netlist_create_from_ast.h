#ifndef NETLIST_CREATE_FROM_AST_H
#define NETLIST_CREATE_FROM_AST_H

extern void create_netlist();
extern void connect_memory_and_alias(ast_node_t* hb_instance, char *instance_name_prefix);

#endif
