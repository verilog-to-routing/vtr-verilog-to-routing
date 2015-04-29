#pragma once
#include <algorithm>
#include <cassert>

#include <boost/intrusive/slist.hpp>
#include <boost/pool/object_pool.hpp>

#include "timing_graph_fwd.hpp"
#include "Time.hpp"

//Forward declaration
class TimingTag : public boost::intrusive::slist_base_hook<> {
    public:
        TimingTag(const Time& time_val, DomainId domain, NodeId node)
            : time_(time_val)
            , clock_domain_(domain)
            , launch_node_(node) {}

        const Time& time() const { return time_; }
        DomainId clock_domain() const { return clock_domain_; };
        NodeId launch_node() const { return launch_node_; };

        //Setters
        void set_time(const Time& new_time) { time_ = new_time; };
        void set_clock_domain(const DomainId new_clock_domain) { clock_domain_ = new_clock_domain; };
        void set_launch_node(const NodeId new_launch_node) { launch_node_ = new_launch_node; };
    private:
        Time time_;
        DomainId clock_domain_;
        NodeId launch_node_;
};

class TimingTags {
        typedef boost::intrusive::slist<TimingTag> TagList;
    public:
        //Getters
        size_t num_tags() const { return tags_.size(); };
        TagList::iterator find_tag_by_clock_domain(DomainId domain_id);
        TagList::iterator begin() { return tags_.begin(); };
        TagList::iterator end() { return tags_.end(); };
        TagList::const_iterator begin() const { return tags_.begin(); };
        TagList::const_iterator end() const { return tags_.end(); };

        //Modifiers
        void add_tag(boost::object_pool<TimingTag>& tag_pool, const Time& new_time, const DomainId new_clock_domain, const NodeId new_launch_node);
        void max_tag(boost::object_pool<TimingTag>& tag_pool, const Time& new_time, const DomainId new_clock_domain, const NodeId new_launch_node);
        void min_tag(boost::object_pool<TimingTag>& tag_pool, const Time& new_time, const DomainId new_clock_domain, const NodeId new_launch_node);
        void clear();


    private:
        TagList tags_;

};

