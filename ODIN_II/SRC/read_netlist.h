#include "read_xml_arch_file.h"

netlist_t *read_netlist (
    const char *net_filename,
    int ntypes,
    const struct s_type_descriptor block_types[],
    t_type_ptr IO_type);
