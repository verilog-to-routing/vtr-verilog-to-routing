#ifndef _flags_h_INCLUDED
#define _flags_h_INCLUDED

#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

typedef struct flags flags;

struct flags {
  bool active : 1;
  bool backbone0 : 1;
  bool backbone1 : 1;
  bool eliminate : 1;
  bool eliminated : 1;
  unsigned factor : 2;
  bool fixed : 1;
  bool subsume : 1;
  bool sweep : 1;
  bool transitive : 1;
};

#define FLAGS(IDX) (KISSAT_assert ((IDX) < VARS), (solver->flags + (IDX)))

#define ACTIVE(IDX) (FLAGS (IDX)->active)
#define ELIMINATED(IDX) (FLAGS (IDX)->eliminated)

struct kissat;

void kissat_activate_literal (struct kissat *, unsigned);
void kissat_activate_literals (struct kissat *, unsigned, unsigned *);

void kissat_mark_eliminated_variable (struct kissat *, unsigned idx);
void kissat_mark_fixed_literal (struct kissat *, unsigned lit);

void kissat_mark_added_literals (struct kissat *, unsigned, unsigned *);
void kissat_mark_removed_literals (struct kissat *, unsigned, unsigned *);

ABC_NAMESPACE_HEADER_END

#endif
