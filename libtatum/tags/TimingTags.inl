#include <algorithm>
#include "tatum_assert.hpp"

namespace tatum {

namespace details {
    struct TagSortComp {
        bool operator()(const TimingTag& lhs, const TimingTag& rhs) {
            if(lhs.type() < rhs.type()) {
                //First by type
                return true;
            } else if(lhs.type() == rhs.type()) {
                //Then by clock domain
                return lhs.clock_domain() < rhs.clock_domain();
            }
            return false;
        }
    };

    struct TagRangeComp {
        bool operator()(const TimingTag& tag, const TagType type) {
            return tag.type() < type;
        }
        bool operator()(const TagType type, const TimingTag& tag) {
            return type < tag.type();
        }
    };
}

/*
 * TimingTags implementation
 */

inline TimingTags::TimingTags(size_t num_reserve) {
    tags_.reserve(num_reserve);
}

inline size_t TimingTags::size() const { 
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

inline TimingTags::tag_range TimingTags::tags(const TagType type) const {
    auto b = std::lower_bound(begin(), end(), type, details::TagRangeComp());
    auto e = std::upper_bound(begin(), end(), type, details::TagRangeComp());

    return tatum::util::make_range(b, e);
}

//Modifiers
inline void TimingTags::add_tag(const TimingTag& tag) {
    TATUM_ASSERT(tag.clock_domain());

    //Upper bound returns an iterator to the first item not less than tag
    auto iter = std::upper_bound(begin(), end(), tag, details::TagSortComp());

    //Insert the tag before the upper bound position
    // This ensures tags_ is always in sorted order
    tags_.insert(iter, tag);
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

inline void TimingTags::min_req(const Time& new_time, const TimingTag& base_tag, bool arr_must_be_valid) {
    iterator iter = find_matching_tag(base_tag);
    if(iter == end()) {
        if(!arr_must_be_valid) {
            //First time we've seen this domain
            TimingTag tag = TimingTag(Time(NAN), new_time, base_tag);
            add_tag(tag);
        }
    } else {
        if(!arr_must_be_valid || iter->arr_time().valid()) {
            iter->min_req(new_time, base_tag);
        }
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

inline void TimingTags::max_req(const Time& new_time, const TimingTag& base_tag, bool arr_must_be_valid) {
    iterator iter = find_matching_tag(base_tag);
    if(iter == end()) {
        if(!arr_must_be_valid) {
            //First time we've seen this domain
            TimingTag tag = TimingTag(new_time, Time(NAN), base_tag);
            add_tag(tag);
        }
    } else {
        if(!arr_must_be_valid || iter->arr_time().valid()) {
            iter->max_req(new_time, base_tag);
        }
    }
}

inline void TimingTags::clear() {
    tags_.clear();
}

inline TimingTags::iterator TimingTags::find_matching_tag(const TimingTag& tag) {
    auto iter = std::lower_bound(begin(), end(), tag, details::TagSortComp());
    if(iter != end() && iter->clock_domain() == tag.clock_domain() && iter->type() == tag.type()) {
        //Match
        return iter;
    } else {
        //Not found
        return end();
    }
}

inline TimingTags::const_iterator TimingTags::find_matching_tag(const TimingTag& tag) const {
    auto iter = std::lower_bound(begin(), end(), tag, details::TagSortComp());
    if(iter != end() && iter->clock_domain() == tag.clock_domain() && iter->type() == tag.type()) {
        //Match
        return iter;
    } else {
        //Not found
        return end();
    }
}

} //namepsace
