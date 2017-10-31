#ifndef	READ_BLIF_H
#define READ_BLIF_H
#include "logic_types.h"
#include "atom_netlist_fwd.h"

AtomNetlist read_and_process_blif(const char *blif_file, 
                                  const t_model *user_models, 
                                  const t_model *library_models,
                                  bool should_absorb_buffers, 
                                  bool should_sweep_dangling_primary_ios, 
                                  bool should_sweep_dangling_nets,
                                  bool should_sweep_dangling_blocks,
                                  bool should_sweep_constant_primary_outputs);

#endif /*READ_BLIF_H*/
