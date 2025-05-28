#ifndef _logging_h_INCLUDED
#define _logging_h_INCLUDED

#include "attribute.h"
#include "extend.h"
#include "reference.h"
#include "watch.h"

#include <stdarg.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

#if defined(LOGGING) && !defined(KISSAT_QUIET)

// clang-format off

const char * kissat_log_lit (kissat *, unsigned lit);
const char * kissat_log_var (kissat *, unsigned idx);
const char * kissat_log_repr (kissat *, unsigned lit, const unsigned *);

void kissat_begin_logging (kissat *, const char *prefix,
                           const char *fmt, ...)
ATTRIBUTE_FORMAT (3, 4);

void kissat_end_logging (void);

void kissat_log_msg (kissat *, const char*, const char *fmt, ...)
ATTRIBUTE_FORMAT (3, 4);

void kissat_log_clause (kissat *, const char*, const clause *,
                        const char *, ...)
ATTRIBUTE_FORMAT (4, 5);

void kissat_log_counted_clause (kissat *, const char*, const clause *,
                                const unsigned *, const char *, ...)
ATTRIBUTE_FORMAT (5, 6);

void kissat_log_repr_clause (kissat *, const char*, const clause *,
			     const unsigned *, const char *, ...)
ATTRIBUTE_FORMAT (5, 6);

void kissat_log_binary (kissat *, const char*,
                        unsigned, unsigned, const char *, ...)
ATTRIBUTE_FORMAT (5, 6);

void kissat_log_unary (kissat *, const char*, unsigned, const char *, ...)
ATTRIBUTE_FORMAT (4, 5);

void kissat_log_lits (kissat *, const char*,
                      size_t, const unsigned *, const char *, ...)
ATTRIBUTE_FORMAT (5, 6);

void kissat_log_litset (kissat *, const char*,
                        size_t, const unsigned *, const char *, ...)
ATTRIBUTE_FORMAT (5, 6);

void kissat_log_litpart (kissat *, const char*,
                         size_t, const unsigned *, const char *, ...)
ATTRIBUTE_FORMAT (5, 6);

void kissat_log_counted_ref_lits (kissat *, const char*, reference,
                                  size_t, const unsigned *,
			          const unsigned * counts, const char *, ...)
ATTRIBUTE_FORMAT (7, 8);

void kissat_log_counted_lits (kissat *, const char*,
			      size_t, const unsigned *,
			      const unsigned * counts, const char *, ...)
ATTRIBUTE_FORMAT (6, 7);

void kissat_log_unsigneds (kissat *, const char*,
                           size_t, const unsigned *, const char *, ...)
ATTRIBUTE_FORMAT (5, 6);

#define INVALID_GATE_ID (~(size_t)0)

void kissat_log_and_gate (kissat *, const char*, size_t id, unsigned * repr,
                          unsigned lhs, size_t, const unsigned * rhs,
			  const char *, ...)
ATTRIBUTE_FORMAT (8, 9);

void kissat_log_xor_gate (kissat *, const char*, size_t id, unsigned * repr,
                          unsigned lhs, size_t, const unsigned * rhs,
			  const char *, ...)
ATTRIBUTE_FORMAT (8, 9);

void kissat_log_ite_gate (kissat *, const char*, size_t id, unsigned * repr,
                          unsigned lhs, unsigned cond, unsigned then_lit,
			  unsigned else_lit, const char *, ...)
ATTRIBUTE_FORMAT (9, 10);

void kissat_log_ints (kissat *, const char*,
                      size_t, const int *, const char *, ...)
ATTRIBUTE_FORMAT (5, 6);

void kissat_log_extensions (kissat *, const char *,
			    size_t, const extension *, const char *, ...)
ATTRIBUTE_FORMAT (5, 6);

void kissat_log_xor (struct kissat *, const char*, unsigned lit,
		     unsigned size, const unsigned *lits, const char *fmt, ...)
ATTRIBUTE_FORMAT (6, 7);

void kissat_log_ref (kissat *, const char*, reference, const char *, ...)
ATTRIBUTE_FORMAT (4, 5);

void kissat_log_resolvent (kissat *, const char*, const char *, ...)
ATTRIBUTE_FORMAT (3, 4);

void kissat_log_watch (kissat *, const char*,
                       unsigned, watch, const char *, ...)
ATTRIBUTE_FORMAT (5, 6);

// clang-format on

#ifndef LOGPREFIX
#define LOGPREFIX "LOG"
#endif

#define LOG(...) \
  do { \
    if (solver && GET_OPTION (log)) \
      kissat_log_msg (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOG2(...) \
  do { \
    if (solver && GET_OPTION (log) > 1) \
      kissat_log_msg (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOG3(...) \
  do { \
    if (solver && GET_OPTION (log) > 2) \
      kissat_log_msg (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOG4(...) \
  do { \
    if (solver && GET_OPTION (log) > 3) \
      kissat_log_msg (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOG5(...) \
  do { \
    if (solver && GET_OPTION (log) > 4) \
      kissat_log_msg (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGLITS(...) \
  do { \
    if (solver && GET_OPTION (log)) \
      kissat_log_lits (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGLITSET(...) \
  do { \
    if (solver && GET_OPTION (log)) \
      kissat_log_litset (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGLITPART(...) \
  do { \
    if (solver && GET_OPTION (log)) \
      kissat_log_litpart (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGCOUNTEDREFLITS(...) \
  do { \
    if (solver && GET_OPTION (log)) \
      kissat_log_counted_ref_lits (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGCOUNTEDLITS(...) \
  do { \
    if (solver && GET_OPTION (log)) \
      kissat_log_counted_lits (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGLITS3(...) \
  do { \
    if (GET_OPTION (log) > 2) \
      kissat_log_lits (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGRES(...) \
  do { \
    if (solver && GET_OPTION (log)) \
      kissat_log_resolvent (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGRES2(...) \
  do { \
    if (solver && GET_OPTION (log) > 1) \
      kissat_log_resolvent (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGEXT(...) \
  do { \
    if (GET_OPTION (log)) \
      kissat_log_extensions (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGEXT2(...) \
  do { \
    if (GET_OPTION (log) > 1) \
      kissat_log_extensions (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGINTS(...) \
  do { \
    if (solver && GET_OPTION (log)) \
      kissat_log_ints (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGINTS3(...) \
  do { \
    if (GET_OPTION (log) > 2) \
      kissat_log_ints (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGUNSIGNEDS2(...) \
  do { \
    if (GET_OPTION (log) > 1) \
      kissat_log_unsigneds (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGUNSIGNEDS3(...) \
  do { \
    if (GET_OPTION (log) > 2) \
      kissat_log_unsigneds (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGANDGATE(...) \
  do { \
    if (GET_OPTION (log) > 0) \
      kissat_log_and_gate (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGXORGATE(...) \
  do { \
    if (GET_OPTION (log) > 0) \
      kissat_log_xor_gate (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGITEGATE(...) \
  do { \
    if (GET_OPTION (log) > 0) \
      kissat_log_ite_gate (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGCLS(...) \
  do { \
    if (solver && GET_OPTION (log)) \
      kissat_log_clause (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGCOUNTEDCLS(...) \
  do { \
    if (solver && GET_OPTION (log)) \
      kissat_log_counted_clause (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGREPRCLS(...) \
  do { \
    if (solver && GET_OPTION (log)) \
      kissat_log_repr_clause (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGLINE(...) \
  do { \
    if (solver && GET_OPTION (log)) \
      kissat_log_line (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGCLS2(...) \
  do { \
    if (GET_OPTION (log) > 1) \
      kissat_log_clause (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGCLS3(...) \
  do { \
    if (GET_OPTION (log) > 2) \
      kissat_log_clause (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGREF(...) \
  do { \
    if (solver && GET_OPTION (log)) \
      kissat_log_ref (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGREF2(...) \
  do { \
    if (solver && GET_OPTION (log) > 1) \
      kissat_log_ref (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGREF3(...) \
  do { \
    if (solver && GET_OPTION (log) > 2) \
      kissat_log_ref (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGBINARY(...) \
  do { \
    if (solver && GET_OPTION (log)) \
      kissat_log_binary (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGBINARY2(...) \
  do { \
    if (GET_OPTION (log) > 1) \
      kissat_log_binary (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGBINARY3(...) \
  do { \
    if (GET_OPTION (log) > 2) \
      kissat_log_binary (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGUNARY(...) \
  do { \
    if (solver && GET_OPTION (log)) \
      kissat_log_unary (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGLIT(LIT) kissat_log_lit (solver, (LIT))
#define LOGVAR(IDX) kissat_log_var (solver, (IDX))
#define LOGREPR(LIT, REPR) kissat_log_repr (solver, (LIT), (REPR))

#define LOGWATCH(...) \
  do { \
    if (GET_OPTION (log)) \
      kissat_log_watch (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#define LOGXOR(...) \
  do { \
    if (GET_OPTION (log)) \
      kissat_log_xor (solver, LOGPREFIX, __VA_ARGS__); \
  } while (0)

#else

#define LOG(...) \
  do { \
  } while (0)
#define LOG2(...) \
  do { \
  } while (0)
#define LOG3(...) \
  do { \
  } while (0)
#define LOG4(...) \
  do { \
  } while (0)
#define LOG5(...) \
  do { \
  } while (0)
#define LOGRES(...) \
  do { \
  } while (0)
#define LOGRES2(...) \
  do { \
  } while (0)
#define LOGLITS(...) \
  do { \
  } while (0)
#define LOGLITSET(...) \
  do { \
  } while (0)
#define LOGLITPART(...) \
  do { \
  } while (0)
#define LOGLITS3(...) \
  do { \
  } while (0)
#define LOGCOUNTEDREFLITS(...) \
  do { \
  } while (0)
#define LOGCOUNTEDLITS(...) \
  do { \
  } while (0)
#define LOGEXT(...) \
  do { \
  } while (0)
#define LOGEXT2(...) \
  do { \
  } while (0)
#define LOGINTS(...) \
  do { \
  } while (0)
#define LOGINTS3(...) \
  do { \
  } while (0)
#define LOGUNSIGNEDS2(...) \
  do { \
  } while (0)
#define LOGUNSIGNEDS3(...) \
  do { \
  } while (0)
#define LOGANDGATE(...) \
  do { \
  } while (0)
#define LOGXORGATE(...) \
  do { \
  } while (0)
#define LOGITEGATE(...) \
  do { \
  } while (0)
#define LOGCLS(...) \
  do { \
  } while (0)
#define LOGCLS2(...) \
  do { \
  } while (0)
#define LOGCLS3(...) \
  do { \
  } while (0)
#define LOGCOUNTEDCLS(...) \
  do { \
  } while (0)
#define LOGREPRCLS(...) \
  do { \
  } while (0)
#define LOGLINE(...) \
  do { \
  } while (0)
#define LOGREF(...) \
  do { \
  } while (0)
#define LOGREF2(...) \
  do { \
  } while (0)
#define LOGREF3(...) \
  do { \
  } while (0)
#define LOGBINARY(...) \
  do { \
  } while (0)
#define LOGBINARY2(...) \
  do { \
  } while (0)
#define LOGBINARY3(...) \
  do { \
  } while (0)
#define LOGUNARY(...) \
  do { \
  } while (0)
#define LOGWATCH(...) \
  do { \
  } while (0)
#define LOGXOR(...) \
  do { \
  } while (0)

#endif

#define LOGTMP(...) \
  LOGLITS (SIZE_STACK (solver->clause), BEGIN_STACK (solver->clause), \
           __VA_ARGS__)

ABC_NAMESPACE_HEADER_END

#endif
