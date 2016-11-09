#ifndef TATUM_RANGE_H
#define TATUM_RANGE_H
#include <iterator>

namespace tatum { namespace util {
/*
 * The tatum::Range template models a range defined by two iterators of type T.
 *
 * It allows conveniently returning a range from a single function call
 * without having to explicity expose the underlying container, or make two
 * explicit calls to retireive the associated begin and end iterators.
 * It also enables the easy use of range-based-for loops.
 *
 * For example:
 *      
 *      class My Data {
 *          public:
 *              typdef std::vector<int>::const_iterator my_iter;
 *              tatum::Range<my_iter> data();
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
        typedef T iterator;
    public:
        Range(T b, T e): begin_(b), end_(e) {}
        T begin() const { return begin_; }
        T end() const { return end_; }
        bool empty() const { return begin_ == end_; }
        size_t size() const { return std::distance(begin_, end_); }
    private:
        T begin_;
        T end_;
};

/*
 * Creates a tatum::Range from a pair of iterators.
 *
 *  Unlike using the tatum::Range() constructor (which requires specifying
 *  the template type T, using tatum::make_range() infers T from the arguments.
 *
 * Example usage:
 *  auto my_range = tatum::make_range(my_vec.begin(), my_vec.end());
 */
template<typename T>
Range<T> make_range(T b, T e) { return Range<T>(b, e); }

}} //namespace

#endif
