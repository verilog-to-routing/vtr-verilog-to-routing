#pragma once

#include "TimingTag.hpp"
#include "memory_pool.hpp"

//How many timing tag objects should be flattened into the TimingTags class?
//  A value of 1 tends to help cache locality and performs best
#define NUM_FLAT_TAGS 1

/*
 * The 'TimingTags' class represents the collection of timing tags (see the 'TimingTag' class)
 * that belong to a particular node in the timing graph.
 *
 * Any operations performed using this task generally consider *all* associated tags.
 * For example, preforming a max_arr() call will apply the max accross all tags with matching
 * clock domain.  If no matching tag is found, the tag will be added to the set of tags.
 *
 * Implementation
 * ====================
 * Nominally the set of tags is implemented as a singly linked list, since each node in the timing graph
 * typically has only a handful (usually << 10) tags this linear search is not too painful.
 *
 * Tag Flattening
 * ----------------
 * As a performance optimization we flatten the first tags of the list as members of this class. 
 * This ensures that the first tag is usually already in the CPU cache before it is queried,
 * offering a noticable speed-up. How many tags are embedded is controlled by the NUM_FLAT_TAGS
 * define.
 *
 * The number of tags to flatten offers a trade-off between better cache locality (the embedded tags
 * will be pulled into the cache with the TimingTags class), and increased memory usage (not all nodes
 * may require all of the flattend tags, in which case it also hurts cache locality.
 *
 * Flattening only the first tag usually offers the best performance (most nodes have only one tag). 
 *
 * Tag Memory allocation
 * -----------------------
 * Note that tags are allocated from a memory pool (tag_pool argument to some functions) which is
 * owned by the analyzer.  Since they are pool allocated, this class does not handle freeing
 * the allocated tags.
 */
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
