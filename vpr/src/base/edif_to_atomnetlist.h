#ifndef READ_EDIF_H
#define READ_EDIF_H
#include "logic_types.h"
#include "atom_netlist_fwd.h"
#include "read_circuit.h"

AtomNetlist read_edif(e_circuit_format circuit_format,
                      const char* edif_file,
                      const t_model* user_models,
                      const t_model* library_models);

#endif /*READ_EDIF_H*/
