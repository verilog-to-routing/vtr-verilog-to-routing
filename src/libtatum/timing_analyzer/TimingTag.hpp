#pragma once
#include <iosfwd>

#include <boost/iterator/iterator_facade.hpp>

#include "Time.hpp"
#include "timing_graph_fwd.hpp"

//Identifies the type of a TimingTag
enum class TagType {
    CLOCK, //This tag corresponds to the clock path
    DATA,  //This tag corresponds to the data path
    UNKOWN //This tag is invalid/default intialized.  Usually indicates an error if encountered in a real design.
};

std::ostream& operator<<(std::ostream& os, const TagType tag_type);

class TimingTag {
    public:
        TimingTag()
            : next_(nullptr)
            , arr_time_(NAN)
            , req_time_(NAN)
            , clock_domain_(INVALID_CLOCK_DOMAIN)
            , launch_node_(-1)
            {}
        TimingTag(const Time& arr_time_val, const Time& req_time_val, DomainId domain, NodeId node)
            : next_(nullptr)
            , arr_time_(arr_time_val)
            , req_time_(req_time_val)
            , clock_domain_(domain)
            , launch_node_(node)
            {}
        TimingTag(const Time& arr_time_val, const Time& req_time_val, const TimingTag& base_tag)
            : next_(nullptr)
            , arr_time_(arr_time_val)
            , req_time_(req_time_val)
            , clock_domain_(base_tag.clock_domain())
            , launch_node_(base_tag.launch_node())
            {}

        //Getters
        const Time& arr_time() const { return arr_time_; }
        const Time& req_time() const { return req_time_; }
        DomainId clock_domain() const { return clock_domain_; }
        NodeId launch_node() const { return launch_node_; }
        TimingTag* next() const { return next_; }

        //Setters
        void set_arr_time(const Time& new_arr_time) { arr_time_ = new_arr_time; };
        void set_req_time(const Time& new_req_time) { req_time_ = new_req_time; };
        void set_clock_domain(const DomainId new_clock_domain) { clock_domain_ = new_clock_domain; };
        void set_launch_node(const NodeId new_launch_node) { launch_node_ = new_launch_node; };
        void set_next(TimingTag* new_next) { next_ = new_next; }
        void update_arr(const Time& new_arr_time, const TimingTag& base_tag);
        void update_req(const Time& new_req_time, const TimingTag& base_tag);

    private:
        TimingTag* next_; //Next element in linked list
        Time arr_time_;
        Time req_time_;
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
