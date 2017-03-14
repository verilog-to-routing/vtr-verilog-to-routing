#include <iostream>
#include <string>

#include "Time.hpp"

namespace tatum {

/*
 * Class members
 */

#if TIME_VEC_WIDTH > 1
    /*
     * Serial / inferred SIMD
     */
    inline void Time::set_value(scalar_type time) {
        for(size_t i = 0; i < time_.size(); i++) {
            time_[i] = time;
        }
    }

    inline void Time::max(const Time& other)  {
        for(size_t i = 0; i < time_.size(); i++) {
            //Use conditional so compiler will vectorize
            time_[i] = (time_[i] > other.time_[i]) ? time_[i] : other.time_[i];
        }
    }

    inline void Time::min(const Time& other)  {
        for(size_t i = 0; i < time_.size(); i++) {
            //Use conditional so compiler will vectorize
            time_[i] = (time_[i] < other.time_[i]) ? time_[i] : other.time_[i];
        }
    }

    inline Time& Time::operator+=(const Time& rhs) {
        for(size_t i = 0; i < time_.size(); i++) {
            time_[i] += rhs.time_[i];
        }

        return *this;
    }

    inline Time& Time::operator-=(const Time& rhs) {
        for(size_t i = 0; i < time_.size(); i++) {
            time_[i] -= rhs.time_[i];
        }

        return *this;
    }

    inline Time::scalar_type Time::value() const { return time_[0]; }

    inline bool Time::valid() const {
        //This is a reduction with a function call inside,
        //so we can't vectorize easily
        bool result = true;
        for(size_t i = 0; i < time_.size(); i++) {
            result &= !std::isnan(time_[i]);
        }
        return result;
    }
#else //Scalar case (TIME_VEC_WIDTH == 1)
    inline Time::scalar_type Time::value() const { return time_; }
    inline void Time::set_value(scalar_type time) { time_ = time; }
    inline bool Time::valid() const { return !std::isnan(time_); }

    inline void Time::max(const Time& other) { time_ = std::max(time_, other.time_); }
    inline void Time::min(const Time& other) { time_ = std::min(time_, other.time_); }
    inline Time& Time::operator+=(const Time& rhs) { time_ += rhs.time_; return *this; }
    inline Time& Time::operator-=(const Time& rhs) { time_ -= rhs.time_; return *this; }

#endif //TIME_VEC_WIDTH

/*
 * External functions
 */

#if TIME_VEC_WIDTH > 1
inline Time operator-(Time in) {
    for(size_t i = 0; i < time_.size(); i++) {
        in.time_[i] = -in.time_[i];
    }
    return in;
}
inline Time operator+(Time in) {
    for(size_t i = 0; i < time_.size(); i++) {
        in.time_[i] = +in.time_[i];
    }
    return in;
}
#else //Scalar case (TIME_VEC_WIDTH == 1)
inline bool operator==(const Time lhs, const Time rhs) {
    return lhs.time_ == rhs.time_;
}

inline bool operator<(const Time lhs, const Time rhs) {
    return lhs.time_ < rhs.time_;
}

inline bool operator>(const Time lhs, const Time rhs) {
    return lhs.time_ > rhs.time_;
}

inline Time operator-(Time in) {
    in.time_ = -in.time_;
    return in;
}
inline Time operator+(Time in) {
    in.time_ = +in.time_;
    return in;
}
#endif //TIME_VEC_WIDTH

inline Time operator+(Time lhs, const Time& rhs) {
    return lhs += rhs;
}

inline Time operator-(Time lhs, const Time& rhs) {
    return lhs -= rhs;
}

inline std::ostream& operator<<(std::ostream& os, const Time& time) {
    os << time.value();
    return os;
}

} //namepsace
