#pragma once

#include <cmath>
#include <array>
#include <iosfwd>
#include <x86intrin.h>

#ifndef TIME_VEC_WIDTH
#define TIME_VEC_WIDTH 1
#endif

//#define TIME_PAD_CACHE_LINE
#define CACHE_LINE_SIZE_BYTES 64 //x86 uses 64-byte cache-lines, check with 'getconf LEVEL1_DCACHE_LINESIZE' on linux commandline for other architectures

//#define TIME_VEC_WIDTH 32

//#define SIMD_INTRINSICS

/*
 * What alignment is required?
 */
#if TIME_VEC_WIDTH > 8
//Required for aligned access with AVX
#define TIME_MEM_ALIGN 8*sizeof(float) 

#elif TIME_VEC_WIDTH >= 4
//Required for aligned access with SSE
#define TIME_MEM_ALIGN 4*sizeof(float) 

#endif //TIME_VEC_WIDTH

/*
 * Do we want to use hand coded SIMD rather than
 * compiler inferred?
 */
#ifdef SIMD_INTRINSICS
#if TIME_VEC_WIDTH >= 8
#define USE_AVX
static_assert(TIME_VEC_WIDTH % 8 == 0, "TIME_VEC_WIDTH must be evenly divisible by 8 when using AVX!");

#elif TIME_VEC_WIDTH >= 4
#define USE_SSE
static_assert(TIME_VEC_WIDTH % 4 == 0, "TIME_VEC_WIDTH must be evenly divisible by 4 when using SSE!");

#elif TIME_VEC_WIDTH > 1
//static_assert(false, "Unsupported time vec width!");
#endif //TIME_VEC_WIDTH

#endif //SIMD_INTRINSICS

class Time {
    public:
        typedef float scalar_type;
        //Initialize from constant float types
        Time(): Time(NAN) {}
        explicit Time(const double time) { set_value(time); }

#if TIME_VEC_WIDTH > 1
#ifdef USE_AVX
     /*
      *AVX
      */
    void set_value(scalar_type time) {
        for(size_t i = 0; i < TIME_VEC_WIDTH/8; i++) {
            __m256 result = _mm256_set1_ps(time);
            _mm256_store_ps(&time_[i*8], result);
        }
    }


    void max(const Time& other) { 
        for(size_t i = 0; i < TIME_VEC_WIDTH/8; i++) {
            __m256 a = _mm256_load_ps(&time_[i*8]);
            __m256 b = _mm256_load_ps(&other.time_[i*8]);
            __m256 result = _mm256_max_ps(a, b);
            _mm256_store_ps(&time_[i*8], result);
        }
    }

    void min(const Time& other) { 
        for(size_t i = 0; i < TIME_VEC_WIDTH/8; i++) {
            __m256 a = _mm256_load_ps(&time_[i*8]);
            __m256 b = _mm256_load_ps(&other.time_[i*8]);
            __m256 result = _mm256_min_ps(a, b);
            _mm256_store_ps(&time_[i*8], result);
        }
    }

    Time& operator+=(const Time& rhs) {
        for(size_t i = 0; i < TIME_VEC_WIDTH/8; i++) {
            __m256 a = _mm256_load_ps(&time_[i*8]);
            __m256 b = _mm256_load_ps(&rhs.time_[i*8]);
            __m256 result = _mm256_add_ps(a, b);
            _mm256_store_ps(&time_[i*8], result);
        }

        return *this;
    }

    Time& operator-=(const Time& rhs) {
        for(size_t i = 0; i < TIME_VEC_WIDTH/8; i++) {
            __m256 a = _mm256_load_ps(&time_[i*8]);
            __m256 b = _mm256_load_ps(&rhs.time_[i*8]);
            __m256 result = _mm256_sub_ps(a, b);
            _mm256_store_ps(&time_[i*8], result);
        }

        return *this;
    }
#elif defined(USE_SSE) //USE_SSE
     /*
      *SSE
      */
    void set_value(scalar_type time) {
        for(size_t i = 0; i < TIME_VEC_WIDTH/4; i++) {
            __m128 result = _mm_set1_ps(time);
            _mm_store_ps(&time_[i*4], result);
        }
    }


    void max(const Time& other) { 
        for(size_t i = 0; i < TIME_VEC_WIDTH/4; i++) {
            __m128 a = _mm_load_ps(&time_[i*4]);
            __m128 b = _mm_load_ps(&other.time_[i*4]);
            __m128 result = _mm_max_ps(a, b);
            _mm_store_ps(&time_[i*4], result);
        }
    }

    void min(const Time& other) { 
        for(size_t i = 0; i < TIME_VEC_WIDTH/4; i++) {
            __m128 a = _mm_load_ps(&time_[i*4]);
            __m128 b = _mm_load_ps(&other.time_[i*4]);
            __m128 result = _mm_min_ps(a, b);
            _mm_store_ps(&time_[i*4], result);
        }
    }

    Time& operator+=(const Time& rhs) {
        for(size_t i = 0; i < TIME_VEC_WIDTH/4; i++) {
            __m128 a = _mm_load_ps(&time_[i*4]);
            __m128 b = _mm_load_ps(&rhs.time_[i*4]);
            __m128 result = _mm_add_ps(a, b);
            _mm_store_ps(&time_[i*4], result);
        }

        return *this;
    }

    Time& operator-=(const Time& rhs) {
        for(size_t i = 0; i < TIME_VEC_WIDTH/4; i++) {
            __m128 a = _mm_load_ps(&time_[i*4]);
            __m128 b = _mm_load_ps(&rhs.time_[i*4]);
            __m128 result = _mm_sub_ps(a, b);
            _mm_store_ps(&time_[i*4], result);
        }

        return *this;
    }

#else //i.e. not def SIMD_INTRINSICS
    /*
     * Serial / inferred SIMD
     */
    void set_value(scalar_type time) {
        for(size_t i = 0; i < time_.size(); i++) {
            time_[i] = time;
        }
    }

    void max(const Time& other)  { 
        for(size_t i = 0; i < time_.size(); i++) {
            //Use conditional so compiler will vectorize
            time_[i] = (time_[i] > other.time_[i]) ? time_[i] : other.time_[i];
        }
    }

    void min(const Time& other)  { 
        for(size_t i = 0; i < time_.size(); i++) {
            //Use conditional so compiler will vectorize
            time_[i] = (time_[i] < other.time_[i]) ? time_[i] : other.time_[i];
        }
    }

    Time& operator+=(const Time& rhs) {
        for(size_t i = 0; i < time_.size(); i++) {
            time_[i] += rhs.time_[i];
        }

        return *this;
    }

    Time& operator-=(const Time& rhs) {
        for(size_t i = 0; i < time_.size(); i++) {
            time_[i] -= rhs.time_[i];
        }

        return *this;
    }
#endif //SIMD_INTRINSICS
    scalar_type value() const { return time_[0]; }
    bool valid() const {
        bool result = true;
        for(size_t i = 0; i < time_.size(); i++) {
            result &= !isnan(time_[i]); 
        }
        return result;
    }
#else //Scalar case
    scalar_type value() const { return time_; }
    void set_value(scalar_type time) { time_ = time; }
    bool valid() const { return !isnan(time_); }

    void max(const Time& other) { time_ = std::max(time_, other.time_); }
    void min(const Time& other) { time_ = std::min(time_, other.time_); }
    Time& operator+=(const Time& rhs) { time_ += rhs.time_; return *this; }
    Time& operator-=(const Time& rhs) { time_ -= rhs.time_; return *this; }

#endif //TIME_VEC_WIDTH

        friend bool operator==(const Time& lhs, const Time& rhs);

    private:
#if TIME_VEC_WIDTH > 1
        //std::array<scalar_type, TIME_VEC_WIDTH> time_;
        //AVX requies 32 byte alignment
#ifdef TIME_MEM_ALIGN
        alignas(TIME_MEM_ALIGN) std::array<scalar_type, TIME_VEC_WIDTH> time_;
#else
        std::array<scalar_type, TIME_VEC_WIDTH> time_;
#endif
#else
        scalar_type time_;
#endif

};

#include "Time.inl"
