#ifndef _handle_h_INCLUDED
#define _handle_h_INCLUDED

#include <signal.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

void kissat_init_signal_handler (void (*handler) (int));
void kissat_reset_signal_handler (void);

void kissat_init_alarm (void (*handler) (void));
void kissat_reset_alarm (void);

#ifdef __MINGW32__
#define SIGNAL_SIGBUS
#else
#define SIGNAL_SIGBUS SIGNAL (SIGBUS)
#endif

#define SIGNALS \
  SIGNAL (SIGABRT) \
  SIGNAL_SIGBUS \
  SIGNAL (SIGINT) \
  SIGNAL (SIGSEGV) \
  SIGNAL (SIGTERM)

// clang-format off

static inline const char *
kissat_signal_name (int sig)
{
#define SIGNAL(SIG) \
  if (sig == SIG) return #SIG;
  SIGNALS
#undef SIGNAL
#ifndef __MINGW32__
  if (sig == SIGALRM)
    return "SIGALRM";
#endif
  return "SIGUNKNOWN";
}

// clang-format on

ABC_NAMESPACE_HEADER_END

#endif
