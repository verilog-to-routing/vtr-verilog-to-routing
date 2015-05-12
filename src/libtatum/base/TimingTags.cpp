#include "assert.hpp"

#include "TimingTags.hpp"
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
void TimingTag::update(const Time& new_time, const TimingTag& base_tag) {
    //NOTE: leave next alone, since we want to keep the linked list intact
    set_time(new_time);
    ASSERT(clock_domain() == base_tag.clock_domain()); //Domain must be the same
    set_launch_node(base_tag.launch_node());
}

/*
 * TimingTags implementation
 */

//Modifiers
void TimingTags::add_tag(MemoryPool& tag_pool, const Time& new_time, const TimingTag& base_tag) {
    //Don't add invalid clock domains
    //Some sources like constant generators may yeild illegal clock domains
    if(base_tag.clock_domain() == INVALID_CLOCK_DOMAIN) {
        return;
    }

#if NUM_FLAT_TAGS >= 1
    if(num_tags_ < (int) head_tags_.max_size()) {
        //Store it as a head tag
        head_tags_[num_tags_] = TimingTag(new_time, base_tag);
        if(num_tags_ != 0) {
            //Link from previous if it exists
            head_tags_[num_tags_-1].set_next(&head_tags_[num_tags_]);
        }
    } else {
        //Store it in a linked list from head tags

        //Allocate form a central storage pool
        VERIFY(tag_pool.get_requested_size() == sizeof(TimingTag)); //Make sure the pool is the correct size
        TimingTag* tag = new(tag_pool.malloc()) TimingTag(new_time, base_tag);

        //Insert one-after the last head in O(1) time
        //Note that we don't maintain the tags in any order since we expect a relatively small number of tags
        //per node
        TimingTag* next_tag = head_tags_[head_tags_.max_size()-1].next(); //Save next link (may be nullptr)
        head_tags_[head_tags_.max_size()-1].set_next(tag); //Tag is now in the list
        tag->set_next(next_tag); //Attach tail of the list
    }
#else
    //Allocate form a central storage pool
    VERIFY(tag_pool.get_requested_size() == sizeof(TimingTag)); //Make sure the pool is the correct size
    TimingTag* tag = new(tag_pool.malloc()) TimingTag(new_time, base_tag);

    if(head_tags_ == nullptr) {
        head_tags_ = tag;
    } else {
        //Insert as the head in O(1) time
        //Note that we don't maintain the tags in any order since we expect a relatively small number of tags
        //per node
        TimingTag* next_tag = head_tags_; //Save next link (may be nullptr)
        head_tags_ = tag; //Tag is now in the list
        tag->set_next(next_tag); //Attach tail of the list
    }
#endif
    //Tag has been added
    num_tags_++;
}

void TimingTags::add_tag(MemoryPool& tag_pool, const TimingTag& base_tag) {
    add_tag(tag_pool, base_tag.time(), base_tag);
}

void TimingTags::max_tag(MemoryPool& tag_pool, const Time& new_time, const TimingTag& base_tag) {
    TimingTagIterator iter = find_tag_by_clock_domain(base_tag.clock_domain());
    if(iter == end()) { 
        //First time we've seen this domain
        add_tag(tag_pool, new_time, base_tag);
    } else {
        TimingTag& matched_tag = *iter;

        //Need to max with existing value
        if(new_time.value() > matched_tag.time().value()) {
            //New value is larger
            //Update max
            matched_tag.update(new_time, base_tag);
        }
    }
}

void TimingTags::min_tag(MemoryPool& tag_pool, const Time& new_time, const TimingTag& base_tag) {
    TimingTagIterator iter = find_tag_by_clock_domain(base_tag.clock_domain());
    if(iter == end()) { 
        //First time we've seen this domain
        add_tag(tag_pool, new_time, base_tag);
    } else {
        TimingTag& matched_tag = *iter;

        //Need to min with existing value
        if(new_time.value() < matched_tag.time().value()) {
            //New value is smaller
            //Update min
            matched_tag.update(new_time, base_tag);
        }
    }
}

void TimingTags::clear() {
    //Note that we do not clean up the tag linked list!
    //Since these are allocated in a memory pool they will be freed by
    //the owner of the pool (typically the analyzer that is calling us)
    num_tags_ = 0;

#if NUM_FLAT_TAGS == 0
    //If we are using an un-flattend linked list
    //we must reset the head pointer
    head_tags_ = nullptr;
#endif
}

TimingTagIterator TimingTags::find_tag_by_clock_domain(DomainId domain_id) {
    auto pred = [domain_id](const TimingTag& tag) {
        return tag.clock_domain() == domain_id;
    };
    return std::find_if(begin(), end(), pred);
}

TimingTagConstIterator TimingTags::find_tag_by_clock_domain(DomainId domain_id) const {
    auto pred = [domain_id](const TimingTag& tag) {
        return tag.clock_domain() == domain_id;
    };
    return std::find_if(begin(), end(), pred);
}
