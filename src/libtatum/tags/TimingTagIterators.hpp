#pragma once
#include <boost/iterator/iterator_facade.hpp>

//Forward Declaration
class TimingTags;
class TimingTag;

/*
 * A standard iterater interface to a collection of TimingTag objects.
 *
 * The actual object pointed to is defined as a template parameter, 
 * allowing the TimingTagIter class to model both const and non-const
 * iterators.
 */
template <class Value>
class TimingTagIter : public boost::iterator_facade<TimingTagIter<Value>, Value, boost::forward_traversal_tag> {
    private:
        Value* p; //Pointer to the TimingTag
    public:
        TimingTagIter(): p(nullptr) {}
        explicit TimingTagIter(Value* tag): p(tag) {}
    private:
        friend class boost::iterator_core_access;

        void increment() { p = p->next(); }
        bool equal(TimingTagIter<Value> const& other) const { return p == other.p; }
        Value& dereference() const { return *p; }
};

/*
 * Actual Non-Const/Const iterators
 */
typedef TimingTagIter<TimingTag> TimingTagIterator;
typedef TimingTagIter<TimingTag const> TimingTagConstIterator;
