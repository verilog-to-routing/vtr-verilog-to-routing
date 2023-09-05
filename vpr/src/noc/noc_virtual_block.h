
#ifndef VTR_NOC_VIRTUAL_BLOCK_H
#define VTR_NOC_VIRTUAL_BLOCK_H

#include "noc_data_types.h"
#include "vtr_vector.h"

class NocVirtualBlock {
  public:
    NocVirtualBlock();
    explicit NocVirtualBlock(NocRouterId mapped_noc_router_id);

    NocRouterId get_mapped_noc_router_id() const;
    void set_mapped_noc_router_id(NocRouterId mapped_noc_router_id);

  private:
    NocRouterId mapped_noc_router_id_;
};

class NocVirtualBlockStorage {
  public:
    NocVirtualBlockStorage();
    // prevent "copying" of this object
    NocVirtualBlockStorage(const NocVirtualBlockStorage&) = delete;
    void operator=(const NocVirtualBlockStorage&) = delete;

    const NocVirtualBlock& get_middleman_block(NocVirtualMiddlemanBlockId id);
    const NocVirtualBlock& get_middleman_block(NocTrafficFlowId id);
    NocVirtualBlock& get_mutable_middleman_block(NocVirtualMiddlemanBlockId id);
    NocVirtualBlock& get_mutable_middleman_block(NocTrafficFlowId id);

    void add_middleman_block();

    void finished_virtual_middleman_blocks_setup();

  private:
    vtr::vector<NocVirtualMiddlemanBlockId, NocVirtualBlock> middle_man_blocks_;

    bool built_virtual_middleman_blocks_;
};

#endif //VTR_NOC_VIRTUAL_BLOCK_H
