#ifndef _util_hpp_INCLUDED
#define _util_hpp_INCLUDED

#include "global.h"

#include <cassert>
#include <cstdint>
#include <vector>

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

using namespace std;

// Common simple utility functions independent from 'Internal'.

/*------------------------------------------------------------------------*/

inline double relative (double a, double b) { return b ? a / b : 0; }
inline double percent (double a, double b) { return relative (100 * a, b); }
inline int sign (int lit) { return (lit > 0) - (lit < 0); }
inline unsigned bign (int lit) { return 1 + (lit < 0); }

/*------------------------------------------------------------------------*/

bool has_suffix (const char *str, const char *suffix);
bool has_prefix (const char *str, const char *prefix);

/*------------------------------------------------------------------------*/

// Parse integer string in the form of
//
//   true
//   false
//   [-]<mantissa>[e<exponent>]
//
// and in the latter case '<val>' has to be within [-INT_MAX,INT_MAX].
//
// The function returns true if parsing is successful and then also sets
// the second argument to the parsed value.

bool parse_int_str (const char *str, int &);

/*------------------------------------------------------------------------*/

inline bool is_power_of_two (unsigned n) { return n && !(n & (n - 1)); }

inline bool contained (int64_t c, int64_t l, int64_t u) {
  return l <= c && c <= u;
}

inline bool aligned (size_t n, size_t alignment) {
  CADICAL_assert (is_power_of_two (alignment));
  const size_t mask = alignment - 1;
  return !(n & mask);
}

inline size_t align (size_t n, size_t alignment) {
  size_t res = n;
  CADICAL_assert (is_power_of_two (alignment));
  const size_t mask = alignment - 1;
  if (res & mask)
    res = (res | mask) + 1;
  CADICAL_assert (aligned (res, alignment));
  return res;
}

/*------------------------------------------------------------------------*/

inline bool parity (unsigned a) {
  CADICAL_assert (sizeof a == 4);
  unsigned tmp = a;
  tmp ^= (tmp >> 16);
  tmp ^= (tmp >> 8);
  tmp ^= (tmp >> 4);
  tmp ^= (tmp >> 2);
  tmp ^= (tmp >> 1);
  return tmp & 1;
}

/*------------------------------------------------------------------------*/

// The standard 'Effective STL' way (though not guaranteed) to clear a
// vector and reduce its capacity to zero, thus deallocating all its
// internal memory.  This is quite important for keeping the actual
// allocated size of watched and occurrence lists small particularly during
// bounded variable elimination where many clauses are added and removed.

template <class T> void erase_vector (std::vector<T> &v) {
  if (v.capacity ()) {
    std::vector<T> ().swap (v);
  }
  CADICAL_assert (!v.capacity ()); // not guaranteed though
}

// The standard 'Effective STL' way (though not guaranteed) to shrink the
// capacity of a vector to its size thus kind of releasing all the internal
// excess memory not needed at the moment any more.

template <class T> void shrink_vector (std::vector<T> &v) {
  if (v.capacity () > v.size ()) {
    std::vector<T> (v).swap (v);
  }
  CADICAL_assert (v.capacity () == v.size ()); // not guaranteed though
}

template <class T>
static void enlarge_init (vector<T> &v, size_t N, const T &i) {
  if (v.size () < N)
    v.resize (N, i);
}

template <class T> static void enlarge_only (vector<T> &v, size_t N) {
  if (v.size () < N)
    v.resize (N, T ());
}

template <class T> static void enlarge_zero (vector<T> &v, size_t N) {
  enlarge_init (v, N, (const T &) 0);
}

// Clean-up class for bad_alloc error safety.

template <typename T> struct DeferDeleteArray {
  T *data;
  DeferDeleteArray (T *t) : data (t) {}
  ~DeferDeleteArray () { delete[] data; }
  void release () { data = nullptr; }
  void free () {
    delete[] data;
    data = nullptr;
  }
};

template <typename T> struct DeferDeletePtr {
  T *data;
  DeferDeletePtr (T *t) : data (t) {}
  ~DeferDeletePtr () { delete data; }
  void release () { data = nullptr; }
  void free () {
    delete data;
    data = nullptr;
  }
};

/*------------------------------------------------------------------------*/

template <class T> inline void clear_n (T *base, size_t n) {
  memset (base, 0, sizeof (T) * n);
}

/*------------------------------------------------------------------------*/

// These are options both to 'cadical' and 'mobical'.  After wasting some
// on not remembering the spelling (British vs American), nor singular vs
// plural and then wanted to use '--color=false', and '--colours=0' too, I
// just factored this out into these two utility functions.

bool is_color_option (const char *arg);    // --color, --colour, ...
bool is_no_color_option (const char *arg); // --no-color, --no-colour, ...

/*------------------------------------------------------------------------*/

uint64_t hash_string (const char *str);

/*------------------------------------------------------------------------*/

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
