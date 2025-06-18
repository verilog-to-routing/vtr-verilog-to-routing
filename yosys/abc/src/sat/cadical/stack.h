#ifndef _stack_h_INCLUDED
#define _stack_h_INCLUDED

#include "global.h"

#include <stdlib.h>

ABC_NAMESPACE_HEADER_START

#define STACK(TYPE) \
  struct { \
    TYPE *begin; \
    TYPE *end; \
    TYPE *allocated; \
  }

#define FULL_STACK(S) ((S).end == (S).allocated)
#define EMPTY_STACK(S) ((S).begin == (S).end)
#define SIZE_STACK(S) ((size_t) ((S).end - (S).begin))
#define CAPACITY_STACK(S) ((size_t) ((S).allocated - (S).begin))

#define INIT_STACK(S) \
  do { \
    (S).begin = (S).end = (S).allocated = 0; \
  } while (0)

#define TOP_STACK(S) (END_STACK (S)[CADICAL_assert (!EMPTY_STACK (S)), -1])

#define PEEK_STACK(S, N) \
  (BEGIN_STACK (S)[CADICAL_assert ((N) < SIZE_STACK (S)), (N)])

#define POKE_STACK(S, N, E) \
  do { \
    PEEK_STACK (S, N) = (E); \
  } while (0)

#define POP_STACK(S) (CADICAL_assert (!EMPTY_STACK (S)), *--(S).end)

#define PUSH_STACK(S, E) \
  do { \
    if (FULL_STACK (S)) \
      ENLARGE_STACK (S); \
    *(S).end++ = (E); \
  } while (0)

#define BEGIN_STACK(S) (S).begin

#define END_STACK(S) (S).end

#define CLEAR_STACK(S) \
  do { \
    (S).end = (S).begin; \
  } while (0)

#define RESIZE_STACK(S, NEW_SIZE) \
  do { \
    const size_t TMP_NEW_SIZE = (NEW_SIZE); \
    CADICAL_assert (TMP_NEW_SIZE <= SIZE_STACK (S)); \
    (S).end = (S).begin + TMP_NEW_SIZE; \
  } while (0)

#define SET_END_OF_STACK(S, P) \
  do { \
    CADICAL_assert (BEGIN_STACK (S) <= (P)); \
    CADICAL_assert ((P) <= END_STACK (S)); \
    if ((P) == END_STACK (S)) \
      break; \
    (S).end = (P); \
  } while (0)

#define RELEASE_STACK(S) \
  do { \
    DEALLOC ((S).begin, CAPACITY_STACK (S)); \
    INIT_STACK (S); \
  } while (0)

#define REMOVE_STACK(T, S, E) \
  do { \
    CADICAL_assert (!EMPTY_STACK (S)); \
    T *END_REMOVE_STACK = END_STACK (S); \
    T *P_REMOVE_STACK = BEGIN_STACK (S); \
    while (*P_REMOVE_STACK != (E)) { \
      P_REMOVE_STACK++; \
      CADICAL_assert (P_REMOVE_STACK != END_REMOVE_STACK); \
    } \
    P_REMOVE_STACK++; \
    while (P_REMOVE_STACK != END_REMOVE_STACK) { \
      P_REMOVE_STACK[-1] = *P_REMOVE_STACK; \
      P_REMOVE_STACK++; \
    } \
    (S).end--; \
  } while (0)

#define all_stack(T, E, S) \
  T E, *E##_PTR = BEGIN_STACK (S), *const E##_END = END_STACK (S); \
  E##_PTR != E##_END && (E = *E##_PTR, true); \
  ++E##_PTR

#define all_pointers(T, E, S) \
  T *E, *const *E##_PTR = BEGIN_STACK (S), \
               *const *const E##_END = END_STACK (S); \
  E##_PTR != E##_END && (E = *E##_PTR, true); \
  ++E##_PTR

// clang-format off

typedef STACK (char) chars;
typedef STACK (int) ints;
typedef STACK (size_t) sizes;
typedef STACK (unsigned) unsigneds;

// clang-format on

ABC_NAMESPACE_HEADER_END

#endif
