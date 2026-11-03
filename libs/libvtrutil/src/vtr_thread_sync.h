#pragma once

/**
 * @file vtr_thread_sync.h
 * @brief Thread synchronization primitives shared by VTR's parallel algorithms.
 *
 * Contains a spin lock and several thread barrier implementations. The
 * barriers share the same interface (constructor taking the thread count,
 * init(), wait()) so callers can switch between them with a type alias.
 */

#include <atomic>
#include <barrier>
#include <condition_variable>
#include <cstddef>
#include <mutex>

namespace vtr {

/**
 * @brief Spin lock implementation using std::atomic_flag
 *
 * Intended for fine-grained locking where contention is rare (e.g. one lock
 * per RR node in the parallel router), so spinning is cheaper than blocking.
 */
class spin_lock_t {
    /// Atomic flag used for the lock implementation
    std::atomic_flag lock_ = ATOMIC_FLAG_INIT;

  public:
    /// @brief Acquires the spin lock, repeatedly attempting until successful
    void acquire() {
        while (std::atomic_flag_test_and_set_explicit(&lock_, std::memory_order_acquire))
            ;
    }

    /// @brief Releases the spin lock, allowing other threads to acquire it
    void release() {
        std::atomic_flag_clear_explicit(&lock_, std::memory_order_release);
    }
};

/**
 * @brief Thread barrier implementation using std::mutex
 *
 * It ensures all participating threads reach a synchronization point
 * before any are allowed to proceed further. It uses a mutex and
 * condition variable to coordinate thread synchronization.
 */
class barrier_mutex_t {
    std::mutex mutex_;
    std::condition_variable cv_;
    size_t count_;
    size_t max_count_;
    size_t generation_ = 0;

  public:
    /**
     * @brief Constructs a barrier for a specific number of threads
     * @param num_threads Number of threads that must call wait() before
     * any thread is allowed to proceed
     */
    explicit inline barrier_mutex_t(size_t num_threads)
        : count_(num_threads)
        , max_count_(num_threads) {}

    /// Initialization method goes unused by this barrier implementation.
    inline void init() {}

    /**
     * @brief Blocks the calling thread until all threads have called wait()
     *
     * When the specified number of threads have called this method, all
     * threads are unblocked and the barrier is reset for the next use.
     */
    inline void wait() {
        std::unique_lock<std::mutex> lock{mutex_};
        size_t gen = generation_;
        if (--count_ == 0) {
            generation_++;
            count_ = max_count_;
            cv_.notify_all();
        } else {
            cv_.wait(lock, [this, &gen] { return gen != generation_; });
        }
    }
};

/**
 * @brief Spin-based thread barrier implementation using std::atomic
 *
 * It ensures all participating threads reach a synchronization point
 * before any are allowed to proceed further. It uses atomic operations
 * to implement Sense-Reversing Centralized Barrier (from Section 5.2.1
 * of Michael L. Scott's textbook) without using mutex locks.
 */
class barrier_spin_t {
    /// Number of threads that must reach the barrier
    size_t num_threads_ = 1;

    /// Atomic counter tracking the number of threads that have arrived at the barrier
    std::atomic<size_t> count_ = 0;

    /// Global sense shared by all participating threads
    std::atomic<bool> sense_ = false;

    /// Thread-local sense value for each participating thread
    inline static thread_local bool local_sense_ = false;

  public:
    /**
     * @brief Constructs a barrier for a specific number of threads
     * @param num_threads Number of threads that must call wait() before
     * any thread is allowed to proceed
     */
    explicit inline barrier_spin_t(size_t num_threads) { num_threads_ = num_threads; }

    /**
     * @brief Initializes the thread-local sense flag
     * @note Should be called by each thread before first using the barrier.
     */
    inline void init() {
        local_sense_ = false;
    }

    /**
     * @brief Blocks the calling thread until all threads have called wait()
     *
     * Uses a sense-reversing algorithm to synchronize threads. The last thread
     * to arrive unblocks all waiting threads. This method avoids using locks or
     * condition variables, making it potentially more efficient for short waits.
     */
    inline void wait() {
        bool s = !local_sense_;
        local_sense_ = s;
        size_t num_arrivals = count_.fetch_add(1) + 1;
        if (num_arrivals == num_threads_) {
            count_.store(0);
            sense_.store(s);
        } else {
            while (sense_.load() != s)
                ; // spin until the last thread arrives
        }
    }
};

/**
 * @brief Thread barrier implementation using std::barrier
 *
 * It ensures all participating threads reach a synchronization point
 * before any are allowed to proceed further.
 */
class standard_barrier_t {
    /// @brief Internal barrier implementation.
    std::barrier<> barrier_;

  public:
    /**
     * @brief Constructs a barrier for a specific number of threads
     *
     *  @param num_threads
     *      Number of threads that must call wait() before any thread is allowed
     *      to proceed.
     */
    explicit inline standard_barrier_t(size_t num_threads)
        : barrier_(num_threads) {}

    /// Initialization method goes unused by this barrier implementation.
    inline void init() {}

    /// @brief Blocks the calling thread until all threads have called wait()
    inline void wait() {
        barrier_.arrive_and_wait();
    }
};

} // namespace vtr
