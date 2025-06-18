#ifndef _reap_h_INCLUDED
#define _reap_h_INCLUDED

#include "global.h"

#include <cstddef>
#include <vector>

ABC_NAMESPACE_CXX_HEADER_START

class Reap {
public:
  Reap ();
  void init ();
  void release ();
  inline bool empty () { return !num_elements; }

  inline size_t size () { return num_elements; }

  void push (unsigned);
  void clear ();
  unsigned pop ();

private:
  size_t num_elements;
  unsigned last_deleted;
  unsigned min_bucket;
  unsigned max_bucket;
  std::vector<unsigned> buckets[33];
};

ABC_NAMESPACE_CXX_HEADER_END

#endif
