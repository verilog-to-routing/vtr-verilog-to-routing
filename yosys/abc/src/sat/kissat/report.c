#include "global.h"

#ifndef KISSAT_QUIET

#include "report.h"
#include "colors.h"
#include "internal.h"
#include "print.h"
#include "resources.h"

#include <inttypes.h>
#include <string.h>

#define MB (kissat_current_resident_set_size () / (double) (1 << 20))

#define REMAINING_VARIABLES \
  kissat_percent (solver->active, statistics->variables_original)

#define REPORTS \
  REP ("seconds", "5.2f", kissat_time (solver)) \
  REP ("MB", "2.0f", MB) \
  REP ("level", ".0f", AVERAGE (level)) \
  REP ("switched", "1" PRIu64, statistics->switched) \
  REP ("reductions", "1" PRIu64, statistics->reductions) \
  REP ("restarts", "2" PRIu64, statistics->restarts) \
  REP ("rate", ".0f", AVERAGE (decision_rate)) \
  REP ("conflicts", "3" PRIu64, CONFLICTS) \
  REP ("redundant", "3" PRIu64, REDUNDANT_CLAUSES) \
  REP ("size/glue", ".1f", \
       kissat_average (AVERAGE (size), AVERAGE (slow_glue))) \
  REP ("size", ".0f", AVERAGE (size)) \
  REP ("glue", ".0f", AVERAGE (slow_glue)) \
  REP ("tier1", "1u", solver->tier1[solver->stable]) \
  REP ("tier2", "1u", solver->tier2[solver->stable]) \
  REP ("trail", ".0f%%", AVERAGE (trail)) \
  REP ("binary", "3" PRIu64, BINARY_CLAUSES) \
  REP ("irredundant", "2" PRIu64, IRREDUNDANT_CLAUSES) \
  REP ("variables", "2u", solver->active) \
  REP ("remaining", "1.0f%%", REMAINING_VARIABLES)

void kissat_report (kissat *solver, bool verbose, char type) {
  statistics *statistics = &solver->statistics;
  const int verbosity = kissat_verbosity (solver);
  if (verbosity < 0)
    return;
  if (verbose && verbosity < 2)
    return;
  char line[128], *p = line;
  unsigned pad[32], n = 1, pos = 0;
  pad[0] = 0;
  // clang-format off
#define REP(NAME,FMT,VALUE) \
  do { \
    *p++ = ' ', pos++; \
    sprintf (p, "%" FMT, VALUE); \
    while (*p) \
      p++, pos++; \
    pad[n++] = pos; \
  } while (0);
  REPORTS
#undef REP
  // clang-format on
  KISSAT_assert (p < line + sizeof line);
  TERMINAL (stdout, 1);
  if (!(solver->limits.reports++ % 20)) {
#define ROWS 3
    unsigned last[ROWS];
    char rows[ROWS][128], *r[ROWS];
    for (unsigned j = 0; j < ROWS; j++)
      last[j] = 0, rows[j][0] = 0, r[j] = rows[j];
    unsigned row = 0, i = 1;
#define REP(NAME, FMT, VALUE) \
  do { \
    if (last[row]) \
      *r[row]++ = ' ', last[row]++; \
    unsigned target = pad[i]; \
    const unsigned name_len = strlen (NAME); \
    const unsigned val_len = target - pad[i - 1] - 1; \
    if (val_len < name_len) \
      target += (name_len - val_len) / 2; \
    while (last[row] + name_len < target) \
      *r[row]++ = ' ', last[row]++; \
    for (const char *p = NAME; *p; p++) \
      *r[row]++ = *p, last[row]++; \
    if (++row == ROWS) \
      row = 0; \
    i++; \
  } while (0);
    REPORTS
#undef REP
    KISSAT_assert (i == n);
    for (unsigned j = 0; j < ROWS; j++) {
      KISSAT_assert (r[j] < rows[j] + sizeof rows[j]);
      *r[j] = 0;
    }
    if (solver->limits.reports > 1)
      kissat_line (solver);
    for (unsigned j = 0; j < ROWS; j++) {
      fputs (solver->prefix, stdout);
      COLOR (CYAN);
      fputs (rows[j], stdout);
      COLOR (NORMAL);
      fputc ('\n', stdout);
    }
    kissat_line (solver);
  }
  kissat_prefix (solver);
  switch (type) {
  case '1':
  case '0':
  case '?':
  case 'i':
  case '.':
    COLOR (BOLD);
    break;
  case 'e':
    COLOR (BOLD GREEN);
    break;
  case '2':
  case 's':
    COLOR (GREEN);
    break;
  case 'f':
  case 't':
  case 'u':
  case 'v':
  case 'w':
  case 'x':
    COLOR (BLUE);
    break;
  case 'b':
  case 'c':
  case 'd':
  case '=':
    COLOR (BOLD BLUE);
    break;
  case '[':
  case ']':
    COLOR (MAGENTA);
    break;
  case '(':
  case ')':
    COLOR (BOLD YELLOW);
  }
  fputc (type, stdout);
  COLOR (NORMAL);
  if (solver->preprocessing)
    COLOR (YELLOW);
  else if (solver->stable)
    COLOR (MAGENTA);
  fputs (line, stdout);
  COLOR (NORMAL);
  fputc ('\n', stdout);
  fflush (stdout);
}

#else

int kissat_report_dummy_to_avoid_warning;

#endif
