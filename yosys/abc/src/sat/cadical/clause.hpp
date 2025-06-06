#ifndef _clause_hpp_INCLUDED
#define _clause_hpp_INCLUDED

#include "global.h"

#include "util.hpp"
#include <climits>
#include <cstdint>
#include <cstdlib>

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

typedef int *literal_iterator;
typedef const int *const_literal_iterator;

/*------------------------------------------------------------------------*/

// The 'Clause' data structure is very important. There are usually many
// clauses and accessing them is a hot-spot.  Thus we use common
// optimizations to reduce memory and improve cache usage, even though this
// induces some complexity in understanding the code.
//
// The most important optimization is to 'embed' the actual literals in the
// clause.  This requires a variadic size structure and thus strictly is not
// 'C' conform, but supported by all compilers we used.  The alternative is
// to store the actual literals somewhere else, which not only needs more
// memory but more importantly also requires another memory access and thus
// is very costly.

struct Clause {
  union {
    int64_t id;   // Used to create LRAT-style proofs
    Clause *copy; // Only valid if 'moved', then that's where to.
    //
    // The 'copy' field is only valid for 'moved' clauses in the moving
    // garbage collector 'copy_non_garbage_clauses' for keeping clauses
    // compactly in a contiguous memory arena.  Otherwise, so almost all of
    // the time, 'id' is valid.  See 'collect.cpp' for details.
  };
  bool conditioned : 1; // Tried for globally blocked clause elimination.
  bool covered : 1;  // Already considered for covered clause elimination.
  bool enqueued : 1; // Enqueued on backward queue.
  bool frozen : 1;   // Temporarily frozen (in covered clause elimination).
  bool garbage : 1;  // can be garbage collected unless it is a 'reason'
  bool gate : 1;     // Clause part of a gate (function definition).
  bool hyper : 1;    // redundant hyper binary or ternary resolved
  bool instantiated : 1; // tried to instantiate
  bool moved : 1;        // moved during garbage collector ('copy' valid)
  bool reason : 1;       // reason / antecedent clause can not be collected
  bool redundant : 1;    // aka 'learned' so not 'irredundant' (original)
  bool transred : 1;     // already checked for transitive reduction
  bool subsume : 1;      // not checked in last subsumption round
  bool swept : 1;        // clause used to sweep equivalences
  bool flushed : 1;      // garbage in proof deleted binaries
  unsigned used : 8; // resolved in conflict analysis since last 'reduce'
  bool vivified : 1; // clause already vivified
  bool vivify : 1;   // clause scheduled to be vivified

  // The glucose level ('LBD' or short 'glue') is a heuristic value for the
  // expected usefulness of a learned clause, where smaller glue is consider
  // more useful.  During learning the 'glue' is determined as the number of
  // decisions in the learned clause.  Thus the glue of a clause is a strict
  // upper limit on the smallest number of decisions needed to make it
  // propagate.  For instance a binary clause will propagate if one of its
  // literals is set to false.  Similarly a learned clause with glue 1 can
  // propagate after one decision, one with glue 2 after 2 decisions etc.
  // In some sense the glue is an abstraction of the size of the clause.
  //
  // See the IJCAI'09 paper by Audemard & Simon for more details.  We
  // switched back and forth between keeping the glue stored in a clause and
  // using it only initially to determine whether it is kept, that is
  // survives clause reduction.  The latter strategy is not bad but also
  // does not allow to use glue values for instance in 'reduce'.
  //
  // More recently we also update the glue and promote clauses to lower
  // level tiers during conflict analysis.  The idea of using three tiers is
  // also due to Chanseok Oh and thus used in all recent 'Maple...' solvers.
  // Tier one are the always kept clauses with low glue at most
  // 'opts.reducetier1glue' (default '2'). The second tier contains all
  // clauses with glue larger than 'opts.reducetier1glue' but smaller or
  // equal than 'opts.reducetier2glue' (default '6').  The third tier
  // consists of clauses with glue larger than 'opts.reducetier2glue'.
  //
  // Clauses in tier one are not deleted in 'reduce'. Clauses in tier
  // two require to be unused in two consecutive 'reduce' intervals before
  // being collected while for clauses in tier three not being used since
  // the last 'reduce' call makes them deletion candidates.  Clauses derived
  // by hyper binary or ternary resolution (even though small and thus with
  // low glue) are always removed if they remain unused during one interval.
  // See 'mark_useless_redundant_clauses_as_garbage' in 'reduce.cpp' and
  // 'bump_clause' in 'analyze.cpp'.
  //
  int glue;

  int size; // Actual size of 'literals' (at least 2).
  int pos;  // Position of last watch replacement [Gent'13].

  // This 'flexible array member' is of variadic 'size' (and actually
  // shrunken if strengthened) and keeps the literals close to the header of
  // the clause to avoid another pointer dereference, which would be costly.

  // In earlier versions we used 'literals[2]' to fake it (in order to
  // support older Microsoft compilers even though this feature is in C99)
  // and at the same time being able to overlay the first two literals with
  // the 'copy' field above, as having a flexible array member inside a
  // union is not allowed.  Now compilers start to figure out that those
  // literals can be accessed with indices larger than 1 and produce
  // warnings.  After having the 'id' field mandatory we now overlay that
  // one with the copy field.

  // However, it turns out that even though flexible array members are in
  // C99 they are not in C11++, and therefore pedantic compilation with
  // '--pedantic' fails completely. Therefore we still support as
  // alternative faked flexible array members, which unfortunately need
  // then again more care when accessing the literals outside the faked
  // virtual sizes and the compiler can somehow figure that out, because
  // that would in turn produce a warning.

#ifndef NFLEXIBLE
  int literals[];
#else
  int literals[2];
#endif

  // Supports simple range based for loops over clauses.

  literal_iterator begin () { return literals; }
  literal_iterator end () { return literals + size; }

  const_literal_iterator begin () const { return literals; }
  const_literal_iterator end () const { return literals + size; }

  static size_t bytes (int size) {

    // Memory sanitizer insists that clauses put into consecutive memory in
    // the arena are still 8 byte aligned.  We could also allocate 8 byte
    // aligned memory there.  However, assuming the real memory foot print
    // of a clause is 8 bytes anyhow, we just allocate 8 byte aligned memory
    // all the time (even if allocated outside of the arena).
    //
    CADICAL_assert (size > 1);
    const size_t header_bytes = sizeof (Clause);
    const size_t actual_literal_bytes = size * sizeof (int);
    size_t combined_bytes = header_bytes + actual_literal_bytes;
#ifdef NFLEXIBLE
    const size_t faked_literals_bytes = sizeof ((Clause *) 0)->literals;
    combined_bytes -= faked_literals_bytes;
#endif
    size_t aligned_bytes = align (combined_bytes, 8);
    return aligned_bytes;
  }

  size_t bytes () const { return bytes (size); }

  // Check whether this clause is ready to be collected and deleted.  The
  // 'reason' flag is only there to protect reason clauses in 'reduce',
  // which does not backtrack to the root level.  If garbage collection is
  // triggered from a preprocessor, which backtracks to the root level, then
  // 'reason' is false for sure. We want to use the same garbage collection
  // code though for both situations and thus hide here this variance.
  //
  bool collect () const { return !reason && garbage; }
};

struct clause_smaller_size {
  bool operator() (const Clause *a, const Clause *b) {
    return a->size < b->size;
  }
};

/*------------------------------------------------------------------------*/

// Place literals over the same variable close to each other.  This would
// allow eager removal of identical literals and detection of tautological
// clauses but is only currently used for better logging (see also
// 'opts.logsort' in 'logging.cpp').

struct clause_lit_less_than {
  bool operator() (int a, int b) const {
    using namespace std;
    int s = abs (a), t = abs (b);
    return s < t || (s == t && a < b);
  }
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
