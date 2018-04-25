#include <stdlib.h>

inline coeff_t fundamental(coeff_t z) {
    ASSERT(z > (coeff_t)0);
    while((z & (coeff_t)1) == 0)
      z = (z >> ((coeff_t)1));
    return z;
}

inline coeff_t cfabs(coeff_t x) {
    return x>=0 ? x : -x;
}
inline int min(int i1, int i2) { return (i1>i2) ? i2 : i1; }
inline int max(int i1, int i2) { return (i1>i2) ? i1 : i2; }
inline coeff_t cfmin(coeff_t c1, coeff_t c2) { return (c1>c2) ? c2 : c1; }
inline coeff_t cfmax(coeff_t c1, coeff_t c2) { return (c1>c2) ? c1 : c2; }

inline int compute_shr(coeff_t c) {
    int shr = 0;
    while((c&1) == 0) {	c = (c>>1); ++shr; }
    return shr;
}

inline int flog2(coeff_t c) { /* floor(log_2(c)) */
    ASSERT(c>0);
    int res = -1;
    while(c != 0) { c = (c>>((coeff_t)1)); ++res; }
    return res;
}

inline int clog2(coeff_t c) { /* ceil(log_2(c)) */
    int floor = flog2(c);
    return floor + ((c==(1<<floor)) ? 0 : 1);
}

inline coeff_t compute_scale(coeff_t c, int scale) {
    return (scale < 0) ? -(c<<(-scale-1)) : (c<<(scale-1));
}

inline int compose_scales(int s1, int s2) {
    if(s1 > 0 && s2 > 0) 	return s1 + s2 - 1;
    else if(s1 < 0 && s2 < 0)	return +(abs(s1) + abs(s2) - 1);
    else 	                return -(abs(s1) + abs(s2) - 1);
}

struct sp_t : pair<int,int> {
    sp_t() : pair<int,int>(0,0) {}
    sp_t(pair<int,int> p) : pair<int,int>(p) {  }
    sp_t(int scale1, int scale2) : pair<int,int>(scale1, scale2) {  }
    sp_t & normalize(coeff_t c1, coeff_t c2) {
	coeff_t fst = compute_scale(c1, first);
	coeff_t snd = compute_scale(c2, second);
	if((fst < 0 && -fst > snd) ||
	   (snd < 0 && -snd > fst) )  {
	    first = -first;
	    second = -second;
	}
	if(abs(first) > 1 && abs(second) > 1) {
	    int minsh = min(abs(first), abs(second)) - 1;
	    first = first > 0 ? (first - minsh) : (first + minsh);
	    second = second > 0 ? (second - minsh) : (second + minsh);
	}
	return *this;
    }
};
typedef const sp_t&            spref_t;

inline coeff_t compute_sp(coeff_t c1, coeff_t c2, spref_t sp) {
    /* shifts are offset by 1, see compute_all_sumdiffs in chains.cpp */
    c1 = (sp.first < 0) ? -(c1<<(-sp.first-1)) : (c1<<(sp.first-1));
    c2 = (sp.second < 0) ? -(c2<<(-sp.second-1)) : (c2<<(sp.second-1));
    return fundamental(cfabs(c1+c2));
}

int csd_bits(coeff_t c);
