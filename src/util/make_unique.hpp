#ifndef MAKE_UNIQUE_HPP
#define MAKE_UNIQUE_HPP

namespace fix {

//C++11 by oversight doesn't include a std::make_unique<>, this is fixed in C++14
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

} //fix

#endif
