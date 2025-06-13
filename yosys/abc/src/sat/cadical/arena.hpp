#ifndef _arena_hpp_INCLUDED
#define _arena_hpp_INCLUDED

#include "global.h"

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

// This memory allocation arena provides fixed size pre-allocated memory for
// the moving garbage collector 'copy_non_garbage_clauses' in 'collect.cpp'
// to hold clauses which should survive garbage collection.

// The advantage of using a pre-allocated arena is that the allocation order
// of the clauses can be adapted in such a way that clauses watched by the
// same literal are allocated consecutively.  This improves locality during
// propagation and thus is more cache friendly. A similar technique is
// implemented in MiniSAT and Glucose and gives substantial speed-up in
// propagations per second even though it might even almost double peek
// memory usage.  Note that in MiniSAT this arena is actually required for
// MiniSAT to be able to use 32 bit clauses references instead of 64 bit
// pointers.  This would restrict the maximum number of clauses and thus is
// a restriction we do not want to use anymore.

// New learned clauses are allocated in CaDiCaL outside of this arena and
// moved to the arena during garbage collection.  The additional 'to' space
// required for such a moving garbage collector is only allocated for those
// clauses surviving garbage collection, which usually needs much less
// memory than all clauses.  The net effect is that in our implementation
// the moving garbage collector using this arena only needs roughly 50% more
// memory than allocating the clauses directly.  Both implementations can be
// compared by varying the 'opts.arenatype' option (which also controls the
// allocation order of clauses during moving them).

// The standard sequence of using the arena is as follows:
//
//   Arena arena;
//   ...
//   arena.prepare (bytes);
//   q1 = arena.copy (p1, bytes1);
//   ...
//   qn = arena.copy (pn, bytesn);
//   CADICAL_assert (bytes1 + ... + bytesn <= bytes);
//   arena.swap ();
//   ...
//   if (!arena.contains (q)) delete q;
//   ...
//   arena.prepare (bytes);
//   q1 = arena.copy (p1, bytes1);
//   ...
//   qn = arena.copy (pn, bytesn);
//   CADICAL_assert (bytes1 + ... + bytesn <= bytes);
//   arena.swap ();
//   ...
//
// One has to be really careful with 'qi' references to arena memory.

struct Internal;

class Arena {

  Internal *internal;

  struct {
    char *start, *top, *end;
  } from, to;

public:
  Arena (Internal *);
  ~Arena ();

  // Prepare 'to' space to hold that amount of memory.  Precondition is that
  // the 'to' space is empty.  The following sequence of 'copy' operations
  // can use as much memory in sum as pre-allocated here.
  //
  void prepare (size_t bytes);

  // Does the memory pointed to by 'p' belong to this arena? More precisely
  // to the 'from' space, since that is the only one remaining after 'swap'.
  //
  bool contains (void *p) const {
    char *c = (char *) p;
    return (from.start <= c && c < from.top) ||
           (to.start <= c && c < to.top);
  }

  // Allocate that amount of memory in 'to' space.  This assumes the 'to'
  // space has been prepared to hold enough memory with 'prepare'.  Then
  // copy the memory pointed to by 'p' of size 'bytes'.  Note that it does
  // not matter whether 'p' is in 'from' or allocated outside of the arena.
  //
  char *copy (const char *p, size_t bytes) {
    char *res = to.top;
    to.top += bytes;
    CADICAL_assert (to.top <= to.end);
    memcpy (res, p, bytes);
    return res;
  }

  // Completely delete 'from' space and then replace 'from' by 'to' (by
  // pointer swapping).  Everything previously allocated (in 'from') and not
  // explicitly copied to 'to' with 'copy' becomes invalid.
  //
  void swap ();
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
