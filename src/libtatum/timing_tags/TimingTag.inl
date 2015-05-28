#include "assert.hpp"

/*
 * TimingTag implementation
 */

inline TimingTag::TimingTag()
    : next_(nullptr)
    , arr_time_(NAN)
    , req_time_(NAN)
    , clock_domain_(INVALID_CLOCK_DOMAIN)
    , launch_node_(-1)
    {}

inline TimingTag::TimingTag(const Time& arr_time_val, const Time& req_time_val, DomainId domain, NodeId node)
    : next_(nullptr)
    , arr_time_(arr_time_val)
    , req_time_(req_time_val)
    , clock_domain_(domain)
    , launch_node_(node)
    {}

inline TimingTag::TimingTag(const Time& arr_time_val, const Time& req_time_val, const TimingTag& base_tag)
    : next_(nullptr)
    , arr_time_(arr_time_val)
    , req_time_(req_time_val)
    , clock_domain_(base_tag.clock_domain())
    , launch_node_(base_tag.launch_node())
    {}


inline void TimingTag::update_arr(const Time& new_arr_time, const TimingTag& base_tag) {
    //NOTE: leave next alone, since we want to keep the linked list intact
    ASSERT(clock_domain() == base_tag.clock_domain()); //Domain must be the same
    set_arr_time(new_arr_time);
    set_launch_node(base_tag.launch_node());
}

inline void TimingTag::update_req(const Time& new_req_time, const TimingTag& base_tag) {
    //NOTE: leave next alone, since we want to keep the linked list intact
    //      leave launch_node alone, since it is set by arrival only
    ASSERT(clock_domain() == base_tag.clock_domain()); //Domain must be the same
    set_req_time(new_req_time);
}


inline void TimingTag::max_arr(const Time& new_arr_time, const TimingTag& base_tag) {
    //Need to min with existing value
    if(!arr_time().valid() || new_arr_time.value() > arr_time().value()) {
        //New value is smaller, or no previous valid value existed
        //Update min
        update_arr(new_arr_time, base_tag);
    }
}

inline void TimingTag::min_req(const Time& new_req_time, const TimingTag& base_tag) {
    //Need to min with existing value
    if(!req_time().valid() || new_req_time.value() < req_time().value()) {
        //New value is smaller, or no previous valid value existed
        //Update min
        update_req(new_req_time, base_tag);
    }
}

inline void TimingTag::min_arr(const Time& new_arr_time, const TimingTag& base_tag) {
    //Need to min with existing value
    if(!arr_time().valid() || new_arr_time.value() < arr_time().value()) {
        //New value is smaller, or no previous valid value existed
        //Update min
        update_arr(new_arr_time, base_tag);
    }
}

inline void TimingTag::max_req(const Time& new_req_time, const TimingTag& base_tag) {
    //Need to min with existing value
    if(!req_time().valid() || new_req_time.value() > req_time().value()) {
        //New value is smaller, or no previous valid value existed
        //Update min
        update_req(new_req_time, base_tag);
    }
}
