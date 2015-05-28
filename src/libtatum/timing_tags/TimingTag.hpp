#pragma once
#include <iosfwd>

#include <boost/iterator/iterator_facade.hpp>

#include "Time.hpp"
#include "timing_graph_fwd.hpp"
#include "TimingTagIterator.hpp"

class TimingTag {
    public:
        TimingTag();
        TimingTag(const Time& arr_time_val, const Time& req_time_val, DomainId domain, NodeId node);
        TimingTag(const Time& arr_time_val, const Time& req_time_val, const TimingTag& base_tag);

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

        void min_req(const Time& new_req_time, const TimingTag& base_tag);
        void max_arr(const Time& new_arr_time, const TimingTag& base_tag);
        void max_req(const Time& new_req_time, const TimingTag& base_tag);
        void min_arr(const Time& new_arr_time, const TimingTag& base_tag);

    private:
        void update_arr(const Time& new_arr_time, const TimingTag& base_tag);
        void update_req(const Time& new_req_time, const TimingTag& base_tag);

        TimingTag* next_; //Next element in linked list
        Time arr_time_;
        Time req_time_;
        DomainId clock_domain_;
        NodeId launch_node_;
};

#include "TimingTag.inl"
