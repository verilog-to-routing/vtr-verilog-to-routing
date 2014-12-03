#pragma once

#include <cmath>
#include <array>
#include <iosfwd>

//#define TIME_PAD_CACHE_LINE
#define CACHE_LINE_SIZE_BYTES 64 //x86 uses 64-byte cache-lines, check with 'getconf LEVEL1_DCACHE_LINESIZE' on linux commandline for other architectures

//#define TIME_VEC_WIDTH 32

class Time {
    public:
        //Initialize from constant float types
#if TIME_VEC_WIDTH > 1
        Time() {
            for(size_t i = 0; i < time_.size(); i++) {
                time_[i] = NAN;
            }
        }

        explicit Time(const double time) {
            for(size_t i = 0; i < time_.size(); i++) {
                time_[i] = time;
            }
        }
#else
        //Use initializer lists for the single time case
        Time(): time_(NAN) { }

        explicit Time(const double time): time_(time) { }
#endif

        float value() const {
#if TIME_VEC_WIDTH > 1
            return time_[0];
#else
            return time_;
#endif
        }
        void set_value(float time) {
#if TIME_VEC_WIDTH > 1
            for(size_t i = 0; i < time_.size(); i++) {
                time_[i] = time;
            }
#else
            time_ = time;
#endif
        }

        void max(const Time& other) { 
#if TIME_VEC_WIDTH > 1
            for(size_t i = 0; i < time_.size(); i++) {
                //Use conditional so compiler will vectorize
                time_[i] = (time_[i] > other.time_[i]) ? time_[i] : other.time_[i];
            }
#else
            time_ = std::max(time_, other.time_);
#endif
        }

        void min(const Time& other) { 
#if TIME_VEC_WIDTH > 1
            for(size_t i = 0; i < time_.size(); i++) {
                //Use conditional so compiler will vectorize
                time_[i] = (time_[i] < other.time_[i]) ? time_[i] : other.time_[i];
            }
#else
            time_ = std::min(time_, other.time_);
#endif
        }

        bool valid() const {
#if TIME_VEC_WIDTH > 1
            bool is_valid = true;
            for(size_t i = 0; i < time_.size(); i++) {
                is_valid &= !isnan(time_[i]);
            }
            return is_valid; 
#else
            return !isnan(time_);
#endif
        }

        Time& operator+=(const Time& rhs) {
#if TIME_VEC_WIDTH > 1
            for(size_t i = 0; i < time_.size(); i++) {
                time_[i] += rhs.time_[i];
            }
#else
                time_ += rhs.time_;
#endif
            return *this;
        }

        Time& operator-=(const Time& rhs) {
#if TIME_VEC_WIDTH > 1
            for(size_t i = 0; i < time_.size(); i++) {
                time_[i] -= rhs.time_[i];
            }
#else
            time_ -= rhs.time_;
#endif
            return *this;
        }

        friend bool operator==(const Time& lhs, const Time& rhs);
    private:
#if TIME_VEC_WIDTH > 1
        std::array<float, TIME_VEC_WIDTH> time_;
#else
        float time_;
#endif

#ifdef TIME_PAD_CACHE_LINE
        //Pad out to the cache line size
        std::array<float, CACHE_LINE_SIZE_BYTES/sizeof(float) - (sizeof(time_)) > pad;
#endif
};

inline bool operator==(const Time& lhs, const Time& rhs) {
#if TIME_VEC_WIDTH > 1
    bool is_equal = true;
    for(size_t i = 0; i < lhs.time_.size(); i++) {
        is_equal &= (lhs.time_[i] == rhs.time_[i]);
    }
    return is_equal;
#else
    return (lhs.time_ == rhs.time_);
#endif
}

inline Time operator+(Time lhs, const Time& rhs) {
    return lhs += rhs;
}

inline Time operator-(Time lhs, const Time& rhs) {
    return lhs -= rhs;
}

std::ostream& operator<<(std::ostream& os, const Time& time);

