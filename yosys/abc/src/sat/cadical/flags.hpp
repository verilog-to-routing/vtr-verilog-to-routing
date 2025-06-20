#ifndef _flags_hpp_INCLUDED
#define _flags_hpp_INCLUDED

#include "global.h"

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

struct Flags { // Variable flags.

  //  The first set of flags is related to 'analyze' and 'minimize'.
  //
  bool seen : 1;       // seen in generating first UIP clause in 'analyze'
  bool keep : 1;       // keep in learned clause in 'minimize'
  bool poison : 1;     // can not be removed in 'minimize'
  bool removable : 1;  // can be removed in 'minimize'
  bool shrinkable : 1; // can be removed in 'shrink'
  bool added : 1; // has already been added to lrat_chain (in 'minimize')

  // These three variable flags are used to schedule clauses in subsumption
  // ('subsume'), variables in bounded variable elimination ('elim') and in
  // hyper ternary resolution ('ternary').
  //
  bool elim : 1;    // removed since last 'elim' round (*)
  bool subsume : 1; // added since last 'subsume' round (*)
  bool ternary : 1; // added in ternary clause since last 'ternary' (*)
  bool sweep : 1;
  bool blockable : 1;

  unsigned char
      marked_signed : 2; // generate correct LRAT chains in decompose
  unsigned char factor : 2;

  // These literal flags are used by blocked clause elimination ('block').
  //
  unsigned char block : 2; // removed since last 'block' round (*)
  unsigned char skip : 2;  // skip this literal as blocking literal

  // Bits for handling assumptions.
  //
  unsigned char assumed : 2;
  unsigned char failed : 2; // 0 if not part of failure
                            // 1 if positive lit is in failure
                            // 2 if negated lit is in failure

  enum {
    UNUSED = 0,
    ACTIVE = 1,
    FIXED = 2,
    ELIMINATED = 3,
    SUBSTITUTED = 4,
    PURE = 5
  };

  unsigned char status : 3;

  // Initialized explicitly in 'Internal::init' through this function.
  //
  Flags () {
    seen = keep = poison = removable = shrinkable = added = sweep = false;
    subsume = elim = ternary = true;
    block = 3u;
    skip = assumed = failed = marked_signed = factor = 0;
    status = UNUSED;
  }

  bool unused () const { return status == UNUSED; }
  bool active () const { return status == ACTIVE; }
  bool fixed () const { return status == FIXED; }
  bool eliminated () const { return status == ELIMINATED; }
  bool substituted () const { return status == SUBSTITUTED; }
  bool pure () const { return status == PURE; }

  // The flags marked with '(*)' are copied during 'External::copy_flags',
  // which in essence means they are reset in the copy if they were clear.
  // This avoids the effort of fruitless preprocessing the copy.

  void copy (Flags &dst) const {
    dst.elim = elim;
    dst.subsume = subsume;
    dst.ternary = ternary;
    dst.block = block;
  }
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
