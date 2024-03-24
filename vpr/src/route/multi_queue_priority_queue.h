#ifndef _MULTI_QUEUE_PRIORITY_QUEUE_H
#define _MULTI_QUEUE_PRIORITY_QUEUE_H

#include "heap_type.h"

#include "MultiQueueIO.h"

class MultiQueuePriorityQueue : public HeapInterface {
  public:
    using pq_node_t = std::tuple<float /*priority*/, t_heap*>;
    struct pq_compare {
        bool operator()(const pq_node_t& u, const pq_node_t& v) {
            return std::get<0>(u) > std::get<0>(v);
        }
    };
    using MQ_IO = MultiQueueIO<pq_compare, float, t_heap*>;

    MultiQueuePriorityQueue();
    MultiQueuePriorityQueue(size_t num_threads, size_t num_queues);
    ~MultiQueuePriorityQueue();
    MultiQueuePriorityQueue& operator=(MultiQueuePriorityQueue&& other) = default;

    t_heap* alloc() final;
    void free(t_heap* hptr) final;

    void init_heap(const DeviceGrid& grid) final;
    void add_to_heap(t_heap* hptr) final;
    void push_back(t_heap* const hptr) final;
    bool is_empty_heap() const final;
    bool is_valid() const final;
    void empty_heap() final;
    t_heap* get_heap_head() final;
    void build_heap() final;
    void set_prune_limit(size_t max_index, size_t prune_limit) final;

    void free_all_memory() final;

  private:
    HeapStorage storage_;
    MQ_IO* pq_;
    std::vector<pq_node_t> push_batch_buffer_;
};

#endif /* _MULTI_QUEUE_PRIORITY_QUEUE_H */
