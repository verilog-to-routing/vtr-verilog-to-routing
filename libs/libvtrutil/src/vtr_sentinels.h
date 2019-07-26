#ifndef VTR_SENTINELS_H
#define VTR_SENTINELS_H
namespace vtr {

//Some specialized containers like vtr::linear_map and
//vtr::vector_map require sentinel values to mark invalid/uninitialized
//values. By convention, such containers query the sentinel objects static
//INVALID() member function to retrieve the sentinel value.
//
//These classes allows users to specify a custom sentinel value.
//
//Usually the containers default to DefaultSentinel

//The sentinel value is the default constructed value of the type
template<class T>
class DefaultSentinel {
  public:
    constexpr static T INVALID() { return T(); }
};

//Specialization for pointer types
template<class T>
class DefaultSentinel<T*> {
  public:
    constexpr static T* INVALID() { return nullptr; }
};

//The sentile value is a specified value of the type
template<class T, T val>
class CustomSentinel {
  public:
    constexpr static T INVALID() { return T(val); }
};

//The common case where -1 is used as the sentinel value
template<class T>
using MinusOneSentinel = CustomSentinel<T, -1>;

} // namespace vtr
#endif
