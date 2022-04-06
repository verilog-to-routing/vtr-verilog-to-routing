#ifndef READ_BLIF_H
#define READ_BLIF_H
#include "logic_types.h"
#include "atom_netlist_fwd.h"
#include "read_circuit.h"

bool is_string_param(const std::string& param);
bool is_binary_param(const std::string& param);
bool is_real_param(const std::string& param);

AtomNetlist read_blif(e_circuit_format circuit_format,
                      const char* blif_file,
                      const t_model* user_models,
                      const t_model* library_models);

#endif /*READ_BLIF_H*/
