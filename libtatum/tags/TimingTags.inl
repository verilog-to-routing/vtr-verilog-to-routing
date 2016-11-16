#include <algorithm>
#include "tatum_assert.hpp"

namespace tatum {

/*
 * TimingTags implementation
 */

inline TimingTags::TimingTags(size_t num_reserve) {
    tags_.reserve(num_reserve);
}

inline size_t TimingTags::size() const { 
    return tags_.size();
}

inline TimingTags::iterator TimingTags::begin(TagType type) {
    iterator iter;
    switch(type) {
        case TagType::CLOCK_LAUNCH: 
            iter = tags_.begin();
            break;
        case TagType::CLOCK_CAPTURE: 
            iter = tags_.begin() + num_clock_launch_tags_;
            break;
        case TagType::DATA: 
            iter = tags_.begin() + num_clock_launch_tags_ + num_clock_capture_tags_;
            break;
        default:
            TATUM_ASSERT_MSG(false, "Invalid tag type");
    }
    return iter;
}

inline TimingTags::const_iterator TimingTags::begin(TagType type) const {
    const_iterator iter;
    switch(type) {
        case TagType::CLOCK_LAUNCH: 
            iter = tags_.begin();
            break;
        case TagType::CLOCK_CAPTURE: 
            iter = tags_.begin() + num_clock_launch_tags_;
            break;
        case TagType::DATA: 
            iter = tags_.begin() + num_clock_launch_tags_ + num_clock_capture_tags_;
            break;
        default:
            TATUM_ASSERT_MSG(false, "Invalid tag type");
    }
    return iter;
}

inline TimingTags::iterator TimingTags::end(TagType type) { 
    iterator iter;
    switch(type) {
        case TagType::CLOCK_LAUNCH: 
            iter = begin(TagType::CLOCK_CAPTURE);
            break;
        case TagType::CLOCK_CAPTURE: 
            iter = begin(TagType::DATA);
            break;
        case TagType::DATA: 
            iter = tags_.end();
            break;
        default:
            TATUM_ASSERT_MSG(false, "Invalid tag type");
    }
    return iter;
}

inline TimingTags::const_iterator TimingTags::end(TagType type) const { 
    const_iterator iter;
    switch(type) {
        case TagType::CLOCK_LAUNCH: 
            iter = begin(TagType::CLOCK_CAPTURE);
            break;
        case TagType::CLOCK_CAPTURE: 
            iter = begin(TagType::DATA);
            break;
        case TagType::DATA: 
            //Pass the true end
            iter = tags_.end();
            break;
        default:
            TATUM_ASSERT_MSG(false, "Invalid tag type");
    }
    return iter;
}

inline TimingTags::tag_range TimingTags::tags() const {
    return tatum::util::make_range(tags_.begin(), tags_.end());
}

inline TimingTags::tag_range TimingTags::tags(const TagType type) const {
    auto rng = tatum::util::make_range(begin(type), end(type));

    switch(type) {
        case TagType::CLOCK_LAUNCH:
            TATUM_ASSERT(rng.size() == num_clock_launch_tags_);
            break;
        case TagType::CLOCK_CAPTURE:
            TATUM_ASSERT(rng.size() == num_clock_capture_tags_);
            break;
        case TagType::DATA:
            TATUM_ASSERT(rng.size() == tags_.size() - num_clock_launch_tags_ - num_clock_capture_tags_);
            break;
        default:
            TATUM_ASSERT_MSG(false, "Invalid tag type");
    }

    return rng;
}

//Modifiers
inline void TimingTags::add_tag(const TimingTag& tag) {
    TATUM_ASSERT(tag.clock_domain());

    //Find the position to insert this tag
    //
    //We keep tags of the same type together.
    //We also prefer to insert new tags at the end if possible
    //(since this is more efficient for the underlying vector storage)

#if 0
    auto insert_iter = end(); //Default to end

    //Linear search
    bool in_matching_range = false;
    for(auto iter = begin(); iter != end(); ++iter) {
        if(iter->type() == tag.type()) {
            if(!in_matching_range) {
                //First matching element, so now within matching type range
                in_matching_range = true;
            }
        } else if (in_matching_range) {
            //Non-matching type: First element out side matching type range
            insert_iter = iter; //We want to insert just before here
            break;
        }
    }
#else
    auto iter = end(tag.type());

    switch(tag.type()) {
        case TagType::CLOCK_LAUNCH: 
            ++num_clock_launch_tags_;
            break;
        case TagType::CLOCK_CAPTURE: 
            ++num_clock_capture_tags_;
            break;
        case TagType::DATA: 
            //Pass
            break;
        default:
            TATUM_ASSERT_MSG(false, "Invalid tag type");
    }
#endif

    //Insert the tag before the upper bound position
    // This ensures tags_ is always in sorted order
    tags_.insert(iter, tag);
}

inline void TimingTags::max_arr(const Time& new_time, const TimingTag& base_tag) {
    iterator iter = find_matching_tag(base_tag);
    if(iter == end(base_tag.type())) {
        //First time we've seen this domain
        TimingTag tag = TimingTag(new_time, Time(NAN), base_tag);
        add_tag(tag);
    } else {
        iter->max_arr(new_time, base_tag);
    }
}

inline void TimingTags::min_req(const Time& new_time, const TimingTag& base_tag, bool arr_must_be_valid) {
    iterator iter = find_matching_tag(base_tag);
    if(iter == end(base_tag.type())) {
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
    if(iter == end(base_tag.type())) {
        //First time we've seen this domain
        TimingTag tag = TimingTag(new_time, Time(NAN), base_tag);
        add_tag(tag);
    } else {
        iter->min_arr(new_time, base_tag);
    }
}

inline void TimingTags::max_req(const Time& new_time, const TimingTag& base_tag, bool arr_must_be_valid) {
    iterator iter = find_matching_tag(base_tag);
    if(iter == end(base_tag.type())) {
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
    //Linear search for matching tag
    auto b = begin(tag.type());
    auto e = end(tag.type());
    for(auto iter = b; iter != e; ++iter) {
        if(iter->type() == tag.type() && iter->clock_domain() == tag.clock_domain()) {
            return iter;
        }
    }
    return e;
}

} //namepsace
