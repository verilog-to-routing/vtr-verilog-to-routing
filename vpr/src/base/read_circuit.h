#ifndef VPR_READ_CIRCUIT_H
#define VPR_READ_CIRCUIT_H

#include "atom_netlist_fwd.h"

struct t_vpr_setup;
struct t_arch;

enum class e_circuit_format {
    AUTO,            ///<Infer from file extension
    BLIF,            ///<Strict structural BLIF
    EBLIF,           ///<Structural blif with extensions
    FPGA_INTERCHANGE ///<FPGA Interhange logical netlis format
};

AtomNetlist read_and_process_circuit(e_circuit_format circuit_format, t_vpr_setup& vpr_setup, t_arch& arch);
#endif
