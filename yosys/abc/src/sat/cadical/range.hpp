#ifndef _range_hpp_INCLUDED
#define _range_hpp_INCLUDED

#include "global.h"

#include <cassert>

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

struct Clause;

/*----------------------------------------------------------------------*/

// Used for compact and safe iteration over positive ranges of integers,
// particularly for iterating over all variable indices.
//
//   Range vars (max_var);
//   for (auto idx : vars) ...
//
// This iterates over '1, ..., max_var' and is safe for non-negative
// numbers, thus also for 'max_var == 0' or 'max_var == INT_MAX'.
//
// Note that
//
//   for (int idx = 1; idx <= max_var; idx++) ...
//
// leads to an overflow if 'max_var == INT_MAX' and thus depending on what
// the compiler does ('int' overflow is undefined) might lead to any
// behaviour (infinite loop or worse array access way out of bounds).
//
// If we make 'idx' in this last 'for' loop an 'unsigned' then it is safe to
// use this idiom, but we would need to cast 'max_var' explicitly to 'int'
// in order to avoid a warning in the loop condition and actually everywhere
// where 'idx' is compared to a 'signed' expression.  Worse for instance
// 'vals[-idx]' will lead to out of bounds access too.  This is awkward and
// using the range iterator provided here is safer in general.
//
// Another issue is that the dereferencing operator '*' below is required to
// return a reference to the internal index of the iterator.  Thus the 'idx'
// in the auto loop is actually of the same type as the internal state of
// the iterator.  To keep it 'signed' and still avoid overflow issues we
// just have to make sure to use the proper increment (with two implicit
// casts, i.e., from 'int' to 'unsigned', then 'unsigned' addition and the
// result is cast back from 'unsigned' to 'int').
//
// For simplicity we keep a reference to the actual maximum integer, e.g.,
// 'max_var', which makes the idiom 'for (auto idx : vars) ...' possible.
// Further note that the referenced integer has to be non-negative before
// starting to iterate (it can be zero though), otherwise it breaks.

class Range {
  static unsigned inc (unsigned u) { return u + 1u; }
  class iterator {
    int idx;

  public:
    iterator (int i) : idx (i) {}
    void operator++ () { idx = inc (idx); }
    const int &operator* () const { return idx; }
    friend bool operator!= (const iterator &a, const iterator &b) {
      return a.idx != b.idx;
    }
  };
  int &n;

public:
  iterator begin () const { return CADICAL_assert (n >= 0), iterator (inc (0)); }
  iterator end () const { return CADICAL_assert (n >= 0), iterator (inc (n)); }
  Range (int &m) : n (m) { CADICAL_assert (m >= 0); }
};

// Same, but iterating over literals '-1,1,-2,2,....,-max_var,max_var'.
//
// The only difference to 'Range' is the 'inc' function, but I am too lazy
// to figure out how to properly factor the code into a generic range
// template with 'inc' as only parameter.  This gives at least clean code.

class Sange {
  static unsigned inc (unsigned u) { return ~u + (u >> 31); }
  class iterator {
    int lit;

  public:
    iterator (int i) : lit (i) {}
    void operator++ () { lit = inc (lit); }
    const int &operator* () const { return lit; }
    friend bool operator!= (const iterator &a, const iterator &b) {
      return a.lit != b.lit;
    }
  };
  int &n;

public:
  iterator begin () const { return CADICAL_assert (n >= 0), iterator (inc (0)); }
  iterator end () const { return CADICAL_assert (n >= 0), iterator (inc (n)); }
  Sange (int &m) : n (m) { CADICAL_assert (m >= 0); }
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
