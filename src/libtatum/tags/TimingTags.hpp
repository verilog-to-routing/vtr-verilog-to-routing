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
        template<class TagPoolType>
        void add_tag(TagPoolType& tag_pool, const TimingTag& src_tag);

        //Setup operations
        template<class TagPoolType>
        void max_arr(TagPoolType& tag_pool, const Time& new_time, const TimingTag& base_tag);

        template<class TagPoolType>
        void min_req(TagPoolType& tag_pool, const Time& new_time, const TimingTag& base_tag);

        //Hold operations
        template<class TagPoolType>
        void min_arr(TagPoolType& tag_pool, const Time& new_time, const TimingTag& base_tag);

        template<class TagPoolType>
        void max_req(TagPoolType& tag_pool, const Time& new_time, const TimingTag& base_tag);

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

//Implementation
#include "TimingTags.inl"
