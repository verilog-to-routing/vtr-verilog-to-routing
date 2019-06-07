#ifndef VTR_RANGE_H
#define VTR_RANGE_H
#include <iterator>

namespace vtr {
/*
 * The vtr::Range template models a range defined by two iterators of type T.
 *
 * It allows conveniently returning a range from a single function call
 * without having to explicity expose the underlying container, or make two
 * explicit calls to retrieve the associated begin and end iterators.
 * It also enables the easy use of range-based-for loops.
 *
 * For example:
 *
 *      class My Data {
 *          public:
 *              typdef std::vector<int>::const_iterator my_iter;
 *              vtr::Range<my_iter> data();
 *          ...
 *          private:
 *              std::vector<int> data_;
 *      };
 *
 *      ...
 *
 *      MyDat my_data;
 *
 *      //fill my_data
 *
 *      for(int val : my_data.data()) {
 *          //work with values stored in my_data
 *      }
 *
 * The empty() and size() methods are convenience wrappers around the relevant
 * iterator comparisons.
 *
 * Note that size() is only constant time if T is a random-access iterator!
 */
template<typename T>
class Range {
  public:
    Range(T b, T e)
        : begin_(b)
        , end_(e) {}
    T begin() { return begin_; }
    T end() { return end_; }
    bool empty() { return begin_ == end_; }
    size_t size() { return std::distance(begin_, end_); }

  private:
    T begin_;
    T end_;
};

/*
 * Creates a vtr::Range from a pair of iterators.
 *
 *  Unlike using the vtr::Range() constructor (which requires specifying
 *  the template type T, using vtr::make_range() infers T from the arguments.
 *
 * Example usage:
 *  auto my_range = vtr::make_range(my_vec.begin(), my_vec.end());
 */
template<typename T>
auto make_range(T b, T e) { return Range<T>(b, e); }

/*
 * Creates a vtr::Range from a container
 */
template<typename Container>
auto make_range(const Container& c) { return make_range(std::begin(c), std::end(c)); }

} // namespace vtr

#endif
