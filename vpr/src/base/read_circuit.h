#ifndef VPR_READ_CIRCUIT_H
#define VPR_READ_CIRCUIT_H
#include "logic_types.h"
#include "atom_netlist_fwd.h"
#include "vpr_types.h"

enum class e_circuit_format {
    AUTO, //Infer from file extension
    BLIF, //Strict structural BLIF
    EBLIF //Structural blif with extensions
};

AtomNetlist read_and_process_circuit(const e_circuit_format circuit_format,
                                     const char* circuit_file,
                                     const t_model* user_models,
                                     const t_model* library_models,
                                     e_const_gen_inference const_gen_inference,
                                     bool should_absorb_buffers,
                                     bool should_sweep_dangling_primary_ios,
                                     bool should_sweep_dangling_nets,
                                     bool should_sweep_dangling_blocks,
                                     bool should_sweep_constant_primary_outputs,
                                     int verbosity);
#endif
