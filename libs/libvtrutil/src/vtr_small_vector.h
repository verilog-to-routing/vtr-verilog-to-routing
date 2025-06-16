#pragma once

#include <memory>
#include <algorithm>
#include <limits>
#include <cstdint>
#include <array>
#include "vtr_assert.h"

namespace vtr {

namespace small_vector_impl {

/**
 * @brief The long format view of the vector.
 *
 * It consists of a dynamically allocated array, capacity and size.
 */
template<class T, class S>
struct long_format {
    T* data_ = nullptr;
    S capacity_ = 0;
    S size_ = 0;
};

/**
 * @brief The short format view of the vector. 
 *
 * It consists of an in-place (potentially empty)
 * array of objects, a pad, and a size.
 */
template<class T, class S, size_t CAPACITY, size_t PAD>
struct short_format {
    std::array<T, CAPACITY> data_;
    std::array<uint8_t, PAD> pad_; ///< Padding to keep size_ aligned in both long_format and short_format
    S size_ = 0;
};

/**
 * @brief A specialized version of short_format for padding of size zero.
 *
 * Since a std::array with zero array size may still have non-zero sizeof()
 */
template<class T, class S, size_t CAPACITY>
struct short_format<T, S, CAPACITY, 0> {
    std::array<T, CAPACITY> data_;
    S size_ = 0;
};

} // namespace small_vector_impl

/**
 * @brief vtr::small_vector is a std::vector like container which:
 *
 *   - consumes less memory: sizeof(vtr::small_vector) < sizeof(std::vector)
 *   - possibly stores elements in-place (i.e. within the object)
 * 
 * On a typical LP64 system a vtr::small_vector consumes 16 bytes by default and supports
 * vectors up to ~2^32 elements long, while a std::vector consumes 24 bytes and supports up
 * to ~2^64 elements. The type used to store the size and capacity is configurable,
 * and set by the second template parameter argument. Setting it to size_t will replicate
 * std::vector's characteristics.
 * 
 * For short vectors vtr::small_vector will try to store elements in-place (i.e. within the
 * vtr::small_vector object) instead of dynamically allocating an array (by re-using the
 * internal storage for the pointer, size and capacity). Whether this is possible depends on
 * the size and alignment requirements of the value type, as compared to
 * vtr::small_vector. If in-place storage is not possible (e.g. due to a large value
 * type, or a large number of elements) a dynamic buffer is allocated (similar to
 * std::vector).
 * 
 * This is a highly specialized container. Unless you have specifically measured it's
 * usefulness you should use std::vector.
 */
template<class T, class S = uint32_t>
class small_vector {
  public: //Types
    typedef T value_type;
    //typedef allocator_type //Allocator, unimplemented
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;

    typedef T* iterator;
    typedef const T* const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    typedef ptrdiff_t difference_type;
    typedef S size_type;

  public: //Constructors
    ///@brief constructor
    small_vector() {
        if (SHORT_CAPACITY == 0) {
            long_.data_ = nullptr;
            long_.capacity_ = 0;
        }
        set_size(0);
    }
    ///@brief constructor
    small_vector(size_type nelem)
        : small_vector() {
        reserve(nelem);
        for (size_type i = 0; i < nelem; i++) {
            emplace_back();
        }
        set_size(0);
    }

  public: //Accessors
    ///@brief Return a const_iterator to the first element
    const_iterator begin() const {
        return cbegin();
    }

    ///@brief Return a const_iterator pointing to the past-the-end element in the container.
    const_iterator end() const {
        return cend();
    }

    ///@brief Return a const_reverse_iterator pointing to the last element in the container (i.e., its reverse beginning).
    const_reverse_iterator rbegin() const {
        return crbegin();
    }

    ///@brief Return a const_reverse_iterator pointing to the theoretical element preceding the first element in the container (which is considered its reverse end).
    const_reverse_iterator rend() const {
        return crend();
    }

    ///@brief Return a const_iterator pointing to the first element in the container.
    const_iterator cbegin() const {
        if (is_short()) {
            return short_.data_.data();
        }
        return long_.data_;
    }

    ///@brief a const_iterator pointing to the past-the-end element in the container.
    const_iterator cend() const {
        if (is_short()) {
            return short_.data_.data() + size();
        }
        return long_.data_ + size();
    }

    ///@brief Return a const_reverse_iterator pointing to the last element in the container (i.e., its reverse beginning).
    const_reverse_iterator crbegin() const {
        return const_reverse_iterator(cend());
    }

    ///@brief Return a const_reverse_iterator pointing to the theoretical element preceding the first element in the container (which is considered its reverse end).
    const_reverse_iterator crend() const {
        return const_reverse_iterator(cbegin());
    }

    ///@brief return the vector size (Padding ensures long/short format sizes are always aligned)
    size_type size() const {
        return long_.size_;
    }

    ///@brief Return the maximum size
    size_t max_size() const {
        return std::numeric_limits<S>::max();
    }

    ///@brief Return the vector capacity
    size_type capacity() const {
        if (is_short()) {
            return SHORT_CAPACITY; //Fixed capacity
        }
        return long_.capacity_;
    }

    ///@brief Return true if empty
    bool empty() const { return size() == 0; }

    ///@brief Immutable indexing operator []
    const_reference operator[](size_t i) const {
        if (is_short()) {
            return short_.data_[i];
        }
        return long_.data_[i];
    }

    ///@brief Immutable at() operator
    const_reference at(size_t i) const {
        if (i > size()) {
            throw std::out_of_range("Index out of bounds");
        }
        return operator[](i);
    }

    ///@brief Return a constant reference to the first element
    const_reference front() const {
        return *begin();
    }

    ///@brief Return a constant reference to the last element
    const_reference back() const {
        return *(end() - 1);
    }

    ///@brief Return a constant pointer to the vector data
    const_pointer data() const {
        if (is_short()) {
            short_.data_;
        }
        return long_.data_;
    }

  public: //Mutators
    ///@brief Return an iterator pointing to the first element in the sequence
    iterator begin() {
        //Call const method and cast-away constness
        return const_cast<iterator>(const_cast<const small_vector<T, S>*>(this)->begin());
    }

    ///@brief Return an iterator referring to the past-the-end element in the vector container.
    iterator end() {
        return const_cast<iterator>(const_cast<const small_vector<T, S>*>(this)->end());
    }

    ///@brief Return a reverse iterator pointing to the last element in the vector (i.e., its reverse beginning).
    reverse_iterator rbegin() {
        //Call const method and cast-away constness
        return reverse_iterator(const_cast<small_vector<T, S>*>(this)->end());
    }

    ///@brief Return  a reverse iterator pointing to the theoretical element preceding the first element in the vector (which is considered its reverse end).
    reverse_iterator rend() {
        return reverse_iterator(const_cast<small_vector<T, S>*>(this)->begin());
    }

    ///@brief Resizes the container so that it contains n elements
    void resize(size_type n) {
        resize(n, value_type());
    }

    ///@brief Resizes the container so that it contains n elements and fills it with val
    void resize(size_type n, value_type val) {
        if (n < size()) {
            //Remove at end
            erase(begin() + n, end());
        } else if (n > size()) {
            //Insert new elements at end
            insert(end(), n - size(), val);
        }
    }

    /**
     * @brief Reserve memory for a spicific number of elemnts
     *
     * Don't change capacity unless requested number of elements is both:
     *   - More than the short capacity (no need to reserve up to short capacity)
     *   - Greater than the current size (capacity can never be below size)
     */
    void reserve(size_type num_elems) {
        if (num_elems > SHORT_CAPACITY && num_elems > size()) {
            change_capacity(num_elems);
        }
    }

    ///@brief Requests the container to reduce its capacity to fit its size.
    void shrink_to_fit() {
        if (!is_short()) {
            change_capacity(size());
        }
    }

    ///@brief Indexing operator []
    reference operator[](size_t i) {
        return const_cast<reference>(const_cast<const small_vector<T, S>*>(this)->operator[](i));
    }

    ///@brief at() operator
    reference at(size_t i) {
        return const_cast<reference>(const_cast<const small_vector<T, S>*>(this)->at(i));
    }

    ///@brief Returns a reference to the first element in the vector.
    reference front() {
        return const_cast<reference>(const_cast<const small_vector<T, S>*>(this)->front());
    }

    ///@brief Returns a reference to the last element in the vector.
    reference back() {
        return const_cast<reference>(const_cast<const small_vector<T, S>*>(this)->back());
    }

    pointer data() {
        return const_cast<pointer>(const_cast<const small_vector<T, S>*>(this)->data());
    }

    /**
     * @brief Assigns new contents to the vector, replacing its current contents, and modifying its size accordingly.
     *
     * Input iterators to the initial and final positions in a sequence. The range used is [first,last),
     * which includes all the elements between first and last, including the element pointed by first 
     * but not the element pointed by last.
     */
    template<class InputIterator>
    void assign(InputIterator first, InputIterator last) {
        insert(begin(), first, last);
    }

    /**
     * @brief Assigns new contents to the vector, replacing its current contents, and modifying its size accordingly.
     *
     * Resize the vector to n and fill it with val
     */
    void assign(size_type n, const value_type& val) {
        insert(begin(), n, val);
    }

    /**
     * @brief Assigns new contents to the vector, replacing its current contents, and modifying its size accordingly.
     *
     * The compiler will automatically construct such objects from initializer list declarators (il)
     */
    void assign(std::initializer_list<value_type> il) {
        assign(il.begin(), il.end());
    }

    ///@brief Construct default value_type at new location
    void push_back(value_type value) {
        auto new_ptr = next_back();

        new (new_ptr) T();

        //Since we took a copy in the argument, we can move it
        //into the new location
        *new_ptr = std::move(value);
    }

    ///@brief Removes the last element in the vector, effectively reducing the container size by one.
    void pop_back() {
        if (size() > 0) {
            erase(end() - 1);
        }
    }

    ///@brief The vector is extended by inserting new elements before the element at the specified position, effectively increasing the container size by the number of elements inserted.
    iterator insert(const_iterator position, const value_type& val) {
        return insert(position, 1, val);
    }

    /** 
     * @brief Insert a new value
     *
     * Location of position as an index, which will be
     * unchanged if the underlying storage is reallocated
     */
    iterator insert(const_iterator position, size_type n, const value_type& val) {
        size_type i = std::distance(cbegin(), position);

        /*
         * If needed, grow capacity
         *
         * Note that change_capacity will automatically convert from short to long
         * format if required.
         */
        size_type new_size = size() + n;
        if (capacity() < new_size) {
            change_capacity(new_size);
        }

        iterator first = begin() + i;
        iterator last = first + n;
        reverse_swap_elements(first, end(), end() + n - 1);

        //Insert new values at end
        std::uninitialized_fill(first, last, val);

        set_size(new_size);

        return first;
    }

    ///@brief Insert n elements at position position and fill them with value val
    iterator insert(const_iterator position, size_type n, value_type&& val) {
        return insert(position, n, value_type(val)); //TODO: optimize for moved val
    }

    //Range insert
    //template<class InputIterator>
    //iterator insert(const_iterator position, InputIterator first, InputIterator last) {
    ////Location of position as an index, which will be
    ////unchanged if the underlying storage is reallocated
    //size_type i = std::distance(cbegin(), position);
    //size_type n = std::distance(first, last);

    ////If needed, grow capacity
    ////
    ////Note that change_capacity will automatically convert from short to long
    ////format if required.
    //size_type new_size = size() + n;
    //if (capacity() < new_size) {
    //change_capacity(new_size);
    //}

    //reverse_swap_elements(begin() + i, end(), end() + n - 1);

    ////Insert new values at end
    //std::uninitialized_copy(first, last, begin() + i);

    //set_size(new_size);

    //return begin() + i;
    //}

    ///@brief Removes from the vector a single element (position).
    iterator erase(const_iterator position) {
        return erase(position, position + 1);
    }

    ///@brief Removes from the vector either a range of elements ([first,last)).
    iterator erase(const_iterator first, const_iterator last) {
        //Number of elements to erase
        size_type n = std::distance(first, last);

        //Location of position as an index, which will be
        //unchanged if the underlying storage is changed
        size_type i_first = std::distance(cbegin(), first);

        size_type new_size = size() - n;

        const_iterator position = first;

        if (!is_short() && new_size <= SHORT_CAPACITY) {
            //Convert from long format to short/in-place format

            //Keep handle on buffer and original size
            auto buff_ptr = long_.data_;
            size_type orig_size = size();

            //Copy into in-place the valid (not-to-be-erased) values in
            //[begin, first) and [last, end)
            //
            //Note that we can use uninitialized_copy since the long format
            //has only basic data types, which have no destructors to call
            auto buff_begin = buff_ptr;
            auto buff_end = buff_begin + orig_size;
            auto erase_begin = buff_ptr + i_first;
            auto erase_end = erase_begin + n;

            //Copy from beginning until start of erase
            auto inplace_ptr = short_.data_.data();
            for (auto buff_itr = buff_begin; buff_itr != erase_begin; ++buff_itr) {
                new (inplace_ptr++) T(*buff_itr);
            }
            //Copy from end of erase until end of buf
            for (auto buff_itr = erase_end; buff_itr != buff_end; ++buff_itr) {
                new (inplace_ptr++) T(*buff_itr);
            }

            VTR_ASSERT_SAFE(std::distance(short_.data_.data(), inplace_ptr) == new_size);

            //Clean-up elements in buffer and free it
            destruct_elements(buff_begin, buff_end);
            dealloc(buff_ptr);

            //New position
            position = begin() + i_first;
        } else {
            //Remove elements in either long or short formats

            iterator first2 = begin() + i_first;
            iterator last2 = first2 + n;

            //Swap all elements in [first, last) to the end.
            //That is with those within [last, end())
            if (last2 < end()) {
                swap_elements(last2, end(), first2);
            }

            //Finally destruct the elements in [last, end()); that is the
            //elements which were originally to be erased
            destruct_elements(end() - n, end());

            //Note that capacity is unchanged, so we do not need to change
            //position in this case
        }

        //Shrink size
        set_size(new_size);

        return begin() + std::distance(cbegin(), position);
    }

    ///@brief Exchanges the content of the container by the content of x, which is another vector object of the same type. Sizes may differ.
    void swap(small_vector<T, S>& other) {
        swap(*this, other);
    }

    ///@brief swaps two vectors
    friend void swap(small_vector<T, S>& lhs, small_vector<T, S>& rhs) {
        using std::swap;

        if (lhs.is_short() && rhs.is_short()) {
            //Both short
            std::swap(lhs.short_, rhs.short_);
        } else if (!lhs.is_short() && !rhs.is_short()) {
            //Both long
            std::swap(lhs.long_, rhs.long_);
        } else {
            //Mixed long/short
            VTR_ASSERT_SAFE(lhs.is_short() != rhs.is_short());

            auto& long_vec = ((lhs.is_short()) ? rhs : lhs);
            auto& short_vec = ((lhs.is_short()) ? lhs : rhs);

            /** 
             * @brief Swapping two vectors of different formats
             *
             * If the two vectors are in different formats we can't just swap them,
             * since the short format has real values (potentially with destructors),
             * while the long format has only basic data types.
             * 
             * Instead we copy the short_vec values into long, destruct the original short_vec
             * values and then set short_vec to point to long_vec's original buffer (avoids
             * extra copy of long elements).
             *
             * Save long data
             */
            pointer long_buf = long_vec.long_.data_;
            size_type long_size = long_vec.long_size_;
            size_type long_capacity = long_vec.long_.capacity_;

            /**
             * @brief Copy short data into long
             *
             * Note that the long format contains only basic data types with no destructors to call,
             * so we can use uninitialzed copy
             */
            std::uninitialized_copy(short_vec.short_.begin(), short_vec.short_.end(), long_vec.short_.data_);
            long_vec.short_.size_ = short_vec.size();

            //Destroy original elements in short
            short_vec.destruct_elements();

            //Copy long data into short
            short_vec.long_.data = long_buf;
            short_vec.long_.capacity_ = long_capacity;
            short_vec.long_.size_ = long_size;
        }
    }

    ///@brief Removes all elements from the vector (which are destroyed), leaving the container with a size of 0.
    void clear() {
        //Destruct all elements and clear size, but do not free memory
        destruct_elements();
        set_size(0);
    }

    ///@brief Inserts a new element at the end of the vector, right after its current last element. This new element is constructed in place using args as the arguments for its constructor.
    template<typename... Args>
    void emplace_back(Args&&... args) {
        //Construct in-place
        new (next_back()) T(std::forward<Args>(args)...);
    }

    //Unsupported: Emplace at position
    //template<typename... Args>
    //void emplace(const_iterator position, Args&&... args) {
    //throw std::logic_error("unimplemented");
    //}

  public: //Comparisons
    ///@brief == p[erator
    friend bool operator==(const small_vector<T, S>& lhs, const small_vector<T, S>& rhs) {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        return std::equal(lhs.begin(), lhs.end(),
                          rhs.begin());
    }

    ///@brief < operator
    friend bool operator<(const small_vector<T, S>& lhs, const small_vector<T, S>& rhs) {
        return std::lexicographical_compare(lhs.begin(), lhs.end(),
                                            rhs.begin(), rhs.end());
    }

    ///@brief != operator
    friend bool operator!=(const small_vector<T, S>& lhs, const small_vector<T, S>& rhs) {
        return !(lhs == rhs);
    }

    ///@brief > operator
    friend bool operator>(const small_vector<T, S>& lhs, const small_vector<T, S>& rhs) {
        return rhs < lhs;
    }

    ///@brief <= operator
    friend bool operator<=(const small_vector<T, S>& lhs, const small_vector<T, S>& rhs) {
        return !(rhs < lhs);
    }

    ///@brief >= operator
    friend bool operator>=(const small_vector<T, S>& lhs, const small_vector<T, S>& rhs) {
        return !(lhs < rhs);
    }

  public: //Lifetime management
    ///@brief destructor
    ~small_vector() {
        destruct_elements();
        if (!is_short()) {
            dealloc(long_.data_);
        }
    }

    ///@brief copy constructor
    small_vector(const small_vector& other) {
        if (other.is_short()) {
            ~small_vector(); //Clean-up elements

            //Copy in place
            short_ = other.short_;
        } else {
            if (!is_short() && capacity() >= other.size()) {
                //Re-use existing buffer, since it has sufficient capacity
                destruct_elements();

            } else {
                ~small_vector(); //Clean-up elements, potentially freeing buffer

                //Create new buffer of exact size
                long_.data_ = alloc(other.size());
                long_.capacity_ = other.size();
            }

            set_size(other.size());

            //Copy elements
            std::uninitialized_copy(other.begin(), other.end(), long_.data_);
        }
    }

    ///@brief copy and swap constructor
    small_vector(small_vector&& other)
        : small_vector() {
        swap(*this, other); //Copy-swap
    }

    small_vector& operator=(small_vector other) {
        swap(*this, other); //Copy-swap
        return *this;
    }

  private: //Internal types
    static constexpr size_t LONG_FMT_SIZE = sizeof(small_vector_impl::long_format<value_type, size_type>);
    static constexpr size_t LONG_FMT_ALIGN = alignof(small_vector_impl::long_format<value_type, size_type>);

    ///@brief The number of value types which can be stored in-place in the object (may be zero)
    static constexpr size_t SHORT_CAPACITY = (LONG_FMT_SIZE - sizeof(size_type)) / sizeof(value_type);

    /**
     * @brief required padding
     *
     * The amount of padding required to ensure the size_ attributes of long_format and short_format
     * are aligned.
     */
    static constexpr size_t SHORT_PAD = LONG_FMT_SIZE - (sizeof(value_type) * SHORT_CAPACITY + sizeof(size_type));

    static constexpr size_t SHORT_FMT_SIZE = sizeof(small_vector_impl::short_format<value_type, size_type, SHORT_CAPACITY, SHORT_PAD>);
    static constexpr size_t SHORT_FMT_ALIGN = alignof(small_vector_impl::short_format<value_type, size_type, SHORT_CAPACITY, SHORT_PAD>);

    static_assert(LONG_FMT_SIZE == SHORT_FMT_SIZE, "Short and long data formats must have same size");
    static_assert(LONG_FMT_ALIGN % SHORT_FMT_ALIGN == 0, "Short and long data formats must have compatible alignment");

  public:
    static constexpr size_t INPLACE_CAPACITY = SHORT_CAPACITY;

  private: //Internal methods
    /**
     * @brief Returns a pointer to the (uninitialized) location for the next element to be added.
     *
     * Automatically grows the storage if needed.
     */
    T* next_back() {
        T* next = nullptr;
        if (size() < SHORT_CAPACITY) { //Space in-place
            next = short_.data_.data() + size();
        } else { //Dynamically allocated
            if (size() == capacity()) {
                //Out of space
                grow();
            }
            next = long_.data_ + size();
        }
        ++long_.size_;
        VTR_ASSERT_SAFE(size() <= capacity());
        return next;
    }

    /**
     * @brief Increases the capacity by GROWTH_FACTOR
     *
     * Note that this automatically handles the case of growing beyond SHORT_CAPACITY and
     * switching to long_format
     */
    void grow() {
        //How much to scale the size of the storage when out of space
        constexpr size_type GROWTH_FACTOR = 2;

        VTR_ASSERT_SAFE_MSG(size() >= SHORT_CAPACITY, "Should only grow capacity when at or beyond SHORT_CAPACITY");
        VTR_ASSERT_SAFE_MSG(capacity() <= (max_size() / GROWTH_FACTOR), "No capacity overflow");
        size_type new_capacity = std::max<size_type>(1, capacity() * GROWTH_FACTOR);
        //TODO: Consider ensuring new_capacity is always a power of 2, may be easier on the memory allocator...

        VTR_ASSERT_SAFE_MSG(new_capacity > capacity(), "Grown capacity should be greater than previous capacity");

        change_capacity(new_capacity);
    }

    /**
     * @brief Changes capacity to new_capacity
     *
     * It is assumed that new_capacity is > SHORT_CAPACITY.
     *
     * If currently in short format, automatically converts to long format
     */
    void change_capacity(size_type new_capacity) {
        VTR_ASSERT_SAFE_MSG(new_capacity >= size(), "New capacity should be at least size");

        if (new_capacity == capacity()) {
            return; //Already at correct capacity
        }

        //Get new raw memory
        T* tmp_data = alloc(new_capacity);

        //Copy values
        std::uninitialized_copy(begin(), end(), tmp_data);

        //Clean-up the old values
        //We do this before updating the array pointer, since if we are updating
        //from short to long the assignment would corrupt the old values
        destruct_elements();

        //Update
        std::swap(long_.data_, tmp_data);
        long_.capacity_ = new_capacity;

        //Free memory if we aren't using the inplace buffer
        if (!is_short()) {
            dealloc(tmp_data);
        }
    }

    ///@brief Returns true if using the short/in-place format
    bool is_short() const {
        return SHORT_CAPACITY > 0u          //Can use the inplace buffer
               && size() <= SHORT_CAPACITY; //Not using the dynamic buffer
    }

    /**
     * @brief set the size 
     *
     * The two data (short/long) are padded to
     * ensure that thier size_ members area always
     * aligned, allowing is to set the size directly
     * for both formats
     */
    void set_size(size_type new_size) {
        short_.size_ = new_size;
    }

    ///@brief Allocates raw (un-initialzied) memory for nelem objects of type T
    static T* alloc(size_type nelem) {
        return static_cast<T*>(::operator new(sizeof(T) * nelem));
    }

    /**
     * @brief Deallocates a block of memory
     * 
     * Caller must ensure any object's associated with this block have already had
     * their destructors called
     */
    static void dealloc(T* data) {
        ::operator delete(data);
    }

    /**
     * @brief Swaps the elements in [src_first, src_last) to positions starting at dst_first
     *
     * Returns an iterator to the element in the first swapped location
     */
    iterator swap_elements(iterator src_first, iterator src_last, iterator dst_first) {
        VTR_ASSERT_SAFE_MSG(src_first < src_last, "First swap range first must start before last");

        auto dst_itr = dst_first;
        for (auto src_itr = src_first; src_itr != src_last; ++src_itr) {
            std::swap(*src_itr, *(dst_itr++));
        }

        return src_first;
    }

    /**
     * @brief Swaps the elements in [src_first, src_last) in reverse order starting at dst_first and working backwards
     *
     * Returns an iterator to the element in the first swapped location
     */
    iterator reverse_swap_elements(iterator src_first, iterator src_last, iterator dst_first) {
        VTR_ASSERT_SAFE_MSG(src_first < src_last, "First swap range first must start before last");

        auto dst_itr = dst_first;
        for (auto src_itr = src_last - 1; src_itr != src_first - 1; --src_itr) {
            std::swap(*src_itr, *(dst_itr--));
        }

        return src_first;
    }

    ///@brief Calls the destructors of all elements currently held
    void destruct_elements() {
        destruct_elements(begin(), end());
    }

    ///@brief Calls the destructors of elements in [first, last] range
    void destruct_elements(iterator first, iterator last) {
        for (auto itr = first; itr != last; ++itr) {
            itr->~T();
        }
    }

    ///@brief Calls the destructors of elements in one position (position)
    void destruct_element(iterator position) {
        destruct_elements(position, position + 1);
    }

  private: //Data
    /*
     * The object data storage is re-used between the long and short formats.
     *
     * If the capacity is small (less than or equal to SHORT_CAPACITY) the
     * short format (which stores element in-place) is used. Otherwise the
     * long format is used and the elements are stored in a dynamically
     * allocated buffer
     */
    union {
        small_vector_impl::long_format<value_type, size_type> long_;
        small_vector_impl::short_format<value_type, size_type, SHORT_CAPACITY, SHORT_PAD> short_;
    };
};

} // namespace vtr
