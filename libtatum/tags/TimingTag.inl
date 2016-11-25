#include "tatum_assert.hpp"

namespace tatum {

/*
 * TimingTag implementation
 */

inline TimingTag::TimingTag()
    : time_(NAN)
    , launch_node_(NodeId::INVALID())
    , clock_domain_(DomainId::INVALID())
    , type_(TagType::UNKOWN)
    {}

inline TimingTag::TimingTag(const Time& time_val, const DomainId domain, const NodeId node, const TagType new_type)
    : time_(time_val)
    , launch_node_(node)
    , clock_domain_(domain)
    , type_(new_type)
    {}

inline TimingTag::TimingTag(const Time& time_val, const TimingTag& base_tag)
    : time_(time_val)
    , launch_node_(base_tag.launch_node())
    , clock_domain_(base_tag.clock_domain())
    , type_(base_tag.type())
    {}


inline void TimingTag::update(const Time& new_time, const TimingTag& base_tag) {
    TATUM_ASSERT(type() == base_tag.type()); //Type must be the same
    TATUM_ASSERT(clock_domain() == base_tag.clock_domain()); //Domain must be the same
    set_time(new_time);
    set_launch_node(base_tag.launch_node());
}

inline void TimingTag::max(const Time& new_time, const TimingTag& base_tag) {
    //Need to min with existing value
    if(!time().valid() || new_time > time()) {
        //New value is smaller, or no previous valid value existed
        //Update min
        update(new_time, base_tag);
    }
}

inline void TimingTag::min(const Time& new_time, const TimingTag& base_tag) {
    //Need to min with existing value
    if(!time().valid() || new_time < time()) {
        //New value is smaller, or no previous valid value existed
        //Update min
        update(new_time, base_tag);
    }
}

inline std::ostream& operator<<(std::ostream& os, TagType type) {
    if(type == TagType::DATA_ARRIVAL) os << "DATA_ARRIVAL";
    else if(type == TagType::DATA_REQUIRED) os << "DATA_REQUIRED";
    else if(type == TagType::CLOCK_LAUNCH) os << "CLOCK_LAUNCH";
    else if(type == TagType::CLOCK_CAPTURE) os << "CLOCK_CAPTURE";
    else TATUM_ASSERT_MSG(false, "Unrecognized TagType");
    return os;
}

} //namepsace
