#ifndef _limit_hpp_INCLUDED
#define _limit_hpp_INCLUDED

#include "global.h"

#include <cstdint>
#include <limits>

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

struct Internal;

struct Limit {

  bool initialized;

  int64_t conflicts;     // conflict limit if non-negative
  int64_t decisions;     // decision limit if non-negative
  int64_t preprocessing; // limit on preprocessing rounds
  int64_t localsearch;   // limit on local search rounds

  int64_t compact;   // conflict limit for next 'compact'
  int64_t condition; // conflict limit for next 'condition'
  int64_t elim;      // conflict limit for next 'elim'
  int64_t flush;     // conflict limit for next 'flush'
  int64_t inprobe;   // conflict limit for next 'inprobe'
  int64_t reduce;    // conflict limit for next 'reduce'
  int64_t rephase;   // conflict limit for next 'rephase'
  int64_t report;    // report limit for header
  int64_t restart;   // conflict limit for next 'restart'
  int64_t stabilize; // conflict/ticks limit for next 'stabilize'

  int keptsize;           // maximum kept size in 'reduce'
  int keptglue;           // maximum kept glue in 'reduce'
  int64_t recompute_tier; // conflict limit for next tier recomputation

  // How often rephased during (1) or out (0) of stabilization.
  //
  int64_t rephased[2];

  // Current elimination bound per eliminated variable.
  //
  int64_t elimbound;

  struct {
    int check;  // countdown to next terminator call
    int forced; // forced termination for testing
  } terminate;

  Limit ();
};

struct Delay {
  struct {
    int64_t interval = 0, limit = 0;
    bool bypass = 0;

    bool delay () {
      if (bypass)
        return true;
      if (limit) {
        --limit;
        return true;
      } else {
        return false;
      }
    }

    void bump_delay () {
      interval += interval < INT64_MAX;
      limit = interval;
    }

    void reduce_delay () {
      if (!interval)
        return;
      interval /= 2;
      limit = interval;
    }

    void bypass_delay () { bypass = 1; }
    void unbypass_delay () { bypass = 0; }
  } bumpreasons;
};

struct Last {
  struct {
    int64_t propagations;
  } transred;
  struct {
    int64_t ticks;
  } sweep, vivify, probe;
  struct {
    int64_t fixed, subsumephases, marked;
  } elim;
  struct {
    int64_t reductions;
  } inprobe;
  struct {
    int64_t conflicts;
  } reduce, rephase;
  struct {
    int64_t ticks;
    int64_t marked;
  } ternary;
  struct {
    int64_t fixed;
  } collect;
  struct {
    int64_t marked, ticks;
  } factor;
  struct {
    int64_t conflicts;
    int64_t ticks;
  } stabilize;
  Last ();
};

struct Inc {
  int64_t flush;         // flushing interval in terms of conflicts
  int64_t stabilize;     // base ticks limit after first mode switch
  int64_t conflicts;     // next conflict limit if non-negative
  int64_t decisions;     // next decision limit if non-negative
  int64_t preprocessing; // next preprocessing limit if non-negative
  int64_t localsearch;   // next local search limit if non-negative
  Inc ();
};

#define SET_EFFORT_LIMIT(LIMIT, NAME, THRESHHOLD) \
  int64_t LIMIT; \
  do { \
    const int64_t OLD_LIMIT = stats.ticks.NAME; \
    const int64_t TICKS = stats.ticks.search[0] + stats.ticks.search[1]; \
    const int64_t LAST = last.NAME.ticks; \
    int64_t REFERENCE = TICKS - LAST; \
    if (!REFERENCE || !stats.conflicts) { \
      VERBOSE (2, "last %" PRId64 " current %" PRId64 " delta %" PRId64, \
               LAST, TICKS, REFERENCE); \
      REFERENCE = opts.preprocessinit; \
    } \
    const double EFFORT = (double) opts.NAME##effort * 1e-3; \
    const int64_t DELTA = EFFORT * REFERENCE; \
    const int64_t THRESH = opts.NAME##thresh * clauses.size (); \
    if (THRESHHOLD && DELTA < THRESH) { \
      VERBOSE (2, \
               "delaying %s with ticklimit %" PRId64 \
               " and threshhold %" PRId64, \
               #NAME, DELTA, THRESH); \
      return false; \
    } \
    last.NAME.ticks = TICKS; \
    const int64_t NEW_LIMIT = OLD_LIMIT + DELTA; \
    LIMIT = NEW_LIMIT; \
  } while (0)

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
