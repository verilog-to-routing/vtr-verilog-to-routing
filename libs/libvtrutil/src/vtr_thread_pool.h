#pragma once

/** 
 * @file vtr_thread_pool.h
 * @brief A generic thread pool for parallel task execution
 */

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>
#include <functional>
#include <cstddef>
#include <vector>
#include "vtr_log.h"
#include "vtr_time.h"

namespace vtr {

/**
 * A thread pool for parallel task execution. It is a naive
 * implementation which uses a queue for each thread and assigns
 * tasks in a round robin fashion.
 *
 * Example usage:
 *
 * vtr::thread_pool pool(4);
 * pool.schedule_work([]{
 *     // Task body
 * });
 * pool.wait_for_all(); // There's no API to wait for a single task
 */
class thread_pool {
  private:
    /* Thread-local data */
    struct ThreadData {
        std::thread thread;
        /* Per-thread task queue */
        std::queue<std::function<void()>> task_queue;

        /* Threads wait on cv for a stop signal or a new task
         * queue_mutex is required for condition variable */
        std::mutex queue_mutex;
        std::condition_variable cv;
        bool stop = false;
    };

    /* Container for thread-local data */
    std::vector<std::unique_ptr<ThreadData>> threads;
    /* Used for round-robin scheduling */
    std::atomic<size_t> next_thread{0};
    /* Used for wait_for_all */
    std::atomic<size_t> active_tasks{0};

    /* Condition variable for wait_for_all */
    std::mutex completion_mutex;
    std::condition_variable completion_cv;

  public:
    thread_pool(size_t thread_count) {
        threads.reserve(thread_count);

        for (size_t i = 0; i < thread_count; i++) {
            auto thread_data = std::make_unique<ThreadData>();

            thread_data->thread = std::thread([&]() {
                ThreadData* td = thread_data.get();

                while (true) {
                    std::function<void()> task;

                    { /* Wait until a task is available or stop signal is received */
                        std::unique_lock<std::mutex> lock(td->queue_mutex);

                        td->cv.wait(lock, [td]() {
                            return td->stop || !td->task_queue.empty();
                        });

                        if (td->stop && td->task_queue.empty()) {
                            return;
                        }

                        /* Fetch a task from the queue */
                        task = std::move(td->task_queue.front());
                        td->task_queue.pop();
                    }

                    vtr::Timer task_timer;
                    task();
                }
            });

            threads.push_back(std::move(thread_data));
        }
    }

    template<typename F>
    void schedule_work(F&& f) {
        active_tasks++;

        /* Round-robin thread assignment */
        size_t thread_idx = (next_thread++) % threads.size();
        auto thread_data = threads[thread_idx].get();

        auto task = [this, f = std::forward<F>(f)]() {
            vtr::Timer task_timer;

            try {
                f();
            } catch (const std::exception& e) {
                VTR_LOG_ERROR("Thread %zu failed task with error: %s\n",
                              std::this_thread::get_id(), e.what());
                throw;
            } catch (...) {
                VTR_LOG_ERROR("Thread %zu failed task with unknown error\n",
                              std::this_thread::get_id());
                throw;
            }

            size_t remaining = --active_tasks;
            if (remaining == 0) {
                completion_cv.notify_all();
            }
        };

        /* Queue new task */
        {
            std::lock_guard<std::mutex> lock(thread_data->queue_mutex);
            thread_data->task_queue.push(std::move(task));
        }
        thread_data->cv.notify_one();
    }

    void wait_for_all() {
        std::unique_lock<std::mutex> lock(completion_mutex);
        completion_cv.wait(lock, [this]() { return active_tasks == 0; });
    }

    ~thread_pool() {
        /* Stop all threads */
        for (auto& thread_data : threads) {
            {
                std::lock_guard<std::mutex> lock(thread_data->queue_mutex);
                thread_data->stop = true;
            }
            thread_data->cv.notify_one();
        }

        for (auto& thread_data : threads) {
            if (thread_data->thread.joinable()) {
                thread_data->thread.join();
            }
        }
    }
};

} // namespace vtr
