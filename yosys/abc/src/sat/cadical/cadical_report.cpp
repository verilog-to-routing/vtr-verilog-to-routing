#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

#ifndef CADICAL_QUIET

/*------------------------------------------------------------------------*/

// Provide nicely formatted progress report messages while running through
// the 'report' function below.  The code is so complex, because it should
// be easy to add and remove reporting of certain statistics, while still
// providing a nicely looking format with automatically aligned headers.

/*------------------------------------------------------------------------*\

The 'reports' are shown as 'c <char> ...' with '<char>' as follows:

i  propagated learned unit clause
O  backtracked after phases reset to original phase
F  backtracked after flipping phases
#  backtracked after randomly setting phases
B  backtracked after resetting to best phases
W  backtracked after local search improved phases
b  blocked clause elimination
G  before garbage collection
C  after garbage collection
/  compacted internal literals and remapped external to internal
c  covered clause elimination
d  decomposed binary implication graph and substituted equivalent literals
2  removed duplicated binary clauses
e  bounded variable elimination round
^  variable elimination bound increased
I  variable instantiation
[  start of stable search phase
]  end of stable search phase
{  start of unstable search phase
}  end of unstable search phase
P  preprocessing round (capital 'P')
L  local search round
*  start of solving without the need to restore clauses
+  start of solving before restoring clauses
r  start of solving after restoring clauses
1  end of solving returns satisfiable
0  end of solving returns unsatisfiable
?  end of solving due to interrupt
l  lucky phase solving
p  failed literal probing round (lower case 'p')
.  before reducing redundant clauses
f  flushed redundant clauses
-  reduced redundant clauses
~  start of resetting phases
R  restart
s  subsumed clause removal round
3  ternary resolution round
t  transition reduction of binary implication graph
u  vivified tier1 clauses
v  vivified tier2 clauses
x  vivified tier3 clauses
w  vivified irredundant clauses

The order of the list follows the occurrences of 'report' in the source
files, i.e., obtained from "grep 'report (' *.cpp".   Note that some of the
reports are only printed for higher verbosity level (for instance 'R').

\*------------------------------------------------------------------------*/

struct Report {

  const char *header;
  char buffer[32];
  int pos;

  Report (const char *h, int precision, int min, double value);
  Report () {}

  void print_header (char *line);
};

/*------------------------------------------------------------------------*/

void Report::print_header (char *line) {
  int len = strlen (header);
  for (int i = -1, j = pos - (len + 1) / 2 - 3; i < len; i++, j++)
    line[j] = i < 0 ? ' ' : header[i];
}

Report::Report (const char *h, int precision, int min, double value)
    : header (h) {
  char fmt[32];
  if (precision < 0)
    snprintf (fmt, sizeof fmt, "%%.%df", -precision - 1);
  else
    snprintf (fmt, sizeof fmt, "%%.%df", precision);
  snprintf (buffer, sizeof buffer, fmt, value);
  const int width = strlen (buffer);
  if (precision < 0)
    strcat (buffer, "%");
  if (width >= min)
    return;
  if (precision < 0)
    snprintf (fmt, sizeof fmt, "%%%d.%df%%%%", min, -precision - 1);
  else
    snprintf (fmt, sizeof fmt, "%%%d.%df", min, precision);
  snprintf (buffer, sizeof buffer, fmt, value);
}

/*------------------------------------------------------------------------*/

// The following statistics are printed in columns, whenever 'report' is
// called.  For instance 'reduce' with prefix '-' will call it.  The other
// more interesting report is due to learning a unit, called iteration, with
// prefix 'i'.  To add another statistics column, add a corresponding line
// here.  If you want to report something else add 'report (..)' functions.

#define TIME opts.reportsolve ? solve_time () : time ()

#define MB (current_resident_set_size () / (double) (1l << 20))

#define REMAINING (percent (active (), stats.variables_original))

#define TRAIL (percent (averages.current.trail.slow, max_var))

#define TARGET (percent (target_assigned, max_var))

#define BEST (percent (best_assigned, max_var))

#define REPORTS \
  /*     HEADER, PRECISION, MIN, VALUE */ \
  REPORT ("seconds", 2, 5, TIME) \
  REPORT ("MB", 0, 2, MB) \
  REPORT ("level", 0, 2, averages.current.level) \
  REPORT ("reductions", 0, 1, stats.reductions) \
  REPORT ("restarts", 0, 3, stats.restarts) \
  REPORT ("rate", 0, 2, averages.current.decisions) \
  REPORT ("conflicts", 0, 4, stats.conflicts) \
  REPORT ("redundant", 0, 4, stats.current.redundant) \
  REPORT ("trail", -1, 2, TRAIL) \
  REPORT ("glue", 0, 1, averages.current.glue.slow) \
  REPORT ("irredundant", 0, 4, stats.current.irredundant) \
  REPORT ("variables", 0, 3, active ()) \
  REPORT ("remaining", -1, 2, REMAINING)

// Note, keep an empty line before this line (because of '\')!

#if 0 // ADDITIONAL STATISTICS TO REPORT

REPORT("best",        -1, 2, BEST) \
REPORT("target",      -1, 2, TARGET) \
REPORT("maxvar",       0, 2, external->max_var)

#endif

static const int num_reports = // as compile time constant
#define REPORT(HEAD, PREC, MIN, EXPR) 1 +
    REPORTS
#undef REPORT
    0;

/*------------------------------------------------------------------------*/

void Internal::report (char type, int verbose) {
  if (!opts.report)
    return;
#ifdef LOGGING
  if (!opts.log)
#endif
    if (opts.quiet || (verbose > opts.verbose))
      return;
  if (!reported) {
    CADICAL_assert (!lim.report);
    reported = true;
    MSG ("%stime measured in %s time %s%s", tout.magenta_code (),
         internal->opts.realtime ? "real" : "process",
         internal->opts.reportsolve ? "in solving" : "since initialization",
         tout.normal_code ());
  }
  Report reports[num_reports];
  int n = 0;
#define REPORT(HEAD, PREC, MIN, EXPR) \
  CADICAL_assert (n < num_reports); \
  reports[n++] = Report (HEAD, PREC, MIN, (double) (EXPR));
  REPORTS
#undef REPORT
  if (!lim.report) {
    print_prefix ();
    fputc ('\n', stdout);
    int pos = 4;
    for (int i = 0; i < n; i++) {
      int len = strlen (reports[i].buffer);
      reports[i].pos = pos + (len + 1) / 2;
      pos += len + 1;
    }
    const int max_line = pos + 20, nrows = 3;
    char *line = new char[max_line];
    for (int start = 0; start < nrows; start++) {
      int i;
      for (i = 0; i < max_line; i++)
        line[i] = ' ';
      for (i = start; i < n; i += nrows)
        reports[i].print_header (line);
      for (i = max_line - 1; line[i - 1] == ' '; i--)
        ;
      line[i] = 0;
      print_prefix ();
      tout.yellow ();
      fputs (line, stdout);
      tout.normal ();
      fputc ('\n', stdout);
    }
    print_prefix ();
    fputc ('\n', stdout);
    delete[] line;
    lim.report = 19;
  } else
    lim.report--;
  print_prefix ();
  switch (type) {
  case '[':
  case ']':
    tout.magenta (true);
    break;
  case 's':
  case 'b':
  case 'c':
    tout.green (false);
    break;
  case 'e':
    tout.green (true);
    break;
  case 'p':
  case '2':
  case '3':
  case 'u':
  case 'v':
  case 'w':
  case 'x':
  case 'f':
  case '=':
    tout.blue (false);
    break;
  case 't':
    tout.cyan (false);
    break;
  case 'd':
    tout.blue (true);
    break;
  case 'z':
  case '!':
    tout.cyan (true);
    break;
  case '-':
    tout.normal ();
    break;
  case '/':
    tout.yellow (true);
    break;
  case 'a':
  case 'n':
    tout.red (false);
    break;
  case '0':
  case '1':
  case '?':
  case 'i':
    tout.bold ();
    break;
  case 'L':
  case 'P':
    tout.bold ();
    tout.underline ();
    break;
  default:
    break;
  }
  fputc (type, stdout);
  if (stable || type == ']')
    tout.magenta ();
  else if (type != 'L' && type != 'P')
    tout.normal ();
  for (int i = 0; i < n; i++) {
    fputc (' ', stdout);
    fputs (reports[i].buffer, stdout);
  }
  if (stable || type == 'L' || type == 'P' || type == ']')
    tout.normal ();
  fputc ('\n', stdout);
  fflush (stdout);
}

#else // ifndef CADICAL_QUIET

void Internal::report (char, int) {}

#endif

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
