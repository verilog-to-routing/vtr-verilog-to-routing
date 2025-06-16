#ifndef _queue_h_INCLUDED
#define _queue_h_INCLUDED

#include "global.h"
ABC_NAMESPACE_HEADER_START

#define DISCONNECT UINT_MAX
#define DISCONNECTED(IDX) ((int) (IDX) < 0)

struct kissat;

typedef struct links links;
typedef struct queue queue;

struct links {
  unsigned prev, next;
  unsigned stamp;
};

struct queue {
  unsigned first, last, stamp;
  struct {
    unsigned idx, stamp;
  } search;
};

void kissat_init_queue (struct kissat *);
void kissat_reset_search_of_queue (struct kissat *);
void kissat_reassign_queue_stamps (struct kissat *);

#define LINK(IDX) (solver->links[KISSAT_assert ((IDX) < VARS), (IDX)])

#if defined(CHECK_QUEUE) && !defined(KISSAT_NDEBUG)
void kissat_check_queue (struct kissat *);
#else
#define kissat_check_queue(...) \
  do { \
  } while (0)
#endif

ABC_NAMESPACE_HEADER_END

#endif
