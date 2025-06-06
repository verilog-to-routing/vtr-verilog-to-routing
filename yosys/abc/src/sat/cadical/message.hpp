#ifndef _message_h_INCLUDED
#define _message_h_INCLUDED

#include "global.h"

ABC_NAMESPACE_CXX_HEADER_START

/*------------------------------------------------------------------------*/

// Macros for compact message code.

#ifndef CADICAL_QUIET

#define LINE() \
  do { \
    if (internal) \
      internal->message (); \
  } while (0)

#define MSG(...) \
  do { \
    if (internal) \
      internal->message (__VA_ARGS__); \
  } while (0)

#define PHASE(...) \
  do { \
    if (internal) \
      internal->phase (__VA_ARGS__); \
  } while (0)

#define SECTION(...) \
  do { \
    if (internal) \
      internal->section (__VA_ARGS__); \
  } while (0)

#define VERBOSE(...) \
  do { \
    if (internal) \
      internal->verbose (__VA_ARGS__); \
  } while (0)

#else

#define LINE() \
  do { \
  } while (0)
#define MSG(...) \
  do { \
  } while (0)
#define PHASE(...) \
  do { \
  } while (0)
#define SECTION(...) \
  do { \
  } while (0)
#define VERBOSE(...) \
  do { \
  } while (0)

#endif

#define FATAL fatal
#define WARNING(...) internal->warning (__VA_ARGS__)

/*------------------------------------------------------------------------*/

ABC_NAMESPACE_CXX_HEADER_END

#endif // ifndef _message_h_INCLUDED
