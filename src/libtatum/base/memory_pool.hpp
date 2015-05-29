#pragma once

//XXX: This is a somewhat hacky trick to divorce boost
//     pool from its dependance on boost::mutex, which
//     pulls in boost system and boost date time.
//     We instead override using boost::mutex and instead
//     use C++11 mutexs
#define BOOST_THREAD_MUTEX_HPP
#include <mutex>
namespace boost {
    using std::mutex;
}

#include <boost/pool/pool.hpp>

//Isolate downstream code from boost
typedef boost::pool<> MemoryPool;


class MutexMemoryPool {
    public:
        MutexMemoryPool(MemoryPool::size_type obj_size)
            : memory_pool_(obj_size) {}

        void* malloc() {
            std::lock_guard<std::mutex> lock(mutex_);
            return memory_pool_.malloc();
        }

        bool purge_memory() {
            std::lock_guard<std::mutex> lock(mutex_);
            return memory_pool_.purge_memory();
        }

        MemoryPool::size_type get_requested_size() {
            std::lock_guard<std::mutex> lock(mutex_);
            return memory_pool_.get_requested_size();
        }

    private:
        //FIXME: Single mutex is not likely not parallel scalable...
        std::mutex mutex_;
        MemoryPool memory_pool_;
};
