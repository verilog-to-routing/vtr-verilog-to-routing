#ifndef VTR_STRONG_ID_H
#define VTR_STRONG_ID_H
/*
 * This header provides the StrongId class, a template which can be used to
 * create strong Id's which avoid accidental type conversions (generating
 * compiler errors when they occur).
 *
 * Motivation
 * ==========
 * It is common to use an Id (typically an integer) to identify and represent a component.
 * A basic example (poor style):
 *
 *      size_t count_net_terminals(int net_id);
 *
 * Where a plain int is used to represent the net identifier.
 * Using a plain basic type is poor style since it makes it unclear that the parameter is
 * an Id.
 *
 * A better example is to use a typedef:
 *
 *      typedef int NetId;
 *
 *      size_t count_net_teriminals(NetId net_id);
 *
 * It is now clear that the parameter is expecting an Id.
 *
 * However this approach has some limitations. In particular, typedef's only create type
 * aliases, and still allow conversions. This is problematic if there are multiple types
 * of Ids. For example:
 *
 *      typedef int NetId;
 *      typedef int BlkId;
 *
 *      size_t count_net_teriminals(NetId net_id);
 *
 *      BlkId blk_id = 10;
 *      NetId net_id = 42;
 *
 *      count_net_teriminals(net_id); //OK
 *      count_net_teriminals(blk_id); //Bug: passed a BlkId as a NetId
 *
 * Since typdefs are aliases the compiler issues no errors or warnings, and silently passes
 * the BlkId where a NetId is expected. This results in hard to diagnose bugs.
 *
 * We can avoid this issue by using a StrongId:
 *
 *      struct net_id_tag; //Phantom tag for NetId
 *      struct blk_id_tag; //Phantom tag for BlkId
 *
 *      typedef StrongId<net_id_tag> NetId;
 *      typedef StrongId<blk_id_tag> BlkId;
 *
 *      size_t count_net_teriminals(NetId net_id);
 *
 *      BlkId blk_id = 10;
 *      NetId net_id = 42;
 *
 *      count_net_teriminals(net_id); //OK
 *      count_net_teriminals(blk_id); //Compiler Error: NetId expected!
 *
 * StrongId is a template which implements the basic features of an Id, but disallows silent conversions
 * between different types of Ids. It uses another 'tag' type (passed as the first template parameter)
 * to uniquely identify the type of the Id (preventing conversions between different types of Ids).
 *
 * Usage
 * =====
 *
 * The StrongId template class takes one required and three optional template parameters:
 *
 *    1) Tag        - the unique type used to identify this type of Ids [Required]
 *    2) T          - the underlying integral id type (default: int) [Optional]
 *    3) T sentinel - a value representing an invalid Id (default: -1) [Optional]
 *
 * If no value is supllied during construction the StrongId is initialized to the invalid/sentinel value.
 *
 * Example 1: default definition
 *
 *      struct net_id_tag;
 *      typedef StrongId<net_id_tag> NetId; //Internally stores an integer Id, -1 represents invalid
 *
 * Example 2: definition with custom underlying type
 *
 *      struct blk_id_tag;
 *      typedef StrongId<net_id_tag,size_t> BlkId; //Internally stores a size_t Id, -1 represents invalid
 *
 * Example 3: definition with custom underlying type and custom sentinel value
 *
 *      struct pin_id_tag;
 *      typedef StrongId<net_id_tag,size_t,0> PinId; //Internally stores a size_t Id, 0 represents invalid
 *
 * Example 4: Creating Ids
 *
 *      struct net_id_tag;
 *      typedef StrongId<net_id_tag> MyId; //Internally stores an integer Id, -1 represents invalid
 *
 *      MyId my_id;           //Defaults to the sentinel value (-1 by default)
 *      MyId my_other_id = 5; //Explicit construction
 *      MyId my_thrid_id(25); //Explicit construction
 *
 * Example 5: Comparing Ids
 *
 *      struct net_id_tag;
 *      typedef StrongId<net_id_tag> MyId; //Internally stores an integer Id, -1 represents invalid
 *
 *      MyId my_id;           //Defaults to the sentinel value (-1 by default)
 *      MyId my_id_one = 1;
 *      MyId my_id_two = 2;
 *      MyId my_id_also_one = 1;
 *
 *      my_id_one == my_id_also_one; //True
 *      my_id_one == my_id; //False
 *      my_id_one == my_id_two; //False
 *      my_id_one != my_id_two; //True
 *
 * Example 5: Checking for invalid Ids
 *
 *      struct net_id_tag;
 *      typedef StrongId<net_id_tag> MyId; //Internally stores an integer Id, -1 represents invalid
 *
 *      MyId my_id;           //Defaults to the sentinel value
 *      MyId my_id_one = 1;
 *
 *      //Comparison against a constructed invalid id
 *      my_id == MyId::INVALID(); //True
 *      my_id_one == MyId::INVALID(); //False
 *      my_id_one != MyId::INVALID(); //True
 *
 *      //The Id can also be evaluated in a boolean context against the sentinel value
 *      if(my_id) //False, my_id is invalid
 *      if(!my_id) //True my_id is valid
 *      if(my_id_one) //True my_id_one is valid
 *
 * Example 6: Indexing data structures
 *
 *      struct my_id_tag;
 *      typedef StrongId<net_id_tag> MyId; //Internally stores an integer Id, -1 represents invalid
 *
 *      std::vector<int> my_vec = {0, 1, 2, 3, 4, 5};
 *
 *      MyId my_id = 2;
 *
 *      my_vec[size_t(my_id)]; //Access the third element via explicit conversion
 */
#include <type_traits> //for std::is_integral
#include <cstddef>     //for std::size_t
#include <functional>  //for std::hash

namespace vtr {

//Forward declare the class (needed for operator declarations)
template<typename tag, typename T, T sentinel>
class StrongId;

//Forward declare the equality/inequality operators
// We need to do this before the class definition so the class can
// friend them
template<typename tag, typename T, T sentinel>
bool operator==(const StrongId<tag, T, sentinel>& lhs, const StrongId<tag, T, sentinel>& rhs);

template<typename tag, typename T, T sentinel>
bool operator!=(const StrongId<tag, T, sentinel>& lhs, const StrongId<tag, T, sentinel>& rhs);

template<typename tag, typename T, T sentinel>
bool operator<(const StrongId<tag, T, sentinel>& lhs, const StrongId<tag, T, sentinel>& rhs);

//Class template definition with default template parameters
template<typename tag, typename T = int, T sentinel = T(-1)>
class StrongId {
    static_assert(std::is_integral<T>::value, "T must be integral");

  public:
    //Gets the invalid Id
    static constexpr StrongId INVALID() { return StrongId(); }

    //Default to the sentinel value
    constexpr StrongId()
        : id_(sentinel) {}

    //Only allow explict constructions from a raw Id (no automatic conversions)
    explicit constexpr StrongId(T id)
        : id_(id) {}

    //Allow some explicit conversion to useful types

    //Allow explicit conversion to bool (e.g. if(id))
    explicit operator bool() const { return *this != INVALID(); }

    //Allow explicit conversion to size_t (e.g. my_vector[size_t(strong_id)])
    explicit operator std::size_t() const { return static_cast<std::size_t>(id_); }

    //To enable hasing Ids
    friend std::hash<StrongId<tag, T, sentinel>>;

    //To enable comparisions between Ids
    // Note that since these are templated functions we provide an empty set of template parameters
    // after the function name (i.e. <>)
    friend bool operator== <>(const StrongId<tag, T, sentinel>& lhs, const StrongId<tag, T, sentinel>& rhs);
    friend bool operator!= <>(const StrongId<tag, T, sentinel>& lhs, const StrongId<tag, T, sentinel>& rhs);
    friend bool operator< <>(const StrongId<tag, T, sentinel>& lhs, const StrongId<tag, T, sentinel>& rhs);

  private:
    T id_;
};

template<typename tag, typename T, T sentinel>
bool operator==(const StrongId<tag, T, sentinel>& lhs, const StrongId<tag, T, sentinel>& rhs) {
    return lhs.id_ == rhs.id_;
}

template<typename tag, typename T, T sentinel>
bool operator!=(const StrongId<tag, T, sentinel>& lhs, const StrongId<tag, T, sentinel>& rhs) {
    return !(lhs == rhs);
}

//Needed for std::map-like containers
template<typename tag, typename T, T sentinel>
bool operator<(const StrongId<tag, T, sentinel>& lhs, const StrongId<tag, T, sentinel>& rhs) {
    return lhs.id_ < rhs.id_;
}

} //namespace vtr

//Specialize std::hash for StrongId's (needed for std::unordered_map-like containers)
namespace std {
template<typename tag, typename T, T sentinel>
struct hash<vtr::StrongId<tag, T, sentinel>> {
    std::size_t operator()(const vtr::StrongId<tag, T, sentinel> k) const noexcept {
        return std::hash<T>()(k.id_); //Hash with the underlying type
    }
};
} //namespace std

#endif
