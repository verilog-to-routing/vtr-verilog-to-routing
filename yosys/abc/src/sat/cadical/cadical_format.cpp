#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

void Format::enlarge () {
  char *old = buffer;
  buffer = new char[size = size ? 2 * size : 1];
  memcpy (buffer, old, count);
  delete[] old;
}

inline void Format::push_char (char ch) {
  if (size == count)
    enlarge ();
  buffer[count++] = ch;
}

void Format::push_string (const char *s) {
  char ch;
  while ((ch = *s++))
    push_char (ch);
}

void Format::push_int (int d) {
  char tmp[16];
  snprintf (tmp, sizeof tmp, "%d", d);
  push_string (tmp);
}

void Format::push_uint64 (uint64_t u) {
  char tmp[16];
  snprintf (tmp, sizeof tmp, "%" PRIu64, u);
  push_string (tmp);
}

static bool match_format (const char *&str, const char *pattern) {
  CADICAL_assert (pattern);
  const char *p = str;
  const char *q = pattern;
  while (*q)
    if (*q++ != *p++)
      return false;
  str = p;
  return true;
}

const char *Format::add (const char *fmt, va_list &ap) {
  const char *p = fmt;
  char ch;
  while ((ch = *p++)) {
    if (ch != '%')
      push_char (ch);
    else if (*p == 'c')
      push_char (va_arg (ap, int)), p++;
    else if (*p == 'd')
      push_int (va_arg (ap, int)), p++;
    else if (*p == 's')
      push_string (va_arg (ap, const char *)), p++;
    else if (match_format (p, PRIu64))
      push_uint64 (va_arg (ap, uint64_t));
    else {
      push_char ('%');
      push_char (*p);
      break;
    } // unsupported
  }
  push_char (0);
  count--; // thus automatic append in subsequent calls.
  return buffer;
}

const char *Format::init (const char *fmt, ...) {
  count = 0;
  va_list ap;
  va_start (ap, fmt);
  const char *res = add (fmt, ap);
  va_end (ap);
  return res;
}

const char *Format::append (const char *fmt, ...) {
  va_list ap;
  va_start (ap, fmt);
  const char *res = add (fmt, ap);
  va_end (ap);
  return res;
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
