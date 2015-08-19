#pragma once

#include "TimingTag.hpp"
#include "memory_pool.hpp"

//How many timing tag objects should be flattened into the TimingTags class?
//  A value of 1 tends to help cache locality and performs best
#define NUM_FLAT_TAGS 1

/**
 * The 'TimingTags' class represents a collection of timing tags (see the 'TimingTag' class)
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
 * typically owned by the TimingAnalyzer. Since they are pool allocated, this class does not handle 
 * freeing the allocated tags.
 */
class TimingTags {
    public:
        /*
         * Getters
         */
        ///\returns The number of timing tags in this set
        size_t num_tags() const { return num_tags_; };

        ///Finds a TimingTag in the current set that has clock domain id matching domain_id
        ///\param domain_id The clock domain id to look for
        ///\returns An iterator to the tag if found, or end() if not found
        TimingTagIterator find_tag_by_clock_domain(DomainId domain_id);
        TimingTagConstIterator find_tag_by_clock_domain(DomainId domain_id) const;

        ///\returns An iterator to the first tag in the current set
        TimingTagIterator begin() { return (num_tags_ > 0) ? TimingTagIterator(&head_tags_[0]) : end(); };
        TimingTagConstIterator begin() const { return (num_tags_ > 0) ? TimingTagConstIterator(&head_tags_[0]) : end(); };

        ///\returns An iterator 'one-past-the-end' of the current set
        TimingTagIterator end() { return TimingTagIterator(nullptr); };
        TimingTagConstIterator end() const { return TimingTagConstIterator(nullptr); };

        /*
         * Modifiers
         */
        ///Adds a TimingTag to the current set provided it has a valid clock domain
        ///\param tag_pool The pool memory allocator used to allocate the tag
        ///\param src_tag The source tag who is inserted. Note that the src_tag is copied when inserted (the original is unchanged)
        template<class TagPoolType>
        void add_tag(TagPoolType& tag_pool, const TimingTag& src_tag);

        /*
         * Setup operations
         */
        ///Updates the arrival time of this set of tags to be the maximum.
        ///\param tag_pool The pool memory allocator to use
        ///\param new_time The new arrival time to compare against
        ///\param base_tag The associated metat-data for new_time
        ///\remark Finds (or creates) the tag with the same clock domain as base_tag and update the arrival time if new_time is larger
        template<class TagPoolType>
        void max_arr(TagPoolType& tag_pool, const Time& new_time, const TimingTag& base_tag);

        ///Updates the required time of this set of tags to be the minimum.
        ///\param tag_pool The pool memory allocator to use
        ///\param new_time The new arrival time to compare against
        ///\param base_tag The associated metat-data for new_time
        ///\remark Finds (or creates) the tag with the same clock domain as base_tag and update the required time if new_time is smaller
        template<class TagPoolType>
        void min_req(TagPoolType& tag_pool, const Time& new_time, const TimingTag& base_tag);

        /*
         * Hold operations
         */
        ///Updates the arrival time of this set of tags to be the minimum.
        ///\param tag_pool The pool memory allocator to use
        ///\param new_time The new arrival time to compare against
        ///\param base_tag The associated metat-data for new_time
        ///\remark Finds (or creates) the tag with the same clock domain as base_tag and update the arrival time if new_time is smaller
        template<class TagPoolType>
        void min_arr(TagPoolType& tag_pool, const Time& new_time, const TimingTag& base_tag);

        ///Updates the required time of this set of tags to be the maximum.
        ///\param tag_pool The pool memory allocator to use
        ///\param new_time The new arrival time to compare against
        ///\param base_tag The associated metat-data for new_time
        ///\remark Finds (or creates) the tag with the same clock domain as base_tag and update the required time if new_time is larger
        template<class TagPoolType>
        void max_req(TagPoolType& tag_pool, const Time& new_time, const TimingTag& base_tag);

        ///Clears the tags in the current set
        ///\warning Note this does not deallocate the tags. Tag deallocation is the responsibility of the associated pool allocator
        void clear();


    private:
        int num_tags_;

        //The first NUM_FLAT_TAGS tags are stored directly as members
        //of this object. Any additional tags are stored in a dynamically
        //allocated linked list.
        //Note that despite being an array, each element of head_tags_ is
        //hooked into the linked list
        std::array<TimingTag, NUM_FLAT_TAGS> head_tags_;
};

//Implementation
#include "TimingTags.inl"
