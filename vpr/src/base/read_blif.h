#ifndef READ_BLIF_H
#define READ_BLIF_H

#include <string>
#include "atom_netlist_fwd.h"
#include "read_circuit.h"

class LogicalModels;

bool is_string_param(const std::string& param);
bool is_binary_param(const std::string& param);
bool is_real_param(const std::string& param);

AtomNetlist read_blif(e_circuit_format circuit_format,
                      const char* blif_file,
                      const LogicalModels& models);

#endif /*READ_BLIF_H*/
