#include "crr_thread_pool.h"

#include "vtr_log.h"
#include "vtr_assert.h"

namespace crrgenerator {

// CRRThreadPool implementation

CRRThreadPool::CRRThreadPool(size_t num_threads) {
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
    } else {
        num_threads = std::min(
            num_threads, static_cast<size_t>(std::thread::hardware_concurrency()));
    }

    if (num_threads == 0) {
        num_threads = 1;
    }

    VTR_LOG("Creating CRRThreadPool with %zu threads\n", num_threads);

    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back(&CRRThreadPool::worker_thread, this);
    }
}

CRRThreadPool::~CRRThreadPool() {
    stop();

    // Wake up all threads
    condition_.notify_all();

    // Wait for all threads to finish
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void CRRThreadPool::wait_for_all() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    condition_.wait(
        lock, [this] { return tasks_.empty() && active_tasks_.load() == 0; });
}

size_t CRRThreadPool::get_pending_tasks() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return tasks_.size();
}

void CRRThreadPool::stop() { stop_.store(true); }

void CRRThreadPool::worker_thread() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);

            condition_.wait(lock, [this] { return stop_.load() || !tasks_.empty(); });

            if (stop_.load() && tasks_.empty()) {
                break;
            }

            if (!tasks_.empty()) {
                task = std::move(tasks_.front());
                tasks_.pop();
                active_tasks_.fetch_add(1);
            }
        }

        if (task) {
            try {
                task();
            } catch (const std::exception& e) {
                VTR_LOG_ERROR("Exception in worker thread: %s\n", e.what());
            } catch (...) {
                VTR_LOG_ERROR("Unknown exception in worker thread\n");
            }

            active_tasks_.fetch_sub(1);
            completed_tasks_.fetch_add(1);
            condition_.notify_all();
        }
    }
}

// ProgressTracker implementation

ProgressTracker::ProgressTracker(size_t total_tasks,
                                 const std::string& operation_name)
    : total_(total_tasks)
    , operation_name_(operation_name)
    , start_time_(std::chrono::steady_clock::now()) {
    VTR_LOG("Starting %s: %zu tasks\n", operation_name_.c_str(), total_);
}

void ProgressTracker::increment(size_t count) {
    size_t new_completed = completed_.fetch_add(count) + count;

    // Log progress at intervals
    if (new_completed % std::max(size_t(1), total_ / 20) == 0 || new_completed == total_) {
        log_progress();
    }
}

double ProgressTracker::get_progress_percentage() const {
    if (total_ == 0) return 100.0;
    return (static_cast<double>(completed_.load()) / total_) * 100.0;
}

void ProgressTracker::log_progress() const {
    std::lock_guard<std::mutex> lock(log_mutex_);

    size_t current_completed = completed_.load();
    double percentage = get_progress_percentage();

    auto now = std::chrono::steady_clock::now();
    auto elapsed =
        std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);

    if (current_completed < total_) {
        // Calculate ETA
        if (current_completed > 0) {
            auto total_estimated = elapsed * total_ / current_completed;
            auto eta = total_estimated - elapsed;

            VTR_LOG("%s: %zu/%zu (%.1f%%) - Elapsed: %ds, ETA: %ds\n",
                    operation_name_.c_str(), current_completed, total_, percentage,
                    elapsed.count(), eta.count());
        } else {
            VTR_LOG("%s: %zu/%zu (%.1f%%) - Elapsed: %ds\n", operation_name_.c_str(),
                    current_completed, total_, percentage, elapsed.count());
        }
    } else {
        VTR_LOG("%s: Complete! %zu/%zu tasks in %ds\n", operation_name_.c_str(),
                current_completed, total_, elapsed.count());
    }
}

} // namespace crrgenerator
