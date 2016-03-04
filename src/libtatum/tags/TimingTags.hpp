#pragma once

#include "TimingTag.hpp"
#include "memory_pool.hpp"

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
 */
class TimingTags {
    private:
        typedef std::vector<TimingTag> TagStore;

    public:
        typedef TagStore::iterator iterator;
        typedef TagStore::const_iterator const_iterator;

        TimingTags();

        /*
         * Getters
         */
        ///\returns The number of timing tags in this set
        size_t num_tags() const;

        ///\returns An iterator to the first tag in the current set
        iterator begin();
        const_iterator begin() const;

        ///\returns An iterator 'one-past-the-end' of the current set
        iterator end();
        const_iterator end() const;

        ///Finds a TimingTag in the current set that has clock domain id matching domain_id
        ///\param domain_id The clock domain id to look for
        ///\returns An iterator to the tag if found, or end() if not found
        iterator find_tag_by_clock_domain(DomainId domain_id);
        const_iterator find_tag_by_clock_domain(DomainId domain_id) const;


        /*
         * Modifiers
         */
        ///Adds a TimingTag to the current set provided it has a valid clock domain
        ///\param tag_pool The pool memory allocator used to allocate the tag
        ///\param src_tag The source tag who is inserted. Note that the src_tag is copied when inserted (the original is unchanged)
        void add_tag(const TimingTag& src_tag);

        /*
         * Setup operations
         */
        ///Updates the arrival time of this set of tags to be the maximum.
        ///\param tag_pool The pool memory allocator to use
        ///\param new_time The new arrival time to compare against
        ///\param base_tag The associated metat-data for new_time
        ///\remark Finds (or creates) the tag with the same clock domain as base_tag and update the arrival time if new_time is larger
        void max_arr(const Time& new_time, const TimingTag& base_tag);

        ///Updates the required time of this set of tags to be the minimum.
        ///\param tag_pool The pool memory allocator to use
        ///\param new_time The new arrival time to compare against
        ///\param base_tag The associated metat-data for new_time
        ///\remark Finds (or creates) the tag with the same clock domain as base_tag and update the required time if new_time is smaller
        void min_req(const Time& new_time, const TimingTag& base_tag);

        /*
         * Hold operations
         */
        ///Updates the arrival time of this set of tags to be the minimum.
        ///\param tag_pool The pool memory allocator to use
        ///\param new_time The new arrival time to compare against
        ///\param base_tag The associated metat-data for new_time
        ///\remark Finds (or creates) the tag with the same clock domain as base_tag and update the arrival time if new_time is smaller
        void min_arr(const Time& new_time, const TimingTag& base_tag);

        ///Updates the required time of this set of tags to be the maximum.
        ///\param tag_pool The pool memory allocator to use
        ///\param new_time The new arrival time to compare against
        ///\param base_tag The associated metat-data for new_time
        ///\remark Finds (or creates) the tag with the same clock domain as base_tag and update the required time if new_time is larger
        void max_req(const Time& new_time, const TimingTag& base_tag);

        ///Clears the tags in the current set
        ///\warning Note this does not deallocate the tags. Tag deallocation is the responsibility of the associated pool allocator
        void clear();


    private:
        //How many tags to reserve space for in the constructor
        const static int num_reserved_tags_ = 1; 

        TagStore tags_;
};

//Implementation
#include "TimingTags.inl"
