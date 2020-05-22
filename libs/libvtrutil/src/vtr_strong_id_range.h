#ifndef _VTR_STRONG_ID_RANGE_H
#define _VTR_STRONG_ID_RANGE_H

#include <algorithm>
#include "vtr_assert.h"

namespace vtr {

/*
 * This header defines a utility class for StrongId's.  StrongId's are
 * described in vtr_strong_id.h.  In some cases, StrongId's be considered
 * like random access iterators, but not all StrongId's have this property.
 * In addition, there is utility in refering to a range of id's, and being able
 * to iterator over that range.
 *
 * StrongIdIterator allows a StrongId to be treated like a random access
 * iterator.  Whether this is a correct use of the abstraction is up to the
 * called.
 *
 * StrongIdRange allows a pair of StrongId's to defines a continguous range of
 * ids.  The "end" StrongId is excluded from this range.
 */
template<typename StrongId>
class StrongIdIterator {
  public:
    StrongIdIterator() = default;
    StrongIdIterator& operator=(const StrongIdIterator& other) = default;
    StrongIdIterator(const StrongIdIterator& other) = default;
    explicit StrongIdIterator(StrongId id)
        : id_(id) {
        VTR_ASSERT(bool(id));
    }

    using iterator_category = std::random_access_iterator_tag;
    using value_type = StrongId;
    using reference = StrongId&;
    using pointer = StrongId*;
    using difference_type = ssize_t;

    StrongId& operator*() {
        VTR_ASSERT_SAFE(bool(id_));
        return this->id_;
    }

    StrongIdIterator& operator+=(ssize_t n) {
        VTR_ASSERT_SAFE(bool(id_));
        id_ = StrongId(size_t(id_) + n);
        VTR_ASSERT_SAFE(bool(id_));
        return *this;
    }

    StrongIdIterator& operator-=(ssize_t n) {
        VTR_ASSERT_SAFE(bool(id_));
        id_ = StrongId(size_t(id_) - n);
        VTR_ASSERT_SAFE(bool(id_));
        return *this;
    }

    StrongIdIterator& operator++() {
        VTR_ASSERT_SAFE(bool(id_));
        *this += 1;
        VTR_ASSERT_SAFE(bool(id_));
        return *this;
    }

    StrongIdIterator& operator--() {
        VTR_ASSERT_SAFE(bool(id_));
        *this -= 1;
        VTR_ASSERT_SAFE(bool(id_));
        return *this;
    }

    StrongId operator[](ssize_t offset) const {
        return StrongId(size_t(id_) + offset);
    }

    template<typename IdType>
    friend StrongIdIterator<IdType> operator+(
        const StrongIdIterator<IdType>& lhs,
        ssize_t n) {
        StrongIdIterator ret = lhs;
        ret += n;
        return ret;
    }

    template<typename IdType>
    friend StrongIdIterator<IdType> operator-(
        const StrongIdIterator<IdType>& lhs,
        ssize_t n) {
        StrongIdIterator ret = lhs;
        ret -= n;
        return ret;
    }

    template<typename IdType>
    friend ssize_t operator-(
        const StrongIdIterator<IdType>& lhs,
        const StrongIdIterator<IdType>& rhs) {
        VTR_ASSERT_SAFE(bool(lhs.id_));
        VTR_ASSERT_SAFE(bool(rhs.id_));

        ssize_t ret = size_t(lhs.id_);
        ret -= size_t(rhs.id_);
        return ret;
    }

    template<typename IdType>
    friend bool operator==(const StrongIdIterator<IdType>& lhs, const StrongIdIterator<IdType>& rhs) {
        return lhs.id_ == rhs.id_;
    }

    template<typename IdType>
    friend bool operator!=(const StrongIdIterator<IdType>& lhs, const StrongIdIterator<IdType>& rhs) {
        return lhs.id_ != rhs.id_;
    }

    template<typename IdType>
    friend bool operator<(const StrongIdIterator<IdType>& lhs, const StrongIdIterator<IdType>& rhs) {
        return lhs.id_ < rhs.id_;
    }

  private:
    StrongId id_;
};

template<typename StrongId>
class StrongIdRange {
  public:
    StrongIdRange(StrongId b, StrongId e)
        : begin_(b)
        , end_(e) {
        VTR_ASSERT(begin_ < end_ || begin_ == end_);
    }
    StrongIdIterator<StrongId> begin() const {
        return StrongIdIterator<StrongId>(begin_);
    }
    StrongIdIterator<StrongId> end() const {
        return StrongIdIterator<StrongId>(end_);
    }

    bool empty() { return begin_ == end_; }
    size_t size() {
        return std::distance(begin(), end());
    }

  private:
    StrongId begin_;
    StrongId end_;
};

} //namespace vtr

#endif /* _VTR_STRONG_ID_RANGE_H */
