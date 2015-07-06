#pragma once
#include <iosfwd>

#include "Time.hpp"
#include "timing_graph_fwd.hpp"
#include "TimingTagIterators.hpp"

/*
 * The 'TimingTag' class represents an individual timing tag: the information associated 
 * with a node's arrival/required times.
 *
 *
 * This primarily includes the actual arrival and required time, but also 
 * auxillary metadata such as the clock domain, and launching node.
 *
 * The clock domain in particular is used to tag arrival/required times from different
 * clock domains on a single node.  This enables us to perform only a single graph traversal
 * and still analyze mulitple clocks.
 *
 * NOTE: Timing analyzers usually operate on the collection of tags at a particular node.
 *       This is modelled by the separate 'TimingTags' class.
 */
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

        //Modification operations
        // For the following the passed in time is maxed/minned with the
        // respective arr/req time.  If the value of this tag is updated
        // the meta-data (domain, launch node etc) are copied from the
        // base tag
        void max_arr(const Time& new_arr_time, const TimingTag& base_tag);
        void min_arr(const Time& new_arr_time, const TimingTag& base_tag);
        void min_req(const Time& new_req_time, const TimingTag& base_tag);
        void max_req(const Time& new_req_time, const TimingTag& base_tag);

    private:
        void update_arr(const Time& new_arr_time, const TimingTag& base_tag);
        void update_req(const Time& new_req_time, const TimingTag& base_tag);

        /*
         * Data
         */
        TimingTag* next_; //Next element in linked list of tags at a particular timing graph node
        Time arr_time_; //Arrival time
        Time req_time_; //Required time
        DomainId clock_domain_; //Clock domain for arr/req times
        NodeId launch_node_; //Node which launched this arrival time
};

//Implementation
#include "TimingTag.inl"
