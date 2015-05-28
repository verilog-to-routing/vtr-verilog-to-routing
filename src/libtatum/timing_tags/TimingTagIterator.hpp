#pragma once
#include <boost/iterator/iterator_facade.hpp>

class TimingTags;
class TimingTag;

template <class Value>
class TimingTagIter : public boost::iterator_facade<TimingTagIter<Value>, Value, boost::forward_traversal_tag> {
    private:
        Value* p;
    public:
        TimingTagIter(): p(nullptr) {}
        explicit TimingTagIter(Value* tag): p(tag) {}
    private:
        friend class boost::iterator_core_access;

        void increment() { p = p->next(); }
        bool equal(TimingTagIter<Value> const& other) const { return p == other.p; }
        Value& dereference() const { return *p; }
};

typedef TimingTagIter<TimingTag> TimingTagIterator;
typedef TimingTagIter<TimingTag const> TimingTagConstIterator;
