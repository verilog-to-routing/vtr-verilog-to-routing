#pragma once

#include "TimingTag.hpp"
#include "tatum_range.hpp"

namespace tatum {

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
 * Since each node in the timing graph typically has only a few tags (usually 1 or 2), we
 * perform linear searches to find match tags and tag ranges.
 *
 * Note that to allow efficient iteration of tag ranges (by type) we ensure that tags of the
 * same type are adjacent in the storage vector (i.e. the vector is sorted by type)
 */
class TimingTags {
    private:
        typedef std::vector<TimingTag> TagStore;

        //In practice the vast majority of nodes have only two or one
        //tags, so we reserve space for two to avoid costly memory
        //allocations
        constexpr static size_t DEFAULT_TAGS_TO_RESERVE = 2;

    public:
        typedef TagStore::iterator iterator;
        typedef TagStore::const_iterator const_iterator;

        typedef tatum::util::Range<const_iterator> tag_range;

    public:

        //Constructors
        TimingTags(size_t num_reserve=DEFAULT_TAGS_TO_RESERVE);
        TimingTags(const TimingTags&) = delete;
        TimingTags(TimingTags&&) = default;
        TimingTags& operator=(const TimingTags&) = delete;
        TimingTags& operator=(TimingTags&&) = default;

        /*
         * Getters
         */
        ///\returns The number of timing tags in this set
        size_t size() const;
        bool empty() const { return size() == 0; }

        ///\returns An iterator to the first tag in the current set
        iterator begin();
        const_iterator begin() const;

        ///\returns An iterator 'one-past-the-end' of the current set
        iterator end();
        const_iterator end() const;

        ///\returns A range of all tags
        tag_range tags() const;

        ///\returns A range of all tags matching type
        tag_range tags(const TagType type) const;


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
        void min_req(const Time& new_time, const TimingTag& base_tag, bool arr_must_be_valid);

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
        void max_req(const Time& new_time, const TimingTag& base_tag, bool arr_must_be_valid);

        ///Clears the tags in the current set
        ///\warning Note this does not deallocate the tags. Tag deallocation is the responsibility of the associated pool allocator
        void clear();


    private:
        ///Finds a TimingTag in the current set that has clock domain id matching domain_id
        ///\param domain_id The clock domain id to look for
        ///\returns An iterator to the tag if found, or end() if not found
        iterator find_matching_tag(const TimingTag& tag);

        TagStore tags_;
};

} //namepsace

//Implementation
#include "TimingTags.inl"
