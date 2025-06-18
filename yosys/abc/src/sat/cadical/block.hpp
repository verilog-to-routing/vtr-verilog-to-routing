#ifndef _block_hpp_INCLUDED
#define _block_hpp_INCLUDED

#include "global.h"

#include "heap.hpp" // Alphabetically after 'block.hpp'.

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

struct Internal;

struct block_more_occs_size {
  Internal *internal;
  block_more_occs_size (Internal *i) : internal (i) {}
  bool operator() (unsigned a, unsigned b);
};

typedef heap<block_more_occs_size> BlockSchedule;

class Blocker {

  friend struct Internal;

  vector<struct Clause *> candidates;
  vector<struct Clause *> reschedule;
  BlockSchedule schedule;

  Blocker (Internal *i) : schedule (block_more_occs_size (i)) {}

  void erase () {
    erase_vector (candidates);
    erase_vector (reschedule);
    schedule.erase ();
  }
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
