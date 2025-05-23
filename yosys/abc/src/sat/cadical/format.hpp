#ifndef _format_hpp_INCLUDED
#define _format_hpp_INCLUDED

#include "global.h"

#include <cstdarg>
#include <cstdint>

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

// This class provides a 'printf' style formatting utility.
// Only '%c', '%d', '%s' are supported at this point.
// It is used to capture and save an error message.

class Format {
  char *buffer;
  int64_t count, size;
  void enlarge ();
  void push_char (char);
  void push_string (const char *);
  void push_int (int);
  void push_uint64 (uint64_t);
  const char *add (const char *fmt, va_list &);

public:
  Format () : buffer (0), count (0), size (0) {}
  ~Format () {
    if (buffer)
      delete[] buffer;
  }
  const char *init (const char *fmt, ...) CADICAL_ATTRIBUTE_FORMAT (2, 3);
  const char *append (const char *fmt, ...) CADICAL_ATTRIBUTE_FORMAT (2, 3);
  operator const char * () const { return count ? buffer : 0; }
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
