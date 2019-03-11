#ifndef PACK_PASS_H
#define PACK_PASS_H
#include "pass.h"
#include "pack.h"

class PackNetlistPass : public ClusterPass {
    PackPass(const t_packer_opts& packer_opts,
             std::vector<t_lb_type_rr_graph> lb_type_rr_graphs)
        : packer_opts_(packer_opts)
        , lb_type_rr_graphs_(lb_type_rr_graphs) {}
    
    bool run_on_clustering(ClusteringContext& cluster_ctx) override {
        const auto& arch = ctx().device().arch;

        try_pack(&packer_opts_,
                 &arch,
                 arch.models,
                 arch.models_library,
                 lb_type_rr_graphs_,
                 cluster_ctx);

        return true;
    }

    t_packer_opts packer_opts_;
    std::vector<t_lb_type_rr_graph> lb_type_rr_graphs_;
};

#endif
