#ifndef ODIN_II_H
#define ODIN_II_H

struct netlist_t_t *start_odin_ii(int argc,char **argv);
int terminate_odin_ii(struct netlist_t_t *odin_netlist);

void set_default_config();
void get_options(int argc, char **argv);

#endif