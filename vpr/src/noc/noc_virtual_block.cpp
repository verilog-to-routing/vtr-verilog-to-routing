
#include "noc_virtual_block.h"
#include "vtr_assert.h"

NocVirtualBlock::NocVirtualBlock()
    : NocVirtualBlock(NocRouterId::INVALID()) {
}

NocVirtualBlock::NocVirtualBlock(NocRouterId mapped_noc_router_id)
    : mapped_noc_router_id_(mapped_noc_router_id){
}

NocRouterId NocVirtualBlock::get_mapped_noc_router_id() const {
    return mapped_noc_router_id_;
}

NocVirtualBlockStorage::NocVirtualBlockStorage()
    : built_virtual_middleman_blocks_(false) {
}

void NocVirtualBlock::set_mapped_noc_router_id(NocRouterId mapped_noc_router_id) {
    mapped_noc_router_id_ = mapped_noc_router_id;
}

const NocVirtualBlock& NocVirtualBlockStorage::get_middleman_block(NocVirtualMiddlemanBlockId id) const{
    return middle_man_blocks_[id];
}

const NocVirtualBlock& NocVirtualBlockStorage::get_middleman_block(NocTrafficFlowId id) const{
    return get_middleman_block((NocVirtualMiddlemanBlockId)static_cast<size_t>(id));
}

NocVirtualBlock& NocVirtualBlockStorage::get_mutable_middleman_block(NocVirtualMiddlemanBlockId id) {
    return middle_man_blocks_[id];
}

NocVirtualBlock& NocVirtualBlockStorage::get_mutable_middleman_block(NocTrafficFlowId id) {
    return get_mutable_middleman_block((NocVirtualMiddlemanBlockId)static_cast<size_t>(id));
}

void NocVirtualBlockStorage::add_middleman_block() {
    VTR_ASSERT_MSG(!built_virtual_middleman_blocks_, "NoC virtual middleman blocks have already been added, cannot modify further.");

    middle_man_blocks_.push_back(NocVirtualBlock());
}

void NocVirtualBlockStorage::finished_virtual_middleman_blocks_setup() {
    VTR_ASSERT_MSG(!built_virtual_middleman_blocks_, "NoC virtual middleman block setup has already been finished.");

    built_virtual_middleman_blocks_ = true;
}