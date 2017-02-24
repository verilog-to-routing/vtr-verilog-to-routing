#include "tatum/util/tatum_assert.hpp"

namespace tatum {

/*
 * TimingTag implementation
 */

inline TimingTag::TimingTag()
    : time_(NAN)
    , origin_node_(NodeId::INVALID())
    , launch_clock_domain_(DomainId::INVALID())
    , capture_clock_domain_(DomainId::INVALID())
    , type_(TagType::UNKOWN)
    {}

inline TimingTag::TimingTag(const Time& time_val, 
                            const DomainId launch_domain, 
                            const DomainId capture_domain, 
                            const NodeId node, 
                            const TagType new_type)
    : time_(time_val)
    , origin_node_(node)
    , launch_clock_domain_(launch_domain)
    , capture_clock_domain_(capture_domain)
    , type_(new_type)
    {}

inline TimingTag::TimingTag(const Time& time_val, NodeId origin, const TimingTag& base_tag)
    : time_(time_val)
    , origin_node_(origin)
    , launch_clock_domain_(base_tag.launch_clock_domain())
    , capture_clock_domain_(base_tag.capture_clock_domain())
    , type_(base_tag.type())
    {}


inline void TimingTag::update(const Time& new_time, const NodeId origin, const TimingTag& base_tag) {
    TATUM_ASSERT(type() == base_tag.type()); //Type must be the same

    //Note that we check for a constant tag first, since we might 
    //update the time (which is used to verify whether the tag is a constant
    if(is_const_gen_tag(*this)) {

        //If the tag is a constant, then we want to replace it with the 
        //first non-constant tag which comes along. This ensures that
        //any constant tags get 'swallowed' by real tags and do not
        //continue to propagate through the timing graph
        set_launch_clock_domain(base_tag.launch_clock_domain());
        set_capture_clock_domain(base_tag.capture_clock_domain());
    }

    TATUM_ASSERT((   launch_clock_domain() == base_tag.launch_clock_domain())
                  && capture_clock_domain() == base_tag.capture_clock_domain()); //Same domains

    //Update the tag
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

//Returns true, if tag is a tag from a constant generator
inline bool is_const_gen_tag(const TimingTag& tag) { 
    return    !tag.launch_clock_domain()        //Wildcard launch
           && !tag.capture_clock_domain()       //Wildcard capture
           && std::isinf(tag.time().value());   //inf arrival, we allow +/- inf since different values may be used for setup/hold
}


} //namepsace
