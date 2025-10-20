#pragma once

#include <future>

#include "crr_common.h"

namespace crrgenerator {

/**
 * @brief Simple thread pool implementation for parallel processing
 *
 * This class provides a thread pool for executing tasks in parallel,
 * with progress tracking and synchronization support.
 */
class CRRThreadPool {
 public:
  /**
   * @brief Constructor
   * @param num_threads Number of worker threads (0 = hardware concurrency)
   */
  explicit CRRThreadPool(size_t num_threads = 0);

  /**
   * @brief Destructor - waits for all tasks to complete
   */
  ~CRRThreadPool();

  /**
   * @brief Submit a task to the thread pool
   * @param func Function to execute
   * @param args Arguments to pass to the function
   * @return Future for the result
   */
  template <typename Func, typename... Args>
  auto submit(Func&& func, Args&&... args)
      -> std::future<std::invoke_result_t<Func, Args...>>;

  /**
   * @brief Wait for all submitted tasks to complete
   */
  void wait_for_all();

  /**
   * @brief Get the number of worker threads
   * @return Number of threads
   */
  size_t get_thread_count() const { return workers_.size(); }

  /**
   * @brief Get the number of pending tasks
   * @return Number of tasks in queue
   */
  size_t get_pending_tasks() const;

  /**
   * @brief Stop the thread pool (no new tasks accepted)
   */
  void stop();

  /**
   * @brief Check if the thread pool is stopped
   * @return true if stopped
   */
  bool is_stopped() const { return stop_; }

 private:
  // Worker threads
  std::vector<std::thread> workers_;

  // Task queue
  std::queue<std::function<void()>> tasks_;

  // Synchronization
  mutable std::mutex queue_mutex_;
  std::condition_variable condition_;
  std::atomic<bool> stop_{false};

  // Progress tracking
  std::atomic<size_t> active_tasks_{0};
  std::atomic<size_t> completed_tasks_{0};

  // Worker function
  void worker_thread();
};

/**
 * @brief Progress tracker for parallel operations
 *
 * Thread-safe progress tracking with logging support.
 */
class ProgressTracker {
 public:
  ProgressTracker(size_t total_tasks,
                  const std::string& operation_name = "Processing");

  /**
   * @brief Increment completed task count
   * @param count Number of tasks completed (default: 1)
   */
  void increment(size_t count = 1);

  /**
   * @brief Get current progress as percentage
   * @return Progress percentage (0.0 - 100.0)
   */
  double get_progress_percentage() const;

  /**
   * @brief Get number of completed tasks
   * @return Completed task count
   */
  size_t get_completed() const { return completed_.load(); }

  /**
   * @brief Get total number of tasks
   * @return Total task count
   */
  size_t get_total() const { return total_; }

  /**
   * @brief Check if all tasks are completed
   * @return true if complete
   */
  bool is_complete() const { return completed_.load() >= total_; }

  /**
   * @brief Log current progress
   */
  void log_progress() const;

 private:
  std::atomic<size_t> completed_{0};
  size_t total_;
  std::string operation_name_;
  mutable std::mutex log_mutex_;
  std::chrono::steady_clock::time_point start_time_;
};

// Template implementation for CRRThreadPool::submit
template <typename Func, typename... Args>
auto CRRThreadPool::submit(Func&& func, Args&&... args)
    -> std::future<std::invoke_result_t<Func, Args...>> {
  using return_type = std::invoke_result_t<Func, Args...>;

  auto task = std::make_shared<std::packaged_task<return_type()>>(
      std::bind(std::forward<Func>(func), std::forward<Args>(args)...));

  std::future<return_type> result = task->get_future();

  {
    std::unique_lock<std::mutex> lock(queue_mutex_);

    if (stop_) {
      throw std::runtime_error("Cannot submit task to stopped CRRThreadPool");
    }

    tasks_.emplace([task]() { (*task)(); });
  }

  condition_.notify_one();
  return result;
}

}  // namespace crrgenerator
