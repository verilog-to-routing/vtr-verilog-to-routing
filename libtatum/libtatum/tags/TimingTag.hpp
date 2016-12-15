#pragma once
#include <iosfwd>

#include "Time.hpp"
#include "timing_graph_fwd.hpp"

namespace tatum {

enum class TagType : unsigned char {
    CLOCK_LAUNCH,
    CLOCK_CAPTURE,
    DATA_ARRIVAL,
    DATA_REQUIRED,
    SLACK,
    UNKOWN
};

std::ostream& operator<<(std::ostream& os, TagType type);

//Track the origin node that is the determinant of a tag
// This can be disabled to reduce the size of the TimingTag object.
// If origin node tracking is disabled, the origin_node() accessors 
// will always return an invalid node id and constructors/set_origin_node()
// will ignore the passed node id
#define TATUM_TRACK_ORIGIN_NODE

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
        ///\param launch_domain The clock domain the tag was launched from
        ///\param capture_domain The clock domain the tag was captured on
        ///\param node The origin node's id (e.g. source/sink that originally launched/required this tag)
        TimingTag(const Time& time_val, DomainId launch_domain, DomainId capture_domain, NodeId node, TagType type);

        ///\param arr_time_val The tagged arrival time
        ///\param req_time_val The tagged required time
        ///\param base_tag The tag from which to copy auxilary meta-data (e.g. domain, launch node)
        TimingTag(const Time& time_val, const TimingTag& base_tag);

        /*
         * Getters
         */
        ///\returns This tag's arrival time
        const Time& time() const { return time_; }

        ///\returns This tag's launching clock domain
        DomainId launch_clock_domain() const { return launch_clock_domain_; }
        DomainId capture_clock_domain() const { return capture_clock_domain_; }

        ///\returns This tag's launching node's id
#ifdef TATUM_TRACK_ORIGIN_NODE
        NodeId origin_node() const { return origin_node_; }
#else
        NodeId origin_node() const { return NodeId::INVALID(); }
#endif

        TagType type() const { return type_; }

        /*
         * Setters
         */
        ///\param new_time The new value set as the tag's time
        void set_time(const Time& new_time) { time_ = new_time; }

        ///\param new_clock_domain The new value set as the tag's source clock domain
        void set_launch_clock_domain(const DomainId new_clock_domain) { launch_clock_domain_ = new_clock_domain; }

        ///\param new_clock_domain The new value set as the tag's capture clock domain
        void set_capture_clock_domain(const DomainId new_clock_domain) { capture_clock_domain_ = new_clock_domain; }

        ///\param new_launch_node The new value set as the tag's launching node
#ifdef TATUM_TRACK_ORIGIN_NODE
        void set_origin_node(const NodeId new_origin_node) { origin_node_ = new_origin_node; }
#else
        void set_origin_node(const NodeId /*new_origin_node*/) { }
#endif

        void set_type(const TagType new_type) { type_ = new_type; }

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
        void max(const Time& new_time, const TimingTag& base_tag);

        ///Updates the tag's arrival time if new_arr_time is smaller than the current arrival time.
        ///If the arrival time is updated, meta-data is also updated from base_tag
        ///\param new_arr_time The arrival time to compare against
        ///\param base_tag The tag from which meta-data is copied
        void min(const Time& new_time, const TimingTag& base_tag);

    private:
        void update(const Time& new_time, const TimingTag& base_tag);

        /*
         * Data
         */
        Time time_; //Required time
#ifdef TATUM_TRACK_ORIGIN_NODE
        NodeId origin_node_; //Node which launched this arrival time
#endif
        DomainId launch_clock_domain_; //Clock domain for arr/req times
        DomainId capture_clock_domain_; //Clock domain for arr/req times
        TagType type_;
};

} //namepsace

//Implementation
#include "TimingTag.inl"
