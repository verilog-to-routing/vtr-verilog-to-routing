#include "global.h"

#include "signal.hpp"
#include "cadical.hpp"
#include "resources.hpp"

/*------------------------------------------------------------------------*/

#include <cassert>
#if !defined(__wasm)
#include <csignal>
#endif

/*------------------------------------------------------------------------*/

#if !defined(_WIN32) && !defined(__wasm)
extern "C" {
#include <unistd.h>
}
#endif

ABC_NAMESPACE_IMPL_START

/*------------------------------------------------------------------------*/

// Signal handlers for printing statistics even if solver is interrupted.

namespace CaDiCaL {

static volatile bool caught_signal = false;
static Handler *signal_handler;

#if !defined(_WIN32) && !defined(__wasm)

static volatile bool caught_alarm = false;
static volatile bool alarm_set = false;
static int alarm_time = -1;

void Handler::catch_alarm () { catch_signal (SIGALRM); }

#endif

#if !defined(__wasm)
#define SIGNALS \
  SIGNAL (SIGABRT) \
  SIGNAL (SIGINT) \
  SIGNAL (SIGSEGV) \
  SIGNAL (SIGTERM)

#define SIGNAL(SIG) static void (*SIG##_handler) (int);
SIGNALS
#undef SIGNAL
#endif

#if !defined(_WIN32) && !defined(__wasm)

static void (*SIGALRM_handler) (int);

void Signal::reset_alarm () {
  if (!alarm_set)
    return;
  (void) signal (SIGALRM, SIGALRM_handler);
  SIGALRM_handler = 0;
  caught_alarm = false;
  alarm_set = false;
  alarm_time = -1;
}

#endif

void Signal::reset () {
  signal_handler = 0;
#if !defined(__wasm)
#define SIGNAL(SIG) \
  (void) signal (SIG, SIG##_handler); \
  SIG##_handler = 0;
  SIGNALS
#undef SIGNAL
#ifndef WIN32
  reset_alarm ();
#endif
#endif
  caught_signal = false;
}

const char *Signal::name (int sig) {
#if !defined(__wasm)
#define SIGNAL(SIG) \
  if (sig == SIG) \
    return #SIG;
  SIGNALS
#undef SIGNAL
#ifndef WIN32
  if (sig == SIGALRM)
    return "SIGALRM";
#endif
#endif
  return "UNKNOWN";
}

// TODO printing is not reentrant and might lead to deadlock if the signal
// is raised during another print attempt (and locked IO is used).  To avoid
// this we have to either run our own low-level printing routine here or in
// 'Message' or just dump those statistics somewhere else were we have
// exclusive access to.  All these solutions are painful and not elegant.

static void catch_signal (int sig) {
#if !defined(__wasm)
#ifndef WIN32
  if (sig == SIGALRM && absolute_real_time () >= alarm_time) {
    if (!caught_alarm) {
      caught_alarm = true;
      if (signal_handler)
        signal_handler->catch_alarm ();
    }
    Signal::reset_alarm ();
  } else
#endif
  {
    if (!caught_signal) {
      caught_signal = true;
      if (signal_handler)
        signal_handler->catch_signal (sig);
    }
    Signal::reset ();
    ::raise (sig);
  }
#endif
}

void Signal::set (Handler *h) {
#if !defined(__wasm)
  signal_handler = h;
#define SIGNAL(SIG) SIG##_handler = signal (SIG, catch_signal);
  SIGNALS
#undef SIGNAL
#endif
}

#if !defined(_WIN32) && !defined(__wasm)

void Signal::alarm (int seconds) {
  CADICAL_assert (seconds >= 0);
  CADICAL_assert (!alarm_set);
  CADICAL_assert (alarm_time < 0);
  SIGALRM_handler = signal (SIGALRM, catch_signal);
  alarm_set = true;
  alarm_time = absolute_real_time () + seconds;
  ::alarm (seconds);
}

#endif

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
