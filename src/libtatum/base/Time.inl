#include <iostream>
#include <string>

#include "Time.hpp"

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
            result &= !isnan(time_[i]);
        }
        return result;
    }
#else //Scalar case (TIME_VEC_WIDTH == 1)
    inline Time::scalar_type Time::value() const { return time_; }
    inline void Time::set_value(scalar_type time) { time_ = time; }
    inline bool Time::valid() const { return !isnan(time_); }

    inline void Time::max(const Time& other) { time_ = std::max(time_, other.time_); }
    inline void Time::min(const Time& other) { time_ = std::min(time_, other.time_); }
    inline Time& Time::operator+=(const Time& rhs) { time_ += rhs.time_; return *this; }
    inline Time& Time::operator-=(const Time& rhs) { time_ -= rhs.time_; return *this; }

#endif //TIME_VEC_WIDTH

/*
 * External functions
 */

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

