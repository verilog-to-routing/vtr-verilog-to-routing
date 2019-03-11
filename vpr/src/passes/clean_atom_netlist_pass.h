#ifndef CLEAN_ATOM_NETLIST_PASS_H
#define CLEAN_ATOM_NETLIST_PASS_H

#include "pass.h"


class CleanAtomNetlistPass : public AtomPass {
    
    bool run_on_atom(AtomContext& atom_ctx) override {
        auto& arch = ctx().device().arch;

        clean_circuit(atom_ctx.nlist,
                      &arch.library_models,
                      should_absorb_buffers_,
                      should_sweep_dangling_primary_ios_,
                      should_sweep_dangling_nets_,
                      should_sweep_dangling_blocks_,
                      should_sweep_constant_primary_outputs_);
    }

    private:
        should_absorb_buffers_ = true;
        should_sweep_dangling_primary_ios_ = true;
        should_sweep_dangling_nets_ = true;
        should_sweep_dangling_blocks_ = true;
        should_sweep_constant_primary_outputs_ = true;
};

#endif
