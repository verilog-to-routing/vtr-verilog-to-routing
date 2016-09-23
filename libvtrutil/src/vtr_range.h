#ifndef VTR_RANGE_H
#define VTR_RANGE_H
#include <iterator>

namespace vtr {
    template<typename T>
    class Range {
        public:
            Range(T b, T e): begin_(b), end_(e) {}
            T begin() { return begin_; }
            T end() { return end_; }
            bool empty() { return begin_ == end_; }
            size_t size() { return std::distance(begin_, end_); }
        private:
            T begin_;
            T end_;
    };

    template<typename T>
    Range<T> make_range(T b, T e) { return Range<T>(b, e); }
}

#endif
