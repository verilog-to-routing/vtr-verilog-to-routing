#ifndef _fifo_h_INCLUDED
#define _fifo_h_INCLUDED

#include "stack.h"

#include <string.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

#define FIFO(TYPE) \
  struct { \
    TYPE *begin; \
    TYPE *end; \
    TYPE *start; \
    TYPE *limit; \
    TYPE *allocated; \
  }

#define BEGIN_FIFO(F) ((F).begin)
#define END_FIFO(F) ((F).end)
#define START_FIFO(F) ((F).start)
#define LIMIT_FIFO(F) ((F).limit)
#define ALLOCATED_FIFO(F) ((F).allocated)

#define INIT_FIFO(F) memset (&(F), 0, sizeof (F))

#define EMPTY_FIFO(F) (END_FIFO (F) == BEGIN_FIFO (F))
#define FULL_FIFO(F) (END_FIFO (F) == ALLOCATED_FIFO (F))
#define SIZE_FIFO(F) (END_FIFO (F) - BEGIN_FIFO (F))

#define MOVABLE_FIFO(F) (BEGIN_FIFO (F) == LIMIT_FIFO (F))
#define CAPACITY_FIFO(F) (ALLOCATED_FIFO (F) - START_FIFO (F))

#define ENLARGE_FIFO(T, F)                       \
  do { \
    size_t OLD_BEGIN_OFFSET = BEGIN_FIFO (F) - START_FIFO (F); \
    size_t OLD_END_OFFSET = END_FIFO (F) - START_FIFO (F); \
    size_t OLD_CAPACITY = CAPACITY_FIFO (F); \
    size_t NEW_CAPACITY = OLD_CAPACITY ? 2 * OLD_CAPACITY : 2; \
    size_t OLD_BYTES = OLD_CAPACITY * sizeof *BEGIN_FIFO (F); \
    size_t NEW_BYTES = NEW_CAPACITY * sizeof *BEGIN_FIFO (F); \
    START_FIFO (F) = \
      (T*) kissat_realloc (solver, START_FIFO (F), OLD_BYTES, NEW_BYTES); \
    ALLOCATED_FIFO (F) = START_FIFO (F) + NEW_CAPACITY; \
    LIMIT_FIFO (F) = START_FIFO (F) + NEW_CAPACITY / 2; \
    BEGIN_FIFO (F) = START_FIFO (F) + OLD_BEGIN_OFFSET; \
    END_FIFO (F) = START_FIFO (F) + OLD_END_OFFSET; \
    KISSAT_assert (BEGIN_FIFO (F) < LIMIT_FIFO (F)); \
  } while (0)

#define MOVE_FIFO(F) \
  do { \
    size_t SIZE = SIZE_FIFO (F); \
    size_t BYTES = SIZE * sizeof *BEGIN_FIFO (F); \
    memmove (START_FIFO (F), BEGIN_FIFO (F), BYTES); \
    BEGIN_FIFO (F) = START_FIFO (F); \
    END_FIFO (F) = BEGIN_FIFO (F) + SIZE; \
  } while (0)

#define ENQUEUE_FIFO(T, F, E)                    \
  do { \
    if (FULL_FIFO (F)) \
      ENLARGE_FIFO (T, F);  \
    *END_FIFO (F)++ = (E); \
  } while (0)

#define DEQUEUE_FIFO(F, E) \
  do { \
    KISSAT_assert (!EMPTY_FIFO (F)); \
    (E) = *BEGIN_FIFO (F)++; \
    if (MOVABLE_FIFO (F)) \
      MOVE_FIFO (F); \
  } while (0)

#define POP_FIFO(F) (KISSAT_assert (!EMPTY_FIFO (F)), *--END_FIFO (F))

#define RELEASE_FIFO(F) \
  do { \
    size_t CAPACITY = CAPACITY_FIFO (F); \
    size_t BYTES = CAPACITY * sizeof *BEGIN_FIFO (F); \
    kissat_free (solver, START_FIFO (F), BYTES); \
    INIT_FIFO (F); \
  } while (0)

#define CLEAR_FIFO(F) \
  do { \
    BEGIN_FIFO (F) = END_FIFO (F) = START_FIFO (F); \
  } while (0)

struct unsigned_fifo {
  unsigned *begin, *end;
  unsigned *start, *limit, *allocated;
};

typedef struct unsigned_fifo unsigned_fifo;

#define all_fifo all_stack

ABC_NAMESPACE_HEADER_END

#endif
