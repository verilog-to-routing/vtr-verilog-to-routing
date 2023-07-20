//
// Created by amin on 1/17/23.
//

#ifndef VTR_INTRA_CLUSTER_SERDES_H
#define VTR_INTRA_CLUSTER_SERDES_H

#include <functional>
#include <vector>
#include <unordered_map>

#include "vtr_ndmatrix.h"
#include "vpr_error.h"
#include "matrix.capnp.h"
#include "map_lookahead.capnp.h"
#include "vpr_types.h"
#include "router_lookahead_map_utils.h"

template<typename CapElemType, typename ElemType>
void toVector(std::vector<ElemType>& vec_out,
              const typename capnp::List<CapElemType>::Reader& m_in,
              const std::function<void(std::vector<ElemType>&,
                                       int,
                                       const ElemType&)>& copy_fun) {
    int size = m_in.size();
    vec_out.resize(size);
    for(int idx = 0; idx < size; idx++) {
        copy_fun(vec_out, idx, m_in[idx]);
    }
}

template<typename CapKeyType, typename CapValType, typename KeyType, typename CostType>
void toUnorderedMap(
    std::unordered_map<KeyType, CostType>& map_in,
    const int begin_flat_idx,
    const int end_flat_idx,
    const typename capnp::List<CapKeyType>::Reader& m_out_key,
    const typename capnp::List<CapValType>::Reader& m_out_val,
    const std::function<void(std::unordered_map<KeyType, CostType>&,
                             const KeyType&,
                             const typename CapValType::Reader&)>& copy_fun) {

    for(int flat_idx = begin_flat_idx; flat_idx < end_flat_idx; flat_idx++) {
        copy_fun(map_in, m_out_key[flat_idx], m_out_val[flat_idx]);
    }
}

template<typename CapElemType, typename ElemType>
void fromVector(typename capnp::List<CapElemType>::Builder& m_out,
                const std::vector<ElemType>& vec_in,
                const std::function<void(typename capnp::List<CapElemType>::Builder&,
                                         int,
                                         const ElemType&)>& copy_fun) {

    for(int idx = 0; idx < (int)vec_in.size(); idx++) {
        copy_fun(m_out, idx, vec_in[idx]);
    }
}

template<typename CapKeyType, typename CapValType, typename KeyType, typename CostType>
void FromUnorderedMap(
        typename capnp::List<CapKeyType>::Builder& m_out_key,
        typename capnp::List<CapValType>::Builder& m_out_val,
        const KeyType out_offset,
        const std::unordered_map<KeyType, CostType>& map_in,
        const std::function<void(typename capnp::List<CapKeyType>::Builder&,
                                 typename capnp::List<CapValType>::Builder&,
                                 int,
                                 const KeyType&,
                                 const CostType&)>& copy_fun) {

        int flat_idx = out_offset;
        for (const auto& entry : map_in) {
            copy_fun(m_out_key, m_out_val, flat_idx, entry.first, entry.second);
            flat_idx++;
        }
}




#endif //VTR_INTRA_CLUSTER_SERDES_H
