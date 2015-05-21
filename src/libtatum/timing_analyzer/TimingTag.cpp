#include <iostream>
#include "TimingTag.hpp"
#include "assert.hpp"

/*
 * TagType implementation
 */
std::ostream& operator<<(std::ostream& os, const TagType tag_type) {
    if(tag_type == TagType::CLOCK) os << "CLOCK";
    else if (tag_type == TagType::DATA) os << "DATA";
    else if (tag_type == TagType::UNKOWN) os << "UNKOWN";
    else VERIFY(0);
    return os;
}

/*
 * TimingTag implementation
 */
void TimingTag::update_arr(const Time& new_arr_time, const TimingTag& base_tag) {
    //NOTE: leave next alone, since we want to keep the linked list intact
    ASSERT(clock_domain() == base_tag.clock_domain()); //Domain must be the same
    set_arr_time(new_arr_time);
    set_launch_node(base_tag.launch_node());
}

void TimingTag::update_req(const Time& new_req_time, const TimingTag& base_tag) {
    //NOTE: leave next alone, since we want to keep the linked list intact
    //      leave launch_node alone, since it is set by arrival only
    ASSERT(clock_domain() == base_tag.clock_domain()); //Domain must be the same
    set_req_time(new_req_time);
}

