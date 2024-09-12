#ifndef _VTR_D_ARY_HEAP_H
#define _VTR_D_ARY_HEAP_H

#include <cstdint>
#include <tuple>
#include <vector>

#include "device_grid.h"
#include "rr_graph_fwd.h"
#include "d_ary_heap.tpp"

using pq_prio_t = float;
using pq_index_t = uint32_t;

inline pq_index_t cast_RRNodeId_to_pq_index_t(RRNodeId node) {
    static_assert(sizeof(RRNodeId) == sizeof(pq_index_t));
    return static_cast<pq_index_t>(std::size_t(node));
}

class DAryHeap {
  public:
    using pq_pair_t = std::tuple<pq_prio_t /*priority*/, pq_index_t>;
    struct pq_compare {
        bool operator()(const pq_pair_t& u, const pq_pair_t& v) {
            return std::get<0>(u) > std::get<0>(v);
        }
    };
    using pq_io_t = customized_d_ary_priority_queue<4, pq_pair_t, std::vector<pq_pair_t>, pq_compare>;

    DAryHeap() {
        pq_ = new pq_io_t();
    }

    ~DAryHeap(){
        delete pq_;
    }

    void init_heap(const DeviceGrid& grid) {
        size_t target_heap_size = (grid.width() - 1) * (grid.height() - 1);
        pq_->reserve(target_heap_size);
    }

    bool try_pop(pq_prio_t &prio, RRNodeId &node) {
        if (pq_->empty()) {
            return false;
        } else {
            pq_index_t node_id;
            std::tie(prio, node_id) = pq_->top();
            static_assert(sizeof(RRNodeId) == sizeof(pq_index_t));
            node = RRNodeId(node_id);
            pq_->pop();
            return true;
        }
    }

    void add_to_heap(const pq_prio_t& prio, const RRNodeId& node) {
        pq_->push({prio, cast_RRNodeId_to_pq_index_t(node)});
    }

    void push_back(const pq_prio_t& prio, const RRNodeId& node) {
        pq_->push({prio, cast_RRNodeId_to_pq_index_t(node)});
    }

    bool is_empty_heap() const {
        return (bool)(pq_->empty());
    }

    bool is_valid() const {
        return true;
    }

    void empty_heap() {
        pq_->clear();
    }

    void build_heap() {}

  private:
    pq_io_t* pq_;
};

#endif /* _VTR_D_ARY_HEAP_H */
