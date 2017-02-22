#include "tatum/util/tatum_assert.hpp"

namespace tatum {

/*
 * TimingTag implementation
 */

inline TimingTag::TimingTag()
    : time_(NAN)
#ifdef TATUM_TRACK_ORIGIN_NODE
    , origin_node_(NodeId::INVALID())
#endif
    , launch_clock_domain_(DomainId::INVALID())
    , capture_clock_domain_(DomainId::INVALID())
    , type_(TagType::UNKOWN)
    {}

inline TimingTag::TimingTag(const Time& time_val, 
                            const DomainId launch_domain, 
                            const DomainId capture_domain, 
#ifdef TATUM_TRACK_ORIGIN_NODE
                            const NodeId node, 
#else
                            const NodeId /*node*/,
#endif
                            const TagType new_type)
    : time_(time_val)
#ifdef TATUM_TRACK_ORIGIN_NODE
    , origin_node_(node)
#endif
    , launch_clock_domain_(launch_domain)
    , capture_clock_domain_(capture_domain)
    , type_(new_type)
    {}

inline TimingTag::TimingTag(const Time& time_val, NodeId origin, const TimingTag& base_tag)
    : time_(time_val)
#ifdef TATUM_TRACK_ORIGIN_NODE
    , origin_node_(origin)
#endif
    , launch_clock_domain_(base_tag.launch_clock_domain())
    , capture_clock_domain_(base_tag.capture_clock_domain())
    , type_(base_tag.type())
    {}


inline void TimingTag::update(const Time& new_time, const NodeId origin, const TimingTag& base_tag) {
    TATUM_ASSERT(type() == base_tag.type()); //Type must be the same
    TATUM_ASSERT(launch_clock_domain() == base_tag.launch_clock_domain()); //Domain must be the same
    set_time(new_time);
    set_origin_node(origin);
}

inline void TimingTag::max(const Time& new_time, const NodeId origin, const TimingTag& base_tag) {
    //Need to min with existing value
    if(!time().valid() || new_time > time()) {
        //New value is smaller, or no previous valid value existed
        //Update min
        update(new_time, origin, base_tag);
    }
}

inline void TimingTag::min(const Time& new_time, const NodeId origin, const TimingTag& base_tag) {
    //Need to min with existing value
    if(!time().valid() || new_time < time()) {
        //New value is smaller, or no previous valid value existed
        //Update min
        update(new_time, origin, base_tag);
    }
}

inline std::ostream& operator<<(std::ostream& os, TagType type) {
    if(type == TagType::DATA_ARRIVAL) os << "DATA_ARRIVAL";
    else if(type == TagType::DATA_REQUIRED) os << "DATA_REQUIRED";
    else if(type == TagType::CLOCK_LAUNCH) os << "CLOCK_LAUNCH";
    else if(type == TagType::CLOCK_CAPTURE) os << "CLOCK_CAPTURE";
    else if(type == TagType::SLACK) os << "SLACK";
    else TATUM_ASSERT_MSG(false, "Unrecognized TagType");
    return os;
}

} //namepsace
