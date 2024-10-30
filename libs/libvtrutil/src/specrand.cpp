/*
 * * For inclusion in the SPEC cpu benchmarks
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
 * Copyright (C) 2005, Mutsuo Saito,
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

#include "specrand.h"

double SpecRandomNumberGenerator::spec_rand_() {
    return spec_genrand_int32_() * (1.0 / 4294967296.0);
}

long SpecRandomNumberGenerator::spec_lrand48_() {
    return (long)(spec_genrand_int32_() >> 1);
}

void SpecRandomNumberGenerator::spec_init_genrand_(unsigned long s) {
    mt[0] = s & 0xffffffffUL;
    for (mti = 1; mti < N; mti++) {
        mt[mti] = (1812433253UL * (mt[mti - 1] ^ (mt[mti - 1] >> 30)) + mti);
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array mt[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        mt[mti] &= 0xffffffffUL;
        /* for >32 bit machines */
    }
}

void SpecRandomNumberGenerator::spec_init_by_array_(const unsigned long init_key[], size_t key_length) {
    spec_init_genrand_(19650218UL);
    size_t i = 1;
    size_t j = 0;
    size_t k = (N > key_length ? N : key_length);
    for (; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 30)) * 1664525UL))
                + init_key[j] + j; /* non linear */
        mt[i] &= 0xffffffffUL;     /* for WORDSIZE > 32 machines */
        i++;
        j++;
        if (i >= N) {
            mt[0] = mt[N - 1];
            i = 1;
        }
        if (j >= key_length) j = 0;
    }
    for (k = N - 1; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 30)) * 1566083941UL))
                - i;           /* non linear */
        mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++;
        if (i >= N) {
            mt[0] = mt[N - 1];
            i = 1;
        }
    }

    mt[0] = 0x80000000UL; /* MSB is 1; assuring non-zero initial array */
}

/* generates a random number on [0,0xffffffff]-interval */
unsigned long SpecRandomNumberGenerator::spec_genrand_int32_() {
    unsigned long y;
    static unsigned long mag01[2] = {0x0UL, MATRIX_A};
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    if (mti >= N) { /* generate N words at one time */

        if (mti == N + 1)              /* if init_genrand() has not been called, */
            spec_init_genrand_(5489UL); /* a default initial seed is used */

        for (size_t kk = 0; kk < N - M; kk++) {
            y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
            mt[kk] = mt[kk + M] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        for (size_t kk; kk < N - 1; kk++) {
            y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
            mt[kk] = mt[kk + (M - N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        y = (mt[N - 1] & UPPER_MASK) | (mt[0] & LOWER_MASK);
        mt[N - 1] = mt[M - 1] ^ (y >> 1) ^ mag01[y & 0x1UL];

        mti = 0;
    }

    y = mt[mti++];

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return y;
}

long SpecRandomNumberGenerator::spec_genrand_int31_() {
    return (long)(spec_genrand_int32_() >> 1);
}

double SpecRandomNumberGenerator::spec_genrand_real1_() {
    return spec_genrand_int32_() * (1.0 / 4294967295.0);
    /* divided by 2^32-1 */
}

double SpecRandomNumberGenerator::spec_genrand_real2_() {
    return spec_genrand_int32_() * (1.0 / 4294967296.0);
    /* divided by 2^32 */
}

double SpecRandomNumberGenerator::spec_genrand_real3_() {
    return (((double)spec_genrand_int32_()) + 0.5) * (1.0 / 4294967296.0);
    /* divided by 2^32 */
}

double SpecRandomNumberGenerator::spec_genrand_res53_() {
    unsigned long a = spec_genrand_int32_() >> 5, b = spec_genrand_int32_() >> 6;
    return (a * 67108864.0 + b) * (1.0 / 9007199254740992.0);
}

void SpecRandomNumberGenerator::srandom(int seed) {
    spec_init_genrand_((unsigned long)seed);
}

int SpecRandomNumberGenerator::irand(int imax) {
    return (int)(spec_genrand_int31_() % (imax + 1));
}

float SpecRandomNumberGenerator::frand() {
    return (float)spec_genrand_real2_();
}

SpecRandomNumberGenerator::SpecRandomNumberGenerator(int seed) {
    spec_init_genrand_((unsigned long)seed);
}

SpecRandomNumberGenerator::SpecRandomNumberGenerator()
    : SpecRandomNumberGenerator(0) {}
