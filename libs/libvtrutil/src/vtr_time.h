#ifndef VTR_TIME_H
#define VTR_TIME_H
#include <chrono>
#include <string>

namespace vtr {

///@brief Class for tracking time elapsed since construction
class Timer {
  public:
    Timer();
    virtual ~Timer() = default;

    ///@brief No copy
    Timer(Timer&) = delete;
    Timer& operator=(Timer&) = delete;

    ///@brief No move
    Timer(Timer&&) = delete;
    Timer& operator=(Timer&&) = delete;

    ///@brief Return elapsed time in seconds
    float elapsed_sec() const;

    ///@brief Return peak memory resident set size (in MiB)
    float max_rss_mib() const;

    ///@brief Return change in peak memory resident set size (in MiB)
    float delta_max_rss_mib() const;

  private:
    using clock = std::chrono::steady_clock;
    std::chrono::time_point<clock> start_;

    size_t initial_max_rss_; //Maximum resident set size In bytes
    constexpr static float BYTE_TO_MIB = 1024 * 1024;
};

///@brief Scoped time class which prints the time elapsed for the specifid action
class ScopedActionTimer : public Timer {
  public:
    ScopedActionTimer(const std::string action);
    ~ScopedActionTimer();

    void quiet(bool value);
    bool quiet() const;
    std::string action() const;

  protected:
    int depth() const;
    std::string pad() const;

  private:
    const std::string action_;
    bool quiet_ = false;
    int depth_;
};

/**
 * @brief Scoped elapsed time class which prints the time elapsed for the specified action when it is destructed.
 *
 * For example:
 * 
 *       {
 *           vtr::ScopedFinishTimer timer("my_action");
 * 
 *           //Do other work
 * 
 *           //Will print: 'my_action took X.XX seconds' when out-of-scope
 *       }
 */
class ScopedFinishTimer : public ScopedActionTimer {
  public:
    ScopedFinishTimer(const std::string action);
    ~ScopedFinishTimer();
};

/**
 * @brief Scoped elapsed time class which prints out the action when initialized and again both the action and elapsed time
 *
 * when destructed.
 * For example:
 * 
 *       {
 *           vtr::ScopedStartFinishTimer timer("my_action") //Will print: 'my_action'
 * 
 *           //Do other work
 * 
 *           //Will print 'my_action took X.XX seconds' when out of scope
 *       }
 */
class ScopedStartFinishTimer : public ScopedActionTimer {
  public:
    ScopedStartFinishTimer(const std::string action);
    ~ScopedStartFinishTimer();
};
} // namespace vtr

#endif
