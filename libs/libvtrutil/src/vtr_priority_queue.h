#pragma once

#include <functional>
#include <queue>
#include <vector>

namespace vtr {

/**
 * @brief A std::priority_queue that can be emptied with clear().
 *
 * clear() removes every element and keeps the memory of the underlying container.
 */
template<class T, class Container = std::vector<T>, class Compare = std::less<typename Container::value_type>>
class priority_queue : public std::priority_queue<T, Container, Compare> {
  public:
    using std::priority_queue<T, Container, Compare>::priority_queue;

    /// @brief Removes every element.
    void clear() { this->c.clear(); }
};

} // namespace vtr
