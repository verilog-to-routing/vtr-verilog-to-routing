#pragma once

#include "TimingTag.hpp"
#include "memory_pool.hpp"

//How many timing tag objects should be flattened into the TimingTags class?
//  A value of 1 tends to help cache locality and performs best
#define NUM_FLAT_TAGS 1


class TimingTags {
    public:
        //Getters
        size_t num_tags() const { return num_tags_; };
        TimingTagIterator find_tag_by_clock_domain(DomainId domain_id);
        TimingTagConstIterator find_tag_by_clock_domain(DomainId domain_id) const;
#if NUM_FLAT_TAGS >= 1
        TimingTagIterator begin() { return (num_tags_ > 0) ? TimingTagIterator(&head_tags_[0]) : end(); };
        TimingTagConstIterator begin() const { return (num_tags_ > 0) ? TimingTagConstIterator(&head_tags_[0]) : end(); };
#else
        TimingTagIterator begin() { return TimingTagIterator(head_tags_); };
        TimingTagConstIterator begin() const { return TimingTagConstIterator(head_tags_); };
#endif
        TimingTagIterator end() { return TimingTagIterator(nullptr); };
        TimingTagConstIterator end() const { return TimingTagConstIterator(nullptr); };

        //Modifiers
        //void add_tag(MemoryPool& tag_pool, const Time& new_time, const TimingTag& src_tag);
        void add_tag(MemoryPool& tag_pool, const TimingTag& src_tag);
        void max_arr(MemoryPool& tag_pool, const Time& new_time, const TimingTag& src_tag);
        void min_req(MemoryPool& tag_pool, const Time& new_time, const TimingTag& src_tag);
        void min_req_tag(TimingTag& matched_tag, const Time& new_time, const TimingTag& base_tag);
        void clear();


    private:
        int num_tags_;

#if NUM_FLAT_TAGS >= 1
        //The first NUM_FLAT_TAGS tags are stored directly as members
        //of this object. Any additional tags are stored in a dynamically
        //allocated linked list.
        //Note that despite being an array, each element of head_tags_ is
        //hooked into the linked list
        std::array<TimingTag, NUM_FLAT_TAGS> head_tags_;
#else
        TimingTag* head_tags_;
#endif
};


