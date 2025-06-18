#ifndef _factor_hpp_INCLUDED
#define _factor_hpp_INCLUDED

#include "global.h"

#include "clause.hpp"
#include "heap.hpp"

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

struct Internal;

struct factor_occs_size {
  Internal *internal;
  factor_occs_size (Internal *i) : internal (i) {}
  bool operator() (unsigned a, unsigned b);
};

struct Quotient {
  Quotient (int f) : factor (f) {}
  ~Quotient () {}
  int factor;
  size_t id;
  int64_t bid; // for LRAT
  Quotient *prev, *next;
  vector<Clause *> qlauses;
  vector<size_t> matches;
  size_t matched;
};

typedef heap<factor_occs_size> FactorSchedule;

struct Factoring {
  Factoring (Internal *, int64_t);
  ~Factoring ();

  // These are initialized by the constructor
  Internal *internal;
  int64_t limit;
  FactorSchedule schedule;

  int initial;
  int bound;
  vector<unsigned> count;
  vector<int> fresh;
  vector<int> counted;
  vector<int> nounted;
  vector<Clause *> flauses;
  struct {
    Quotient *first, *last;
  } quotients;
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
