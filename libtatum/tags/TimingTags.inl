#include <algorithm>
#include "tatum_assert.hpp"

namespace tatum {

/*
 * TimingTags implementation
 */

inline TimingTags::TimingTags(size_t num_reserve)
    : size_(0)
    , capacity_(num_reserve)
    , num_clock_launch_tags_(0)
    , num_clock_capture_tags_(0)
    , tags_(capacity_ ? new TimingTag[capacity_] : nullptr)
    {}

inline TimingTags::TimingTags(const TimingTags& other) 
    : size_(other.size())
    , capacity_(size_)
    , num_clock_launch_tags_(other.num_clock_launch_tags_)
    , num_clock_capture_tags_(other.num_clock_capture_tags_)
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
    iterator iter;
    switch(type) {
        case TagType::CLOCK_LAUNCH: 
            iter = begin();
            break;
        case TagType::CLOCK_CAPTURE: 
            iter = begin() + num_clock_launch_tags_;
            break;
        case TagType::DATA: 
            iter = begin() + num_clock_launch_tags_ + num_clock_capture_tags_;
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
            iter = begin();
            break;
        case TagType::CLOCK_CAPTURE: 
            iter = begin() + num_clock_launch_tags_;
            break;
        case TagType::DATA: 
            iter = begin() + num_clock_launch_tags_ + num_clock_capture_tags_;
            break;
        default:
            TATUM_ASSERT_MSG(false, "Invalid tag type");
    }
    return iter;
}

inline TimingTags::iterator TimingTags::end() {
    auto iter = iterator(tags_.get() + size_);
    TATUM_ASSERT_SAFE(iter.p_ >= tags_.get() && iter.p_ <= tags_.get() + size());
    return iter;
}

inline TimingTags::const_iterator TimingTags::end() const {
    auto iter = const_iterator(tags_.get() + size_);
    TATUM_ASSERT_SAFE(iter.p_ >= tags_.get() && iter.p_ <= tags_.get() + size());
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
            iter = end();
            break;
        default:
            TATUM_ASSERT_MSG(false, "Invalid tag type");
    }

    TATUM_ASSERT_SAFE(iter.p_ >= tags_.get() && iter.p_ <= tags_.get() + size());
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
    TATUM_ASSERT(tag.clock_domain());

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
    size_ = 0;
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

inline size_t TimingTags::capacity() const { return capacity_; }

inline TimingTags::iterator TimingTags::insert(iterator iter, const TimingTag& tag) {
    size_t index = std::distance(begin(), iter);
    TATUM_ASSERT(index <= size());

    //TODO: optimize combined growth + insert
    if(capacity() == 0 || capacity() == size()) {
        //Grow and insert simultaneously
        grow_insert(index, tag);
    } else {
        //Insert into existing capacity
        TATUM_ASSERT(size() + 1 <= capacity());

        //Shift everything one position right from end to index
        for(size_t i = size(); i != index; i--) {
            tags_[i] = tags_[i - 1];
        }

        //Insert the new value in the hole at index created by shifting
        tags_[index] = tag;

        //Update the sizes
        ++size_;
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
    }

    return begin() + index;
}

inline void TimingTags::grow_insert(size_t index, const TimingTag& tag) {
    size_t new_capacity = (capacity() == 0) ? 1 : GROWTH_FACTOR * capacity();

    std::unique_ptr<TimingTag[]> new_tags(new TimingTag[new_capacity]);
    std::copy_n(tags_.get(), index, new_tags.get()); //Copy before index
    new_tags[index] = tag; //Insert the new value
    std::copy_n(tags_.get() + index, size() - index, new_tags.get() + index + 1); //Copy after index

    //Swap the tags
    std::swap(tags_, new_tags);

    //Update the capacity
    capacity_ = new_capacity;

    //Update the sizes
    ++size_;
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
}

inline void swap(TimingTags& lhs, TimingTags& rhs) {
    std::swap(lhs.tags_, rhs.tags_);
    std::swap(lhs.num_clock_launch_tags_, rhs.num_clock_launch_tags_);
    std::swap(lhs.num_clock_capture_tags_, rhs.num_clock_capture_tags_);
    std::swap(lhs.size_, rhs.size_);
    std::swap(lhs.capacity_, rhs.capacity_);
}

} //namepsace
