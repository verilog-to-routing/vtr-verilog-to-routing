#ifndef _parse_hpp_INCLUDED
#define _parse_hpp_INCLUDED

#include "global.h"

#include <cassert>
#include <vector>

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

// Factors out common functions for parsing of DIMACS and solution files.

class File;
struct External;
struct Internal;

class Parser {

  Solver *solver;
  Internal *internal;
  External *external;
  File *file;

  void perr (const char *fmt, ...) CADICAL_ATTRIBUTE_FORMAT (2, 3);
  int parse_char ();

  enum {
    FORCED = 0,  // Force reading even if header is broken.
    RELAXED = 1, // Relaxed white space treatment in header.
    STRICT = 2,  // Strict white space and header compliance.
  };

  const char *parse_string (const char *str, char prev);
  const char *parse_positive_int (int &ch, int &res, const char *name);
  const char *parse_lit (int &ch, int &lit, int &vars, int strict);
  const char *parse_dimacs_non_profiled (int &vars, int strict);
  const char *parse_solution_non_profiled ();

  bool *parse_inccnf_too;
  vector<int> *cubes;

public:
  // Parse a DIMACS CNF or ICNF file.
  //
  // Return zero if successful. Otherwise parse error.
  Parser (Solver *s, File *f, bool *i, vector<int> *c)
      : solver (s), internal (s->internal), external (s->external),
        file (f), parse_inccnf_too (i), cubes (c) {}

  // Parse a DIMACS file.  Return zero if successful. Otherwise a parse
  // error is return. The parsed clauses are added to the solver and the
  // maximum variable index found is returned in the 'vars' argument. The
  // 'strict' argument can be '0' in which case the numbers in the header
  // can be arbitrary, e.g., 'p cnf 0 0' all the time, without producing a
  // parse error.  Only for this setting the parsed literals are not checked
  // to overflow the maximum variable index of the header.  The strictest
  // form of parsing is enforced  for the value '2' of 'strict', in which
  // case the header can not have additional white space, while a value of
  // '1' exactly relaxes this, e.g., 'p cnf \t  1   3  \r\n' becomes legal.
  //
  const char *parse_dimacs (int &vars, int strict);

  // Parse a solution file as used in the SAT competition, e.g., with
  // comment lines 'c ...', a status line 's ...' and value lines 'v ...'.
  // Returns zero if successful. Otherwise a string is returned describing
  // the parse error.  The parsed solution is saved in 'solution' and can be
  // accessed with 'sol (int lit)'.  We use it for checking learned clauses.
  //
  const char *parse_solution ();
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
