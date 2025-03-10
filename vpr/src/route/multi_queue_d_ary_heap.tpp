/********************************************************************
 * MultiQueue Implementation
 *
 * Originally authored by Guozheng Zhang, Gilead Posluns, and Mark C. Jeffrey
 * Published at the 36th ACM Symposium on Parallelism in Algorithms and
 * Architectures (SPAA), June 2024
 *
 * Original source: https://github.com/mcj-group/cps
 *
 * This implementation and interface has been modified from the original to:
 *   - Support queue draining functionality
 *   - Enable integration with the VTR project
 *
 * The MultiQueue data structure provides an efficient concurrent priority
 * queue implementation designed for parallel processing applications.
 *
 * Modified: February 2025
 ********************************************************************/

#pragma once

#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <vector>
#include <algorithm>
#include <atomic>
#include <cassert>
#include "d_ary_heap.tpp"

#define CACHELINE 64

// #define PERF 1
#define MQ_IO_ENABLE_CLEAR_FOR_POP

template<
    unsigned D,
    typename PQElement,
    typename Comparator,
    typename PrioType
>
class MultiQueueIO {
    using PQ = customized_d_ary_priority_queue<D, PQElement, std::vector<PQElement>, Comparator>;
    Comparator compare;

    // Special value used to signify that there is no 'min' element in a PQ
    // container. The user should ensure that they do not use this priority
    // while using the MQ.
    static constexpr PrioType EMPTY_PRIO = std::numeric_limits<PrioType>::max();

    struct PQContainer {
        uint64_t pushes = 0;
        uint64_t pops = 0;
        PQ pq;
        std::atomic_flag queueLock = ATOMIC_FLAG_INIT;
        std::atomic<PrioType> min{EMPTY_PRIO};

        void lock() { while(queueLock.test_and_set(std::memory_order_acquire)); }
        bool try_lock() { return queueLock.test_and_set(std::memory_order_acquire); }
        void unlock() { queueLock.clear(std::memory_order_release); }

    } __attribute__((aligned (CACHELINE)));

    std::vector<
        PQContainer
        // FIXME: Disabled this due to VTR not using Boost. There is a C++ way
        //        of doing this, but it requires making an aligned allocator
        //        class. May be a good idea to add to VTR util in the future.
        //        Should profile for performance first; may not be worth it.
        // , boost::alignment::aligned_allocator<PQContainer, CACHELINE>
    > queues;
    uint64_t NUM_QUEUES;

    // Termination:
    //  - numIdle records the number of threads that believe
    //    there are no more work to do.
    //  -numEmpty records number of queues that are empty
    uint64_t threadNum;
    std::atomic<uint64_t> numIdle{0};
    std::atomic<uint64_t> numEmpty;
#ifdef MQ_IO_ENABLE_CLEAR_FOR_POP
    std::atomic<PrioType> minPrioForPop{std::numeric_limits<PrioType>::max()};
#endif

    uint64_t batchSize;

  public:

    MultiQueueIO(uint64_t numQueues, uint64_t numThreads, uint64_t batch)
        : queues(numQueues)
        , NUM_QUEUES(numQueues)
        , threadNum(numThreads)
        , numEmpty(numQueues)
        , batchSize(batch) {}

#ifdef PERF
    uint64_t __attribute__ ((noinline)) ThreadLocalRandom() {
#else
    uint64_t ThreadLocalRandom() {
#endif
        // static thread_local std::mt19937_64 generator;
        // std::uniform_real_distribution<> distribution(min,max);
        // return distribution(generator);
        static uint64_t modMask = NUM_QUEUES - 1;
        static thread_local uint64_t x = pthread_self();
        uint64_t z = (x += UINT64_C(0x9E3779B97F4A7C15));
        z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
        z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
        return (z ^ (z >> 31)) & modMask;
    }

#ifdef PERF
    void __attribute__ ((noinline)) pushInt(uint64_t queue, PQElement item) {
        queues[queue].pq.push(item);
    }
#endif

#ifdef PERF
    void __attribute__ ((noinline)) push(PQElement item) {
#else
    inline void push(PQElement item) {
#endif
        uint64_t queue;
        while (true) {
            queue = ThreadLocalRandom();
            if (!queues[queue].try_lock()) break;
        }
        auto& q = queues[queue];
        q.pushes++;
        if (q.pq.empty())
            numEmpty.fetch_sub(1, std::memory_order_relaxed);
#ifdef PERF
        pushInt(queue, item);
#else
        q.pq.push(item);
#endif
        q.min.store(
            q.pq.size() > 0
                ? std::get<0>(q.pq.top())
                : EMPTY_PRIO,
            std::memory_order_release
        );
        q.unlock();
    }

#ifdef PERF
    void __attribute__ ((noinline)) pushBatch(uint64_t size, PQElement *items) {
#else
    inline void pushBatch(uint64_t size, PQElement *items) {
#endif
        uint64_t queue;
        while (true) {
            queue = ThreadLocalRandom();
            if (!queues[queue].try_lock()) break;
        }
        auto& q = queues[queue];
        q.pushes += size;
        if (q.pq.empty())
            numEmpty.fetch_sub(1, std::memory_order_relaxed);
        for (uint64_t i = 0; i < size; i++) {
#ifdef PERF
            pushInt(queue, items[i]);
#else
            q.pq.push(items[i]);
#endif
        }
        q.min.store(
            q.pq.size() > 0
                ? std::get<0>(q.pq.top())
                : EMPTY_PRIO,
            std::memory_order_release
        );
        q.unlock();
    }

    // Simplified Termination detection idea from the 2021 MultiQueue paper:
    // Repeatedly try popping and stop when numIdle >= threadNum,
    // That is, stop when all threads agree that there are no more work
#ifdef PERF
    boost::optional<PQElement> __attribute__ ((noinline)) tryPop() {
#else
    inline std::optional<PQElement> tryPop() {
#endif
        auto item = pop();
        if (item) return item;

        // increment count and keep on trying to pop
        uint64_t num = numIdle.fetch_add(1, std::memory_order_relaxed) + 1;
        do {
            item = pop();
            if (item) break;
            if (num >= threadNum) return {};

            num = numIdle.load(std::memory_order_relaxed);

        } while (true);

        numIdle.fetch_sub(1, std::memory_order_relaxed);
        return item;
    }

#ifdef MQ_IO_ENABLE_CLEAR_FOR_POP
    inline void setMinPrioForPop(PrioType newMinPrio) {
        PrioType oldMinPrio = minPrioForPop.load(std::memory_order_relaxed);
        while (compare({oldMinPrio, 0}, {newMinPrio, 0}) /* old > new */ &&
               !minPrioForPop.compare_exchange_weak(oldMinPrio, newMinPrio)) ;
    }
#endif

#ifdef PERF
    boost::optional<PQElement> __attribute__ ((noinline)) pop() {
#else
    inline std::optional<PQElement> pop() {
#endif
        uint64_t poppingQueue = NUM_QUEUES;
        while (true) {
            // Pick the higher priority max of queue i and j
            uint64_t i = ThreadLocalRandom();
            uint64_t j = ThreadLocalRandom();
            while (j == i) {
                j = ThreadLocalRandom();
            }

            PrioType minI = queues[i].min.load(std::memory_order_acquire);
            PrioType minJ = queues[j].min.load(std::memory_order_acquire);

            if (minI == EMPTY_PRIO && minJ == EMPTY_PRIO) {
                uint64_t emptyQueues = numEmpty.load(std::memory_order_relaxed);
                if (emptyQueues >= queues.size()) break;
                else continue;
            }

            if (minI != EMPTY_PRIO && minJ != EMPTY_PRIO) {
                poppingQueue = compare({minJ, 0}, {minI, 0}) ? i : j;
            } else if (minJ == EMPTY_PRIO) {
                poppingQueue = i;
            } else {
                poppingQueue = j;
            }
            if (queues[poppingQueue].try_lock()) continue;
            auto& q = queues[poppingQueue];
            if (!q.pq.empty()) {
#ifdef MQ_IO_ENABLE_CLEAR_FOR_POP
                PrioType minPrio = minPrioForPop.load(std::memory_order_acquire);
                if (compare(q.pq.top(), {minPrio, 0})) {
                    q.pq.clear();
                    // do not add `q.pops` on purpose
                    numEmpty.fetch_add(1, std::memory_order_relaxed);
                    q.min.store(EMPTY_PRIO, std::memory_order_release);
                } else {
#endif
                    PQElement retItem = q.pq.top();
                    q.pq.pop();
                    q.pops++;
                    if (q.pq.empty())
                        numEmpty.fetch_add(1, std::memory_order_relaxed);
                    q.min.store(
                        q.pq.size() > 0
                            ? std::get<0>(q.pq.top())
                            : EMPTY_PRIO,
                        std::memory_order_release
                    );
                    q.unlock();
                    return retItem;
#ifdef MQ_IO_ENABLE_CLEAR_FOR_POP
                }
#endif
            }
            q.unlock();
        }
        return {};
    }

#ifdef PERF
    boost::optional<uint64_t> __attribute__ ((noinline)) tryPopBatch(PQElement* ret) {
#else
    inline std::optional<uint64_t> tryPopBatch(PQElement* ret) {
#endif
        auto item = popBatch(ret);
        if (item) return item;

        // increment count and keep on trying to pop
        uint64_t num = numIdle.fetch_add(1, std::memory_order_relaxed) + 1;
        do {
            item = popBatch(ret);
            if (item) break;
            if (num >= threadNum) return {};

            num = numIdle.load(std::memory_order_relaxed);

        } while (true);

        numIdle.fetch_sub(1, std::memory_order_relaxed);
        return item;
    }

#ifdef PERF
    void __attribute__ ((noinline)) popInt(uint64_t queue, PQElement *ret) {
        auto& q = queues[queue];
        *ret = q.pq.top();
        q.pq.pop();
    }
#endif


#ifdef PERF
    boost::optional<uint64_t> __attribute__ ((noinline)) popBatch(PQElement* ret) {
#else
    inline std::optional<uint64_t> popBatch(PQElement* ret) {
#endif
        uint64_t poppingQueue = NUM_QUEUES;
        while (true) {
            // Pick the higher priority max of queue i and j
            uint64_t i = ThreadLocalRandom();
            uint64_t j = ThreadLocalRandom();
            while (j == i) {
                j = ThreadLocalRandom();
            }

            PrioType minI = queues[i].min.load(std::memory_order_acquire);
            PrioType minJ = queues[j].min.load(std::memory_order_acquire);

            if (minI == EMPTY_PRIO && minJ == EMPTY_PRIO) {
                uint64_t emptyQueues = numEmpty.load(std::memory_order_relaxed);
                if (emptyQueues >= queues.size()) break;
                else continue;
            }

            if (minI != EMPTY_PRIO && minJ != EMPTY_PRIO) {
                poppingQueue = compare({minJ, 0}, {minI, 0}) ? i : j;
            } else if (minJ == EMPTY_PRIO) {
                poppingQueue = i;
            } else {
                poppingQueue = j;
            }
            if (queues[poppingQueue].try_lock()) continue;
            auto& q = queues[poppingQueue];
            if (q.pq.empty()) {
                q.unlock();
                continue;
            }

            uint64_t num = 0;
            for (num = 0; num < batchSize; num++) {
                if (q.pq.empty()) break;
#ifdef PERF
                popInt(poppingQueue, &ret[num]);
#else
                ret[num] = q.pq.top();
                q.pq.pop();
#endif

            }
            q.pops += num;
            if (q.pq.empty())
                numEmpty.fetch_add(1, std::memory_order_relaxed);
            q.min.store(
                q.pq.size() > 0
                    ? std::get<0>(q.pq.top())
                    : EMPTY_PRIO,
                std::memory_order_release
            );
            q.unlock();
            if (num == 0) continue;

            return num;
        }
        return {};
    }

    inline uint64_t getQueueOccupancy() const {
        uint64_t maxOccupancy = 0;
        for (uint64_t i = 0; i < NUM_QUEUES; i++) {
            maxOccupancy = std::max(maxOccupancy, queues[i].pq.size());
        }
        return maxOccupancy;
    }

    // Get the number of pushes to all queues.
    // Note: this is not lock protected.
    inline uint64_t getNumPushes() const {
        uint64_t totalPushes = 0;
        for (uint64_t i = 0; i < NUM_QUEUES; i++) {
            totalPushes += queues[i].pushes;
        }
        return totalPushes;
    }

    // Get the number of pops to all queues.
    // Note: this is not lock protected.
    inline uint64_t getNumPops() const {
        uint64_t totalPops = 0;
        for (uint64_t i = 0; i < NUM_QUEUES; i++) {
            totalPops += queues[i].pops;
        }
        return totalPops;
    }

    inline void stat() const {
        std::cout << "total pushes "<< getNumPushes() << "\n";
        std::cout << "total pops "<< getNumPops() << "\n";
    }

    // Note: this is only called at the end of algorithm as a
    // sanity check, therefore it is not lock protected.
    inline bool empty() const {
        for (uint i = 0; i < NUM_QUEUES;i++) {
            if (!queues[i].pq.empty()) {
                return false;
            }
        }
        return true;
    }

    // Resets the MultiQueue to a state as if it was reinitialized.
    // This must be called before using the MQ again after using TypPop().
    // Note: this assumes the queues are already empty and unlocked.
    inline void reset() {
        for (uint64_t i = 0; i < NUM_QUEUES; i++) {
            assert(queues[i].pq.empty() && "reset() assumes empty queues");
            assert((queues[i].queueLock.test(std::memory_order_relaxed) == 0)
                   && "reset() assumes unlocked queues");
            queues[i].pushes = 0;
            queues[i].pops = 0;
            queues[i].min.store(EMPTY_PRIO, std::memory_order_relaxed);
        }
        numIdle.store(0, std::memory_order_relaxed);
        numEmpty.store(NUM_QUEUES, std::memory_order_relaxed);
#ifdef MQ_IO_ENABLE_CLEAR_FOR_POP
        minPrioForPop.store(std::numeric_limits<PrioType>::max(), std::memory_order_relaxed);
#endif
    }
};
