#ifndef	READ_BLIF_H
#define READ_BLIF_H
#include "logic_types.h"
#include "atom_netlist_fwd.h"

AtomNetlist read_and_process_blif(const char *blif_file, 
                                  const t_model *user_models, 
                                  const t_model *library_models,
                                  bool should_absorb_buffers, 
                                  bool should_sweep_primary_ios, 
                                  bool should_sweep_nets,
                                  bool should_sweep_blocks, 
                                  bool read_activity_file, 
                                  char * activity_file);

#endif /*READ_BLIF_H*/
