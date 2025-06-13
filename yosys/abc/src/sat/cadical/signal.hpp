#ifndef _signal_hpp_INCLUDED
#define _signal_hpp_INCLUDED

#include "global.h"

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

// Helper class for handling signals in applications.

class Handler {
public:
  Handler () {}
  virtual ~Handler () {}
  virtual void catch_signal (int sig) = 0;
#ifndef WIN32
  virtual void catch_alarm ();
#endif
};

class Signal {

public:
  static void set (Handler *);
  static void reset ();
#ifndef WIN32
  static void alarm (int seconds);
  static void reset_alarm ();
#endif

  static const char *name (int sig);
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
