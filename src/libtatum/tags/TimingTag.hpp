#pragma once
#include <iosfwd>

#include "Time.hpp"
#include "timing_graph_fwd.hpp"
#include "TimingTagIterators.hpp"

/**
 * The 'TimingTag' class represents an individual timing tag: the information associated
 * with a node's arrival/required times.
 *
 *
 * This primarily includes the actual arrival and required time, but also
 * auxillary metadata such as the clock domain, and launching node.
 *
 * The clock domain in particular is used to tag arrival/required times from different
 * clock domains at a single node.  This enables us to perform only a single graph traversal
 * and still analyze mulitple clocks.
 *
 * NOTE: Timing analyzers usually operate on the collection of tags at a particular node.
 *       This is modelled by the separate 'TimingTags' class.
 */
class TimingTag {
    public:
        /*
         * Constructors
         */
        TimingTag();

        ///\param arr_time_val The tagged arrival time
        ///\param req_time_val The tagged required time
        ///\param domain The clock domain the arrival/required times were launched from
        ///\param node The original launch node's id (i.e. primary input that originally launched this tag)
        TimingTag(const Time& arr_time_val, const Time& req_time_val, DomainId domain, NodeId node);

        ///\param arr_time_val The tagged arrival time
        ///\param req_time_val The tagged required time
        ///\param base_tag The tag from which to copy auxilary meta-data (e.g. domain, launch node)
        TimingTag(const Time& arr_time_val, const Time& req_time_val, const TimingTag& base_tag);

        /*
         * Getters
         */
        ///\returns This tag's arrival time
        const Time& arr_time() const { return arr_time_; }

        ///\returns This tag's required time
        const Time& req_time() const { return req_time_; }

        ///\returns This tag's launching clock domain
        DomainId clock_domain() const { return clock_domain_; }

        ///\returns This tag's launching node's id
        NodeId launch_node() const { return launch_node_; }

        /*
         * Setters
         */
        ///\param new_arr_time The new value set as the tag's arrival time
        void set_arr_time(const Time& new_arr_time) { arr_time_ = new_arr_time; };

        ///\param new_req_time The new value set as the tag's required time
        void set_req_time(const Time& new_req_time) { req_time_ = new_req_time; };

        ///\param new_req_time The new value set as the tag's clock domain
        void set_clock_domain(const DomainId new_clock_domain) { clock_domain_ = new_clock_domain; };

        ///\param new_launch_node The new value set as the tag's launching node
        void set_launch_node(const NodeId new_launch_node) { launch_node_ = new_launch_node; };

        /*
         * Modification operations
         *  For the following the passed in time is maxed/minned with the
         *  respective arr/req time.  If the value of this tag is updated
         *  the meta-data (domain, launch node etc) are copied from the
         *  base tag
         */
        ///Updates the tag's arrival time if new_arr_time is larger than the current arrival time.
        ///If the arrival time is updated, meta-data is also updated from base_tag
        ///\param new_arr_time The arrival time to compare against
        ///\param base_tag The tag from which meta-data is copied
        void max_arr(const Time& new_arr_time, const TimingTag& base_tag);

        ///Updates the tag's arrival time if new_arr_time is smaller than the current arrival time.
        ///If the arrival time is updated, meta-data is also updated from base_tag
        ///\param new_arr_time The arrival time to compare against
        ///\param base_tag The tag from which meta-data is copied
        void min_arr(const Time& new_arr_time, const TimingTag& base_tag);

        ///Updates the tag's required time if new_req_time is smaller than the current required time.
        ///If the required time is updated, meta-data is also updated from base_tag
        ///\param new_arr_time The arrival time to compare against
        ///\param base_tag The tag from which meta-data is copied
        void min_req(const Time& new_req_time, const TimingTag& base_tag);

        ///Updates the tag's required time if new_req_time is larger than the current required time.
        ///If the required time is updated, meta-data is also updated from base_tag
        ///\param new_arr_time The arrival time to compare against
        ///\param base_tag The tag from which meta-data is copied
        void max_req(const Time& new_req_time, const TimingTag& base_tag);

    private:
        void update_arr(const Time& new_arr_time, const TimingTag& base_tag);
        void update_req(const Time& new_req_time, const TimingTag& base_tag);

        /*
         * Data
         */
        Time arr_time_; //Arrival time
        Time req_time_; //Required time
        DomainId clock_domain_; //Clock domain for arr/req times
        NodeId launch_node_; //Node which launched this arrival time
};

//Implementation
#include "TimingTag.inl"
