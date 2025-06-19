#ifndef _elim_hpp_INCLUDED
#define _elim_hpp_INCLUDED

#include "global.h"

#include "heap.hpp" // Alphabetically after 'elim.hpp'.

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

struct Internal;

struct elim_more {
  Internal *internal;
  elim_more (Internal *i) : internal (i) {}
  bool operator() (unsigned a, unsigned b);
};

typedef heap<elim_more> ElimSchedule;

struct proof_clause {
  int64_t id;
  vector<int> literals;
  // for lrat
  unsigned cid; // cadical_kitten id
  bool learned;
  vector<int64_t> chain;
};

enum GateType { NO = 0, EQUI = 1, AND = 2, ITE = 3, XOR = 4, DEF = 5 };

struct Eliminator {

  Internal *internal;
  ElimSchedule schedule;

  Eliminator (Internal *i)
      : internal (i), schedule (elim_more (i)), definition_unit (0),
        gatetype (NO) {}
  ~Eliminator ();

  queue<Clause *> backward;

  Clause *dequeue ();
  void enqueue (Clause *);

  vector<Clause *> gates;
  unsigned definition_unit;
  vector<proof_clause> proof_clauses;
  vector<int> marked;
  GateType gatetype;
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
