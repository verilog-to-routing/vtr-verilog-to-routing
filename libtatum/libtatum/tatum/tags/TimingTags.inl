#include <algorithm>
#include "tatum/util/tatum_assert.hpp"

namespace tatum {

/*
 * TimingTags implementation
 */

//TODO: given that we know we typically add tags in CLOCK_LAUNCH, DATA_ARRIVAL, CLOCK_CAPTURE, DATA_REQUIRED
//      order, we should probably order their storage that way

inline TimingTags::TimingTags(size_t num_reserve)
    : size_(0)
    , capacity_(num_reserve)
    , num_clock_launch_tags_(0)
    , num_clock_capture_tags_(0)
    , num_data_arrival_tags_(0)
    , num_data_required_tags_(0)
    , tags_(capacity_ ? new TimingTag[capacity_] : nullptr)
    {}

inline TimingTags::TimingTags(const TimingTags& other) 
    : size_(other.size())
    , capacity_(size_)
    , num_clock_launch_tags_(other.num_clock_launch_tags_)
    , num_clock_capture_tags_(other.num_clock_capture_tags_)
    , num_data_arrival_tags_(other.num_data_arrival_tags_)
    , num_data_required_tags_(other.num_data_required_tags_)
    , tags_(capacity_ ? new TimingTag[capacity_] : nullptr) {
    std::copy(other.tags_.get(), other.tags_.get() + other.size(), tags_.get());
}

inline TimingTags::TimingTags(TimingTags&& other)
    : TimingTags(0) {
    swap(*this, other);
}

inline TimingTags& TimingTags::operator=(TimingTags other) {
    swap(*this, other);
    return *this;
}

inline size_t TimingTags::size() const { 
    return size_;
}

inline TimingTags::iterator TimingTags::begin() {
    auto iter = iterator(tags_.get());

    return iter;
}

inline TimingTags::const_iterator TimingTags::begin() const {
    return const_iterator(tags_.get());
}

inline TimingTags::iterator TimingTags::begin(TagType type) {
    const_iterator const_iter = const_cast<const TimingTags*>(this)->begin(type);
    return iterator(const_cast<TimingTag*>(const_iter.p_));
}

inline TimingTags::const_iterator TimingTags::begin(TagType type) const {
    const_iterator iter;
    switch(type) {
        case TagType::CLOCK_LAUNCH: 
            iter = begin();
            break;
        case TagType::CLOCK_CAPTURE: 
            iter = begin() + num_clock_launch_tags_;
            break;
        case TagType::DATA_ARRIVAL: 
            iter = begin() + num_clock_launch_tags_ + num_clock_capture_tags_;
            break;
        case TagType::DATA_REQUIRED: 
            iter = begin() + num_clock_launch_tags_ + num_clock_capture_tags_ + num_data_arrival_tags_;
            break;
        case TagType::SLACK: 
            iter = begin() + num_clock_launch_tags_ + num_clock_capture_tags_ + num_data_arrival_tags_ + num_data_required_tags_;
            break;
        default:
            TATUM_ASSERT_MSG(false, "Invalid tag type");
    }
    return iter;
}

inline TimingTags::iterator TimingTags::end() {
    const_iterator const_iter = const_cast<const TimingTags*>(this)->end();
    return iterator(const_cast<TimingTag*>(const_iter.p_));
}

inline TimingTags::const_iterator TimingTags::end() const {
    auto iter = const_iterator(tags_.get() + size_);
    TATUM_ASSERT_SAFE(iter.p_ >= tags_.get() && iter.p_ <= tags_.get() + size());
    return iter;
}


inline TimingTags::iterator TimingTags::end(TagType type) { 
    const_iterator const_iter = const_cast<const TimingTags*>(this)->end(type);
    return iterator(const_cast<TimingTag*>(const_iter.p_));
}

inline TimingTags::const_iterator TimingTags::end(TagType type) const { 
    const_iterator iter;
    switch(type) {
        case TagType::CLOCK_LAUNCH: 
            iter = begin(TagType::CLOCK_CAPTURE);
            break;
        case TagType::CLOCK_CAPTURE: 
            iter = begin(TagType::DATA_ARRIVAL);
            break;
        case TagType::DATA_ARRIVAL: 
            iter = begin(TagType::DATA_REQUIRED);
            break;
        case TagType::DATA_REQUIRED: 
            iter = begin(TagType::SLACK);
            break;
        case TagType::SLACK: 
            //Pass the true end
            iter = end();
            break;
        default:
            TATUM_ASSERT_MSG(false, "Invalid tag type");
    }
    TATUM_ASSERT_SAFE(iter.p_ >= tags_.get() && iter.p_ <= tags_.get() + size());
    return iter;
}

inline TimingTags::tag_range TimingTags::tags() const {
    return tatum::util::make_range(begin(), end());
}

inline TimingTags::tag_range TimingTags::tags(const TagType type) const {
    return tatum::util::make_range(begin(type), end(type));
}

//Modifiers
inline void TimingTags::add_tag(const TimingTag& tag) {
    //Find the position to insert this tag
    //
    //We keep tags of the same type together.
    //We also prefer to insert new tags at the end if possible
    //(since this is more efficient for the underlying vector storage)

    auto iter = end(tag.type());

    //Insert the tag before the upper bound position
    // This ensures tags_ is always in sorted order
    insert(iter, tag);
}

inline void TimingTags::max(const Time& new_time, const NodeId origin, const TimingTag& base_tag, bool arr_must_be_valid) {
    auto bool_iter = find_matching_tag(base_tag, arr_must_be_valid);

    bool valid = bool_iter.first;
    if(valid) {
        auto iter = bool_iter.second;
        if(iter == end(base_tag.type())) {
            //An exact match was not found

            //First time we've seen this domain
            TimingTag tag = TimingTag(new_time, origin, base_tag);
            add_tag(tag);
        } else {
            iter->max(new_time, origin, base_tag);
        }
    }
}

inline void TimingTags::min(const Time& new_time, const NodeId origin, const TimingTag& base_tag, bool arr_must_be_valid) {
    auto bool_iter = find_matching_tag(base_tag, arr_must_be_valid);

    bool valid = bool_iter.first;
    if(valid) {
        auto iter = bool_iter.second;
        if(iter == end(base_tag.type())) {
            //An exact match was not found

            //First time we've seen this domain
            TimingTag tag = TimingTag(new_time, origin, base_tag);
            add_tag(tag);
        } else {
            iter->min(new_time, origin, base_tag);
        }
    }
}

inline void TimingTags::clear() {
    size_ = 0;
    num_clock_launch_tags_ = 0;
    num_clock_capture_tags_ = 0;
    num_data_arrival_tags_ = 0;
    num_data_required_tags_ = 0;
}

inline std::pair<bool,TimingTags::iterator> TimingTags::find_matching_tag(const TimingTag& tag, bool arr_must_be_valid) {
    if(arr_must_be_valid) {
        TATUM_ASSERT(tag.type() == TagType::DATA_REQUIRED);
        auto bool_iter = find_data_required_with_valid_data_arrival(tag.launch_clock_domain(), tag.capture_clock_domain());
        return bool_iter;
    } else {
        auto iter = find_matching_tag(tag.type(), tag.launch_clock_domain(), tag.capture_clock_domain());
        return {true, iter};
    }
}

inline TimingTags::iterator TimingTags::find_matching_tag(TagType type, DomainId launch_domain, DomainId capture_domain) {
    //Linear search for matching tag
    auto b = begin(type);
    auto e = end(type);
    for(auto iter = b; iter != e; ++iter) {
        bool match_launch =    !launch_domain               //Search wildcard
                            || !iter->launch_clock_domain() //Match wildcard
                            || launch_domain == iter->launch_clock_domain(); //Exact match

        bool match_capture =   !capture_domain               //Search wildcard
                            || !iter->capture_clock_domain() //Match wildcard
                            || capture_domain == iter->capture_clock_domain(); //Exact match

        if(match_launch && match_capture) {
            return iter;
        }
    }
    return e;
}

inline std::pair<bool,TimingTags::iterator> TimingTags::find_data_required_with_valid_data_arrival(DomainId launch_domain, DomainId capture_domain) {
    //Look for the matching arrival
    auto arr_iter = find_matching_tag(TagType::DATA_ARRIVAL, launch_domain, DomainId::INVALID());
    if(arr_iter == end(TagType::DATA_ARRIVAL) || !arr_iter->time().valid()) {
        //No valid arrival
        return {false, end(TagType::DATA_REQUIRED)};
    }

    //Find the matching required
    return {true, find_matching_tag(TagType::DATA_REQUIRED, launch_domain, capture_domain)};
}

inline size_t TimingTags::capacity() const { return capacity_; }

inline TimingTags::iterator TimingTags::insert(iterator iter, const TimingTag& tag) {
    size_t index = std::distance(begin(), iter);
    TATUM_ASSERT(index <= size());

    if(capacity() == 0 || capacity() == size()) {
        //Grow and insert simultaneously
        grow_insert(index, tag);
    } else {
        //Insert into existing capacity
        TATUM_ASSERT(size() + 1 <= capacity());

        //Shift everything one position right from end to index
        //TODO: use std::copy_backward?
        for(size_t i = size(); i != index; --i) {
            tags_[i] = tags_[i - 1];
        }

        //Insert the new value in the hole at index created by shifting
        tags_[index] = tag;

        //Update the sizes
        increment_size(tag.type());
    }

    return begin() + index;
}

inline void TimingTags::grow_insert(size_t index, const TimingTag& tag) {
    size_t new_capacity = (capacity() == 0) ? 1 : GROWTH_FACTOR * capacity();

    //We construct a new copy of ourselves at the new capacity and with the new
    //tag inserted
    TimingTags new_tags(new_capacity);

    std::copy_n(tags_.get(), index, new_tags.tags_.get()); //Copy before index
    new_tags.tags_[index] = tag; //Insert the new value
    std::copy_n(tags_.get() + index, size() - index, new_tags.tags_.get() + index + 1); //Copy after index

    //Copy the sizes
    new_tags.size_ = size_;
    new_tags.num_clock_launch_tags_ = num_clock_launch_tags_;
    new_tags.num_clock_capture_tags_ = num_clock_capture_tags_;
    new_tags.num_data_arrival_tags_ = num_data_arrival_tags_;
    new_tags.num_data_required_tags_ = num_data_required_tags_;

    //Increment to account for the inserted tag
    new_tags.increment_size(tag.type());

    //Update ourselves
    swap(*this, new_tags);
}

inline void TimingTags::increment_size(TagType type) {
    ++size_;
    switch(type) {
        case TagType::CLOCK_LAUNCH: 
            ++num_clock_launch_tags_;
            break;
        case TagType::CLOCK_CAPTURE: 
            ++num_clock_capture_tags_;
            break;
        case TagType::DATA_ARRIVAL: 
            ++num_data_arrival_tags_;
            break;
        case TagType::DATA_REQUIRED: 
            ++num_data_required_tags_;
            break;
        case TagType::SLACK: 
            //Pass
            break;
        default:
            TATUM_ASSERT_MSG(false, "Invalid tag type");
    }
}

inline void swap(TimingTags& lhs, TimingTags& rhs) {
    std::swap(lhs.tags_, rhs.tags_);
    std::swap(lhs.num_clock_launch_tags_, rhs.num_clock_launch_tags_);
    std::swap(lhs.num_clock_capture_tags_, rhs.num_clock_capture_tags_);
    std::swap(lhs.num_data_arrival_tags_, rhs.num_data_arrival_tags_);
    std::swap(lhs.num_data_required_tags_, rhs.num_data_required_tags_);
    std::swap(lhs.size_, rhs.size_);
    std::swap(lhs.capacity_, rhs.capacity_);
}

} //namepsace
