#pragma once

#include <cmath>
#include <array>
#include <iosfwd>

#ifndef TIME_VEC_WIDTH
#define TIME_VEC_WIDTH 1
#endif

/*
 * What alignment is required?
 */
#if TIME_VEC_WIDTH > 8
//Required for aligned access with AVX
# define TIME_MEM_ALIGN 8*sizeof(float)

#elif TIME_VEC_WIDTH >= 4
//Required for aligned access with SSE
# define TIME_MEM_ALIGN 4*sizeof(float)

#endif //TIME_VEC_WIDTH

#if TIME_VEC_WIDTH > 1
# include <x86intrin.h>
#endif

namespace tatum {

class Time {
    public:
        typedef float scalar_type;
    public: //Constructors
        Time(): Time(NAN) {}

        ///Initialize from float types
        explicit Time(const double time) { set_value(time); }

    public: //Accessors
        ///The current time value
        scalar_type value() const;

        ///Indicates whether the current time value is valid
        bool valid() const;

        ///Updates the time value with the max of itself and other
        void max(const Time& other);
        ///Updates the time value with the min of itself and other
        void min(const Time& other);

        ///Allow conversions to scalar_type (usually float)
        operator scalar_type() const { return value(); }

    public: //Mutators
        ///Set the current time value to time
        void set_value(scalar_type time);


        Time& operator+=(const Time& rhs);
        Time& operator-=(const Time& rhs);

        friend bool operator==(const Time lhs, const Time rhs);
        friend bool operator<(const Time lhs, const Time rhs);
        friend bool operator>(const Time lhs, const Time rhs);
        friend Time operator-(const Time val);
        friend Time operator+(const Time val);

    private:
#if TIME_VEC_WIDTH > 1
        alignas(TIME_MEM_ALIGN) std::array<scalar_type, TIME_VEC_WIDTH> time_;
#else
        scalar_type time_;
#endif

};

} //namepsace

#include "Time.inl"
