#pragma once

#include <cmath>
#include <iosfwd>

class Time {
    public:
        //Initialize from constant float types
        Time(): time_(NAN) {}
        explicit Time(const double time): time_(time) {}

        float value() const {return time_;}
        void set_value(float time) {time_ = time;}

        void max(const Time& other) { time_ = std::max(time_, other.time_); }
        void min(const Time& other) { time_ = std::min(time_, other.time_); }

        bool valid() const { return !isnan(time_); }

        Time& operator+=(const Time& rhs) {
            time_ += rhs.time_;
            return *this;
        }

        Time& operator-=(const Time& rhs) {
            time_ -= rhs.time_;
            return *this;
        }
    private:
        float time_;
};

inline Time operator+(Time lhs, const Time& rhs) {
    return lhs += rhs;
}

inline Time operator-(Time lhs, const Time& rhs) {
    return lhs -= rhs;
}

std::ostream& operator<<(std::ostream& os, const Time& time);

