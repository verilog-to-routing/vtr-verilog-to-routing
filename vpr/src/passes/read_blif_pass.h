#ifndef READ_BLIF_PASS_H
#define READ_BLIF_PASS_H
#include "pass.h"
#include "read_blif.h"

class ReadBlifPass : public AtomPass {
    ReadBlifPass(const e_circuit_format& cct_fmt,
                 const std::string& blif_file_path)
        : cct_fmt_(cct_fmt)
        , blif_file_path_(blif_file_path) {}
    
    
    bool run_on_atom(AtomContext& atom_ctx) override {
        const auto& arch = ctx().device().arch;

        atom_ctx.netlist = read_blif(cct_fmt_, blif_file_path, arch.models, arch.model_library);

        return true;
    }

    std::string blif_file_path_;
    e_circuit_format cct_fmt_;
};

#endif
