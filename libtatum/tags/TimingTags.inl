#include <algorithm>
#include "tatum_assert.hpp"

namespace tatum {

/*
 * TimingTags implementation
 */

inline TimingTags::TimingTags(size_t num_reserve) {
    tags_.reserve(num_reserve);
}

inline size_t TimingTags::num_tags() const { 
    return tags_.size();
}

inline TimingTags::iterator TimingTags::begin() {
    return tags_.begin(); 
}

inline TimingTags::const_iterator TimingTags::begin() const { 
    return tags_.begin(); 
}

inline TimingTags::iterator TimingTags::end() { 
    return tags_.end(); 
}

inline TimingTags::const_iterator TimingTags::end() const { 
    return tags_.end(); 
}

//Modifiers
inline void TimingTags::add_tag(const TimingTag& tag) {
    TATUM_ASSERT(tag.clock_domain());

    tags_.push_back(tag);
}

inline void TimingTags::max_arr(const Time& new_time, const TimingTag& base_tag) {
    iterator iter = find_matching_tag(base_tag);
    if(iter == end()) {
        //First time we've seen this domain
        TimingTag tag = TimingTag(new_time, Time(NAN), base_tag);
        add_tag(tag);
    } else {
        iter->max_arr(new_time, base_tag);
    }
}

inline void TimingTags::min_req(const Time& new_time, const TimingTag& base_tag) {
    iterator iter = find_matching_tag(base_tag);
    if(iter == end()) {
        //First time we've seen this domain
        TimingTag tag = TimingTag(Time(NAN), new_time, base_tag);
        add_tag(tag);
    } else {
        iter->min_req(new_time, base_tag);
    }
}

inline void TimingTags::min_arr(const Time& new_time, const TimingTag& base_tag) {
    iterator iter = find_matching_tag(base_tag);
    if(iter == end()) {
        //First time we've seen this domain
        TimingTag tag = TimingTag(new_time, Time(NAN), base_tag);
        add_tag(tag);
    } else {
        iter->min_arr(new_time, base_tag);
    }
}

inline void TimingTags::max_req(const Time& new_time, const TimingTag& base_tag) {
    iterator iter = find_matching_tag(base_tag);
    if(iter == end()) {
        //First time we've seen this domain
        TimingTag tag = TimingTag(new_time, Time(NAN), base_tag);
        add_tag(tag);
    } else {
        iter->max_req(new_time, base_tag);
    }
}

inline void TimingTags::clear() {
    tags_.clear();
}

inline TimingTags::iterator TimingTags::find_matching_tag(const TimingTag& tag) {
    auto pred = [&](const TimingTag& check_tag) {
        return    tag.clock_domain() == check_tag.clock_domain()
               && tag.type() == check_tag.type();
    };
    return std::find_if(begin(), end(), pred);
}

inline TimingTags::const_iterator TimingTags::find_matching_tag(const TimingTag& tag) const {
    auto pred = [&](const TimingTag& check_tag) {
        return    tag.clock_domain() == check_tag.clock_domain()
               && tag.type() == check_tag.type();
    };
    return std::find_if(begin(), end(), pred);
}

} //namepsace
