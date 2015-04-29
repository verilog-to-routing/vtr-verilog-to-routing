#pragma once
#include <algorithm>
#include <cassert>
#include <iterator>

#include <boost/intrusive/slist.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include "timing_graph_fwd.hpp"
#include "Time.hpp"

//How many timing tag objects should be flattened into the TimingTags class?
//  This typically helps cache locality, 1 tends to perform best
#define NUM_FLAT_TAGS 1

class TimingTag {
    public:
        TimingTag()
            : next_(nullptr)
            , time_(NAN)
            , clock_domain_(INVALID_CLOCK_DOMAIN)
            , launch_node_(-1) {}
        TimingTag(const Time& time_val, DomainId domain, NodeId node)
            : next_(nullptr)
            , time_(time_val)
            , clock_domain_(domain)
            , launch_node_(node) {}

        const Time& time() const { return time_; }
        DomainId clock_domain() const { return clock_domain_; };
        NodeId launch_node() const { return launch_node_; };
        TimingTag* next() const { return next_; }

        //Setters
        void set_time(const Time& new_time) { time_ = new_time; };
        void set_clock_domain(const DomainId new_clock_domain) { clock_domain_ = new_clock_domain; };
        void set_launch_node(const NodeId new_launch_node) { launch_node_ = new_launch_node; };
        void set_next(TimingTag* new_next) { next_ = new_next; }
    private:
        TimingTag* next_;
        Time time_;
        DomainId clock_domain_;
        NodeId launch_node_;
};

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

class TimingTags {
    public:
        //Getters
        size_t num_tags() const { return num_tags_; };
        TimingTagIterator find_tag_by_clock_domain(DomainId domain_id);
        TimingTagIterator begin() { return (num_tags_ > 0) ? TimingTagIterator(&head_tags_[0]) : end(); };
        TimingTagIterator end() { return TimingTagIterator(nullptr); };
        TimingTagConstIterator begin() const { return (num_tags_ > 0) ? TimingTagConstIterator(&head_tags_[0]) : end(); };
        TimingTagConstIterator end() const { return TimingTagConstIterator(nullptr); };

        //Modifiers
        void add_tag(boost::pool<>& tag_pool, const Time& new_time, const DomainId new_clock_domain, const NodeId new_launch_node);
        void max_tag(boost::pool<>& tag_pool, const Time& new_time, const DomainId new_clock_domain, const NodeId new_launch_node);
        void min_tag(boost::pool<>& tag_pool, const Time& new_time, const DomainId new_clock_domain, const NodeId new_launch_node);
        void clear();


    private:
        int num_tags_;

        //The first NUM_FLAT_TAGS tags are stored directly as members
        //of this object. Any additional tags are stored in a dynamically
        //allocated linked list.
        //Note that despite being an array, each element of head_tags_ is
        //hooked into the linked list
        std::array<TimingTag, NUM_FLAT_TAGS> head_tags_;
};


