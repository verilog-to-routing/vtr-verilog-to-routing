#include "assert.hpp"
/*
 * TimingTags implementation
 */

inline TimingTags::TimingTags() {
    tags_.reserve(1);
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
    //Don't add invalid clock domains
    //Some sources like constant generators may yeild illegal clock domains
    if(tag.clock_domain() == INVALID_CLOCK_DOMAIN) {
        return;
    }

    //Allocate form a central storage pool
    tags_.push_back(tag);
}

inline void TimingTags::max_arr(const Time& new_time, const TimingTag& base_tag) {
    iterator iter = find_tag_by_clock_domain(base_tag.clock_domain());
    if(iter == end()) {
        //First time we've seen this domain
        TimingTag tag = TimingTag(new_time, Time(NAN), base_tag);
        add_tag(tag);
    } else {
        iter->max_arr(new_time, base_tag);
    }
}

inline void TimingTags::min_req(const Time& new_time, const TimingTag& base_tag) {
    iterator iter = find_tag_by_clock_domain(base_tag.clock_domain());
    if(iter == end()) {
        //First time we've seen this domain
        TimingTag tag = TimingTag(Time(NAN), new_time, base_tag);
        add_tag(tag);
    } else {
        iter->min_req(new_time, base_tag);
    }
}

inline void TimingTags::min_arr(const Time& new_time, const TimingTag& base_tag) {
    iterator iter = find_tag_by_clock_domain(base_tag.clock_domain());
    if(iter == end()) {
        //First time we've seen this domain
        TimingTag tag = TimingTag(new_time, Time(NAN), base_tag);
        add_tag(tag);
    } else {
        iter->min_arr(new_time, base_tag);
    }
}

inline void TimingTags::max_req(const Time& new_time, const TimingTag& base_tag) {
    iterator iter = find_tag_by_clock_domain(base_tag.clock_domain());
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

inline TimingTags::iterator TimingTags::find_tag_by_clock_domain(DomainId domain_id) {
    auto pred = [domain_id](const TimingTag& tag) {
        return tag.clock_domain() == domain_id;
    };
    return std::find_if(begin(), end(), pred);
}

inline TimingTags::const_iterator TimingTags::find_tag_by_clock_domain(DomainId domain_id) const {
    auto pred = [domain_id](const TimingTag& tag) {
        return tag.clock_domain() == domain_id;
    };
    return std::find_if(begin(), end(), pred);
}
