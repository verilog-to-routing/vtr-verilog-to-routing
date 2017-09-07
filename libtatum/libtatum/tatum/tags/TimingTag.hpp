#pragma once
#include <iosfwd>
#include <limits>

#include "tatum/Time.hpp"
#include "tatum/TimingGraphFwd.hpp"

namespace tatum {

enum class TagType : unsigned char {
    CLOCK_LAUNCH,
    CLOCK_CAPTURE,
    DATA_ARRIVAL,
    DATA_REQUIRED,
    SLACK,
    UNKOWN
};

class TimingTag;

std::ostream& operator<<(std::ostream& os, TagType type);

bool is_const_gen_tag(const TimingTag& tag);

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
    public: //Static
        //Returns a tag suitable for use at a constant generator, during 
        //setup analysis.
        //
        ///In particular it's domains are left invalid (i.e. wildcards),
        //and it's arrival time set to -inf (so it will be maxed out by
        //any non-constant tag)
        static TimingTag CONST_GEN_TAG_SETUP() {
            return TimingTag(Time(-std::numeric_limits<float>::infinity()),
                             DomainId::INVALID(),
                             DomainId::INVALID(),
                             NodeId::INVALID(),
                             TagType::DATA_ARRIVAL);
        }

        //Returns a tag suitable for use at a constant generator, during 
        //hold analysis.
        //
        ///In particular it's domains are left invalid (i.e. wildcards),
        //and it's arrival time set to +inf (so it will be minned out by
        //any non-constant tag)
        static TimingTag CONST_GEN_TAG_HOLD() {
            return TimingTag(Time(+std::numeric_limits<float>::infinity()),
                             DomainId::INVALID(),
                             DomainId::INVALID(),
                             NodeId::INVALID(),
                             TagType::DATA_ARRIVAL);
        }
    public: //Constructors
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
        TimingTag(const Time& time_val, NodeId origin, const TimingTag& base_tag);


    public: //Accessors
        ///\returns This tag's arrival time
        const Time& time() const { return time_; }

        ///\returns This tag's launching clock domain
        DomainId launch_clock_domain() const { return launch_clock_domain_; }
        DomainId capture_clock_domain() const { return capture_clock_domain_; }

        ///\returns This tag's launching node's id
        NodeId origin_node() const { return origin_node_; }

        TagType type() const { return type_; }

    public: //Mutators
        ///\param new_time The new value set as the tag's time
        void set_time(const Time& new_time) { time_ = new_time; }

        ///\param new_clock_domain The new value set as the tag's source clock domain
        void set_launch_clock_domain(const DomainId new_clock_domain) { launch_clock_domain_ = new_clock_domain; }

        ///\param new_clock_domain The new value set as the tag's capture clock domain
        void set_capture_clock_domain(const DomainId new_clock_domain) { capture_clock_domain_ = new_clock_domain; }

        ///\param new_launch_node The new value set as the tag's launching node
        void set_origin_node(const NodeId new_origin_node) { origin_node_ = new_origin_node; }

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
        void max(const Time& new_time, const NodeId origin, const TimingTag& base_tag);

        ///Updates the tag's arrival time if new_arr_time is smaller than the current arrival time.
        ///If the arrival time is updated, meta-data is also updated from base_tag
        ///\param new_arr_time The arrival time to compare against
        ///\param base_tag The tag from which meta-data is copied
        void min(const Time& new_time, const NodeId origin, const TimingTag& base_tag);

    private:
        void update(const Time& new_time, const NodeId origin, const TimingTag& base_tag);

        /*
         * Data
         */
        Time time_; //Required time
        NodeId origin_node_; //Node which launched this arr/req time
        DomainId launch_clock_domain_; //Clock domain for arr/req times
        DomainId capture_clock_domain_; //Clock domain for arr/req times
        TagType type_;
};


//For comparing the values of two timing tags
struct TimingTagValueComp {
    bool operator()(const tatum::TimingTag& lhs, const tatum::TimingTag& rhs) {
        return lhs.time().value() < rhs.time().value();
    }
};

} //namepsace

//Implementation
#include "TimingTag.inl"
