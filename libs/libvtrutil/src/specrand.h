#ifndef VPR_SPEC_RAND_H
#define VPR_SPEC_RAND_H
/*
 * For inclusion in the SPEC cpu benchmarks
 * This file implements the random number generation necessary for the SPEC cpu benchmarks. The functions
 * defined here are used in vtr_random.h/cpp
 *
 *
 * A C-program for MT19937, with initialization improved 2002/1/26.
 * Coded by Takuji Nishimura and Makoto Matsumoto.
 *
 * Before using, initialize the state by using init_genrand(seed)  
 * or init_by_array(init_key, key_length).
 *
 * Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
 * All rights reserved.
 * Copyright (C) 2005, Mutsuo Saito
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3. The names of its contributors may not be used to endorse or promote 
 * products derived from this software without specific prior written 
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * Any feedback is very welcome.
 * http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
 * email: m-mat @ math.sci.hiroshima-u.ac.jp (remove space)
 */
/* Slightly modified for use in SPEC CPU by Cloyce D. Spradling (5 Nov 2009)
 */

#include "vtr_random.h"

class SpecRandomNumberGenerator : public vtr::RandomNumberGeneratorInterface {
  public:
    SpecRandomNumberGenerator(const SpecRandomNumberGenerator&) = delete;
    SpecRandomNumberGenerator& operator=(SpecRandomNumberGenerator& other) = delete;

    SpecRandomNumberGenerator();
    explicit SpecRandomNumberGenerator(int seed);

    virtual void srandom(int seed) override;
    virtual int irand(int imax) override;
    virtual float frand() override;

  private:
    /// @brief initializes mt[N] with a seed
    void spec_init_genrand_(unsigned long s);

    /**
     * @brief initialize by an array with array-length
     * @param init_key the array for initializing keys
     * @param key_length the length of array
     */
    void spec_init_by_array_(const unsigned long init_key[], size_t key_length);

    /// @brief generates a random number on [0,0xffffffff]-interval
    unsigned long spec_genrand_int32_();

    /// @brief Just a copy of spec_genrand_real2()
    double spec_rand_();
    /// @brief Just a copy of spec_genrand_int31()
    long spec_lrand48_();

    /// @brief generates a random number on [0,0x7fffffff]-interval */
    long spec_genrand_int31_();

    /// @brief generates a random number on [0,1]-real-interval
    double spec_genrand_real1_();

    /// @brief generates a random number on [0,1)-real-interval
    double spec_genrand_real2_();

    /// @brief generates a random number on (0,1)-real-interval
    double spec_genrand_real3_();

    /// @brief generates a random number on [0,1) with 53-bit resolution
    double spec_genrand_res53_();

  private:
    /// Period parameters
    static constexpr size_t M = 397;
    static constexpr size_t N = 624;
    /// constant vector a
    static constexpr unsigned long MATRIX_A = 0x9908b0dfUL;
    /// most significant w-r bits
    static constexpr unsigned long UPPER_MASK = 0x80000000UL;
    /// least significant r bits
    static constexpr unsigned long LOWER_MASK = 0x7fffffffUL;
    /// mti==N+1 means mt[N] is not initialized
    size_t mti = N + 1;
    /// the array for the state vector
    unsigned long mt[N];

};

#endif
