#ifndef _radix_hpp_INCLUDED
#define _radix_hpp_INCLUDED

#include "global.h"

#include <cassert>
#include <cstring>
#include <iterator>
#include <vector>

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

using namespace std;

// This provides an implementation of a generic radix sort algorithm. The
// reason for having it is that for certain benchmarks and certain parts of
// CaDiCaL where sorting is used, the standard sorting algorithm 'sort'
// turned out to be a hot-spot. Up to 30% of the total running time was for
// instance used for some benchmarks in sorting variables during bumping
// to make sure to bump them in 'enqueued' order.
//
// Further, in most cases, where we need to sort something, sorting is
// actually performed on positive numbers (such as the 'enqueued' time stamp
// during bumping), which allows to use radix sort or variants. At least
// starting with medium sized arrays to be sorted (say above 1000 elements,
// but see discussion on 'MSORT' below), radix sort can be way faster.
//
// Finally it is stable, which is actually preferred most of the time too.
//
// This template algorithm 'rsort' takes as first template parameter the
// iterator class similar to the standard 'sort' algorithm template, but
// then as second parameter a function class (similar to the second 'less
// than' parameter of 'sort') which can obtain a 'rank' from each element,
// on which they are compared.  The 'rank' should be able to turn an element
// into a number.  The type of these ranks is determined automatically but
// should be 'unsigned'.

struct pointer_rank {
  typedef size_t Type;
  Type operator() (void *ptr) { return (size_t) ptr; }
};

template <class I, class Rank> void rsort (I first, I last, Rank rank) {
  typedef typename iterator_traits<I>::value_type T;
  typedef typename Rank::Type R;

  CADICAL_assert (first <= last);
  const size_t n = last - first;
  if (n <= 1)
    return;

  const size_t l = 8;        // Radix 8, thus byte-wise.
  const size_t w = (1 << l); // So many buckets.

  const unsigned mask = w - 1; // Fast mod 'w'.

// Uncomment the following define for large values of 'w' in order to keep
// the large bucket array 'count' on the heap instead of the stack.
//
// #define CADICAL_RADIX_BUCKETS_ON_THE_HEAP
//
#ifdef CADICAL_RADIX_BUCKETS_ON_THE_HEAP
  size_t *count = new size_t[w]; // Put buckets on the heap.
#else
  size_t count[w]; // Put buckets on the stack.
#endif

  I a = first, b = last, c = a;
  bool initialized = false;
  std::vector<T> v;

  R upper = 0, lower = ~upper;
  R shifted = mask;
  bool bounded = false;

  R masked_lower = 0, masked_upper = mask;

  for (size_t i = 0; i < 8 * sizeof (rank (*first));
       i += l, shifted <<= l) {

    if (bounded && (lower & shifted) == (upper & shifted))
      continue;

    memset (count + masked_lower, 0,
            (masked_upper - masked_lower + 1) * sizeof *count);

    const I end = c + n;
    bool sorted = true;
    R last = 0;

    for (I p = c; p != end; p++) {
      const auto r = rank (*p);
      if (!bounded)
        lower &= r, upper |= r;
      const auto s = r >> i;
      const auto m = s & mask;
      if (sorted && last > m)
        sorted = false;
      else
        last = m;
      count[m]++;
    }

    masked_lower = (lower >> i) & mask;
    masked_upper = (upper >> i) & mask;

    if (!bounded) {
      bounded = true;
      if ((lower & shifted) == (upper & shifted))
        continue;
    }

    if (sorted)
      continue;

    size_t pos = 0;
    for (R j = masked_lower; j <= masked_upper; j++) {
      const size_t delta = count[j];
      count[j] = pos;
      pos += delta;
    }

    if (!initialized) {
      CADICAL_assert (&*c == &*a); // MS VC++
      v.resize (n);
      b = v.begin ();
      initialized = true;
    }

    I d = (&*c == &*a) ? b : a; // MS VC++

    for (I p = c; p != end; p++) {
      const auto r = rank (*p);
      const auto s = r >> i;
      const auto m = s & mask;
      d[count[m]++] = *p;
    }
    c = d;
  }

  if (c == b) {
    for (size_t i = 0; i < n; i++)
      a[i] = b[i];
  }

#ifdef CADICAL_RADIX_BUCKETS_ON_THE_HEAP
  delete[] count;
#endif

#ifndef CADICAL_NDEBUG
  for (I p = first; p + 1 != last; p++)
    CADICAL_assert (rank (p[0]) <= rank (p[1]));
#endif
}

// It turns out that for small number of elements (like '100') and in
// particular for large value ranges the standard sorting function is
// considerably faster than our radix sort (like 2.5x). This negative effect
// vanishes at around 800 elements (sorting integers) and thus we provide a
// function 'MSORT' which selects between standard sort and radix sort based
// on the number of elements.  However we failed to put this into proper C++
// style template code and thus have to use a macro instead.  We also do not
// use it everywhere instead of 'rsort' since it requires a fourth
// parameter, which is awkward, particular in those situation where we
// expect large arrays to be sorted anyhow (such as during sorting the
// clauses in an arena or the probes during probing).  The first argument
// is the limit up to which we use the standard sort.  Above the limit we
// use radix sort.  As usual we do not want to hard code it here (default
// is '800') in order to make fuzzing and delta debugging more effective.

#define MSORT(LIMIT, FIRST, LAST, RANK, LESS) \
  do { \
    const size_t N = LAST - FIRST; \
    if (N <= (size_t) (LIMIT)) \
      sort (FIRST, LAST, LESS); \
    else \
      rsort (FIRST, LAST, RANK); \
  } while (0)

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
