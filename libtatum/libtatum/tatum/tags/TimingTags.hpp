#pragma once
#include <iterator>
#include <memory>

#include "tatum/tags/TimingTag.hpp"
#include "tatum/util/tatum_range.hpp"

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
    public:
        template<class T>
        class Iterator;
    private:
        //In practice the vast majority of nodes have only a handful of tags,
        //so we reserve space for some to avoid costly memory allocations
        constexpr static size_t DEFAULT_TAGS_TO_RESERVE = 3;
        constexpr static size_t GROWTH_FACTOR = 2;

    public:

        typedef Iterator<TimingTag> iterator;
        typedef Iterator<const TimingTag> const_iterator;

        typedef tatum::util::Range<const_iterator> tag_range;

    public:

        //Constructors
        TimingTags(size_t num_reserve=DEFAULT_TAGS_TO_RESERVE);
        TimingTags(const TimingTags&);
        TimingTags(TimingTags&&);
        TimingTags& operator=(TimingTags);
        ~TimingTags() = default;
        friend void swap(TimingTags& lhs, TimingTags& rhs);

        /*
         * Getters
         */
        ///\returns The number of timing tags in this set
        size_t size() const;
        bool empty() const { return size() == 0; }

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
         * Operations
         */
        ///Updates the arrival time of this set of tags to be the maximum.
        ///\param new_time The new arrival time to compare against
        ///\param base_tag The associated metat-data for new_time
        ///\remark Finds (or creates) the tag with the same clock domain as base_tag and update the arrival time if new_time is larger
        void max(const Time& new_time, const NodeId origin, const TimingTag& base_tag, bool arr_must_be_valid=false);

        ///Updates the required time of this set of tags to be the minimum.
        ///\param new_time The new arrival time to compare against
        ///\param base_tag The associated metat-data for new_time
        ///\remark Finds (or creates) the tag with the same clock domain as base_tag and update the required time if new_time is smaller
        void min(const Time& new_time, const NodeId origin, const TimingTag& base_tag, bool arr_must_be_valid=false);

        ///Clears the tags in the current set
        void clear();

    public:

        //Iterator definition
        template<class T>
        class Iterator : public std::iterator<std::random_access_iterator_tag, T> {
            friend TimingTags;
            public:
                using value_type = typename std::iterator<std::random_access_iterator_tag, T>::value_type;
                using difference_type = typename std::iterator<std::random_access_iterator_tag, T>::difference_type;
                using pointer = typename std::iterator<std::random_access_iterator_tag, T>::pointer;
                using reference = typename std::iterator<std::random_access_iterator_tag, T>::reference;
                using iterator_category = typename std::iterator<std::random_access_iterator_tag, T>::iterator_category;
            public:
                Iterator(): p_(nullptr) {}
                Iterator(pointer p): p_(p) {}
                Iterator(const Iterator& other): p_(other.p_) {}
                Iterator& operator=(const Iterator& other) { p_ = other.p_; return *this; }

                friend bool operator==(Iterator a, Iterator b) { return a.p_ == b.p_; }
                friend bool operator!=(Iterator a, Iterator b) { return a.p_ != b.p_; }

                reference operator*() { return *p_; }
                const reference operator*() const { return *p_; } //Required for MSVC (gcc/clang are fine with only the non-cost version)
                pointer operator->() { return p_; }
                reference operator[](size_t n) { return *(p_ + n); }

                Iterator& operator++() { ++p_; return *this; }
                Iterator operator++(int) { Iterator old = *this; ++p_; return old; }
                Iterator& operator--() { --p_; return *this; }
                Iterator operator--(int) { Iterator old = *this; --p_; return old; }
                Iterator& operator+=(size_t n) { p_ += n; return *this; }
                Iterator& operator-=(size_t n) { p_ -= n; return *this; }
                friend Iterator operator+(Iterator lhs, size_t rhs) { return lhs += rhs; }
                friend Iterator operator-(Iterator lhs, size_t rhs) { return lhs -= rhs; }

                friend difference_type operator-(const Iterator lhs, const Iterator rhs) { return lhs.p_ - rhs.p_; }

                friend bool operator<(Iterator lhs, Iterator rhs) { return lhs.p_ < rhs.p_; }
                friend bool operator>(Iterator lhs, Iterator rhs) { return lhs.p_ > rhs.p_; }
                friend bool operator<=(Iterator lhs, Iterator rhs) { return lhs.p_ <= rhs.p_; }
                friend bool operator>=(Iterator lhs, Iterator rhs) { return lhs.p_ >= rhs.p_; }
                friend void swap(Iterator lhs, Iterator rhs) { std::swap(lhs.p_, rhs.p_); }
            private:
                T* p_ = nullptr;
        };

    private:

        ///\returns An iterator to the first tag in the current set
        iterator begin();
        const_iterator begin() const;
        iterator begin(TagType type);
        const_iterator begin(TagType type) const;

        ///\returns An iterator 'one-past-the-end' of the current set
        iterator end();
        const_iterator end() const;
        iterator end(TagType type);
        const_iterator end(TagType type) const;

        size_t capacity() const;

        ///Finds a timing tag in the current set which matches tag
        ///\returns A pair of bool and iterator. 
        //          The bool is true if it is valid for iterator to be processed.
        //          The iterator is not equal to end(tag.type()) if a matching tag was found
        std::pair<bool,iterator> find_matching_tag(const TimingTag& tag, bool arr_must_be_valid);

        ///Finds a TimingTag in the current set that matches the launch and capture clocks of tag
        ///\returns An iterator to the tag if found, or end(type) if not found
        iterator find_matching_tag(TagType type, DomainId launch_domain, DomainId capture_domain);

        ///Find a TimingTag matching the specified DATA_REQUIRED tag provided there is 
        //a valid associated DATA_ARRIVAL tag
        ///\returns A a pair of bool and iterator. The bool indicates if a valid arrival was found, 
        ///         the iterator is the required tag matching launch and capture which has a valid 
        //          corresponding arrival time, or end(TagType::DATA_REQUIRED)
        std::pair<bool,iterator> find_data_required_with_valid_data_arrival(DomainId launch_domain, DomainId capture_domain);


        iterator insert(iterator iter, const TimingTag& tag);
        void grow_insert(size_t index, const TimingTag& tag);

        void increment_size(TagType type);


    private:
        //We don't expect many tags in a node so unsigned short's/unsigned char's
        //should be more than sufficient. This also allows the class
        //to be packed down to 16 bytes (8 for counters, 8 for pointer)
        //
        //In its current configuration we can store at most:
        //  65536           total tags (size_ and capacity_)
        //  256             clock launch tags (num_clock_launch_tags_)
        //  256             clock capture tags (num_clock_capture_tags_)
        //  256             data arrival tags (num_data_arrival_tags_)
        //  256             data required tags (num_data_required_tags_)
        //  (65536 - 4*256) slack tags (size_ - num_*)
        unsigned short size_ = 0;
        unsigned short capacity_ = 0;
        unsigned char num_clock_launch_tags_ = 0;
        unsigned char num_clock_capture_tags_ = 0;
        unsigned char num_data_arrival_tags_ = 0;
        unsigned char num_data_required_tags_ = 0;
        std::unique_ptr<TimingTag[]> tags_;

};

/*
 * Tag utilities
 */

//Return the tag from the range [first,last) which has the lowest value
TimingTags::const_iterator find_minimum_tag(TimingTags::tag_range tags);

//Return the tag from the range [first,last) which has the highest value
TimingTags::const_iterator find_maximum_tag(TimingTags::tag_range tags);

//Return the tag for the specified clock domains
TimingTags::const_iterator find_tag(TimingTags::tag_range tags,
                                           DomainId launch_domain, 
                                           DomainId capture_domain);

//Returns true of the specified set of tags would constrain a node of type node_type
bool is_constrained(NodeType node_type, TimingTags::tag_range tags);

} //namepsace

//Implementation
#include "TimingTags.inl"
