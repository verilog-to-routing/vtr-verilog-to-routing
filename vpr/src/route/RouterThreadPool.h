#pragma once

/** 
 * @file RouterThreadPool.h
 * @brief A generic thread pool for parallel task execution.
 * 
 * This class manages a pool of threads and allows tasks to be executed in parallel.
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

class RouterThreadPool {
private:
    struct ThreadData {
        std::thread thread;
        std::queue<std::function<void()>> task_queue;
        std::mutex queue_mutex;
        std::condition_variable cv;
        bool stop = false;
        size_t thread_id;  // For debugging
        size_t tasks_completed{0};  // Track number of completed tasks

        ThreadData(size_t id) : thread_id(id) {}
    };

    std::vector<std::unique_ptr<ThreadData>> threads;
    std::atomic<size_t> next_thread{0};  // For round-robin assignment
    std::atomic<size_t> total_tasks_queued{0};
    vtr::Timer pool_timer;  // Track pool lifetime
    std::atomic<size_t> active_tasks{0};
    std::mutex completion_mutex;
    std::condition_variable completion_cv;

public:
    RouterThreadPool(size_t thread_count) {
        // VTR_LOG("Creating thread pool with %zu threads\n", thread_count);
        threads.reserve(thread_count);
        
        for (size_t i = 0; i < thread_count; i++) {
            auto thread_data = std::make_unique<ThreadData>(i);
            
            thread_data->thread = std::thread([this, td = thread_data.get()]() {
                // VTR_LOG("Thread %zu started\n", td->thread_id);
                
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(td->queue_mutex);
                        // if (!td->task_queue.empty()) {
                        //     VTR_LOG("Thread %zu has %zu tasks queued\n", 
                        //            td->thread_id, td->task_queue.size());
                        // }
                        
                        td->cv.wait(lock, [td]() {
                            return td->stop || !td->task_queue.empty();
                        });
                        
                        if (td->stop && td->task_queue.empty()) {
                            // VTR_LOG("Thread %zu stopping after completing %zu tasks\n", 
                            //        td->thread_id, td->tasks_completed);
                            return;
                        }
                        
                        task = std::move(td->task_queue.front());
                        td->task_queue.pop();
                    }
                    
                    vtr::Timer task_timer;
                    task();
                    td->tasks_completed++;
                    // VTR_LOG("Thread %zu completed task %zu in %.3f seconds\n", 
                    //        td->thread_id, td->tasks_completed, task_timer.elapsed_sec());
                }
            });
            
            threads.push_back(std::move(thread_data));
        }
        // VTR_LOG("Thread pool initialization completed in %.3f seconds\n", 
        //        pool_timer.elapsed_sec());
    }

    // Schedule work and get future for result
    template<typename F>
    void schedule_work(F&& f) {
        active_tasks++;
        
        // Round-robin thread assignment
        size_t thread_idx = (next_thread++) % threads.size();
        auto thread_data = threads[thread_idx].get();
        size_t task_id = ++total_tasks_queued;
        
        // VTR_LOG("Scheduling task %zu to thread %zu\n", task_id, thread_data->thread_id);
        
        // Wrap the work with task completion tracking
        auto task = [this, f = std::forward<F>(f), thread_id = thread_data->thread_id, task_id]() {
            vtr::Timer task_timer;
            // VTR_LOG("Thread %zu starting task %zu\n", thread_id, task_id);
            
            try {
                f();
                // VTR_LOG("Thread %zu completed task %zu successfully in %.3f seconds\n", 
                //        thread_id, task_id, task_timer.elapsed_sec());
            } catch (const std::exception& e) {
                VTR_LOG_ERROR("Thread %zu failed task %zu with error: %s\n", 
                             thread_id, task_id, e.what());
                throw;
            } catch (...) {
                VTR_LOG_ERROR("Thread %zu failed task %zu with unknown error\n", 
                             thread_id, task_id);
                throw;
            }

            // Track task completion
            size_t remaining = --active_tasks;
            if (remaining == 0) {
                completion_cv.notify_all();
            }
        };
        
        // Queue the task
        {
            std::lock_guard<std::mutex> lock(thread_data->queue_mutex);
            thread_data->task_queue.push(std::move(task));
            // VTR_LOG("Task %zu queued to thread %zu (queue size: %zu)\n", 
            //        task_id, thread_data->thread_id, thread_data->task_queue.size());
        }
        thread_data->cv.notify_one();
    }

    void wait_for_all() {
        std::unique_lock<std::mutex> lock(completion_mutex);
        completion_cv.wait(lock, [this]() { return active_tasks == 0; });
    }

    ~RouterThreadPool() {
        // VTR_LOG("Shutting down thread pool after %.3f seconds, processed %zu total tasks\n", 
        //        pool_timer.elapsed_sec(), total_tasks_queued.load());
        
        // Signal all threads to stop
        for (auto& thread_data : threads) {
            {
                std::lock_guard<std::mutex> lock(thread_data->queue_mutex);
                thread_data->stop = true;
                // VTR_LOG("Signaling thread %zu to stop (remaining tasks: %zu)\n", 
                //        thread_data->thread_id, thread_data->task_queue.size());
            }
            thread_data->cv.notify_one();
        }
        
        // Join all threads
        for (auto& thread_data : threads) {
            if (thread_data->thread.joinable()) {
                thread_data->thread.join();
                // VTR_LOG("Thread %zu joined after completing %zu tasks\n", 
                //        thread_data->thread_id, thread_data->tasks_completed);
            }
        }
    }
};
