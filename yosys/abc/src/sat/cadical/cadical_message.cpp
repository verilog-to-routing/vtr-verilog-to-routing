#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/
#ifndef CADICAL_QUIET
/*------------------------------------------------------------------------*/

void Internal::print_prefix () { fputs (prefix.c_str (), stdout); }

void Internal::vmessage (const char *fmt, va_list &ap) {
#ifdef LOGGING
  if (!opts.log)
#endif
    if (opts.quiet)
      return;
  print_prefix ();
  vprintf (fmt, ap);
  fputc ('\n', stdout);
  fflush (stdout);
}

void Internal::message (const char *fmt, ...) {
  va_list ap;
  va_start (ap, fmt);
  vmessage (fmt, ap);
  va_end (ap);
}

void Internal::message () {
#ifdef LOGGING
  if (!opts.log)
#endif
    if (opts.quiet)
      return;
  print_prefix ();
  fputc ('\n', stdout);
  fflush (stdout);
}

/*------------------------------------------------------------------------*/

void Internal::vverbose (int level, const char *fmt, va_list &ap) {
#ifdef LOGGING
  if (!opts.log)
#endif
    if (opts.quiet || level > opts.verbose)
      return;
  print_prefix ();
  vprintf (fmt, ap);
  fputc ('\n', stdout);
  fflush (stdout);
}

void Internal::verbose (int level, const char *fmt, ...) {
  va_list ap;
  va_start (ap, fmt);
  vverbose (level, fmt, ap);
  va_end (ap);
}

void Internal::verbose (int level) {
#ifdef LOGGING
  if (!opts.log)
#endif
    if (opts.quiet || level > opts.verbose)
      return;
  print_prefix ();
  fputc ('\n', stdout);
  fflush (stdout);
}

/*------------------------------------------------------------------------*/

void Internal::section (const char *title) {
#ifdef LOGGING
  if (!opts.log)
#endif
    if (opts.quiet)
      return;
  if (stats.sections++)
    MSG ();
  print_prefix ();
  tout.blue ();
  fputs ("--- [ ", stdout);
  tout.blue (true);
  fputs (title, stdout);
  tout.blue ();
  fputs (" ] ", stdout);
  for (int i = strlen (title) + strlen (prefix.c_str ()) + 9; i < 78; i++)
    fputc ('-', stdout);
  tout.normal ();
  fputc ('\n', stdout);
  MSG ();
}

/*------------------------------------------------------------------------*/

void Internal::phase (const char *phase, const char *fmt, ...) {
#ifdef LOGGING
  if (!opts.log)
#endif
    if (opts.quiet || (!force_phase_messages && opts.verbose < 2))
      return;
  print_prefix ();
  printf ("[%s] ", phase);
  va_list ap;
  va_start (ap, fmt);
  vprintf (fmt, ap);
  va_end (ap);
  fputc ('\n', stdout);
  fflush (stdout);
}

void Internal::phase (const char *phase, int64_t count, const char *fmt,
                      ...) {
#ifdef LOGGING
  if (!opts.log)
#endif
    if (opts.quiet || (!force_phase_messages && opts.verbose < 2))
      return;
  print_prefix ();
  printf ("[%s-%" PRId64 "] ", phase, count);
  va_list ap;
  va_start (ap, fmt);
  vprintf (fmt, ap);
  va_end (ap);
  fputc ('\n', stdout);
  fflush (stdout);
}

/*------------------------------------------------------------------------*/
#endif // ifndef CADICAL_QUIET
/*------------------------------------------------------------------------*/

void Internal::warning (const char *fmt, ...) {
  fflush (stdout);
  terr.bold ();
  fputs ("cadical: ", stderr);
  terr.red (1);
  fputs ("warning:", stderr);
  terr.normal ();
  fputc (' ', stderr);
  va_list ap;
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
  fflush (stderr);
}

/*------------------------------------------------------------------------*/

void Internal::error_message_start () {
  fflush (stdout);
  terr.bold ();
  fputs ("cadical: ", stderr);
  terr.red (1);
  fputs ("error:", stderr);
  terr.normal ();
  fputc (' ', stderr);
}

void Internal::error_message_end () {
  fputc ('\n', stderr);
  fflush (stderr);
  // TODO add possibility to use call back instead.
  exit (1);
}

void Internal::verror (const char *fmt, va_list &ap) {
  error_message_start ();
  vfprintf (stderr, fmt, ap);
  error_message_end ();
}

void Internal::error (const char *fmt, ...) {
  va_list ap;
  va_start (ap, fmt);
  verror (fmt, ap);
  va_end (ap); // unreachable
}

/*------------------------------------------------------------------------*/

void fatal_message_start () {
  fflush (stdout);
  terr.bold ();
  fputs ("cadical: ", stderr);
  terr.red (1);
  fputs ("fatal error:", stderr);
  terr.normal ();
  fputc (' ', stderr);
}

void fatal_message_end () {
  fputc ('\n', stderr);
  fflush (stderr);
  abort ();
}

void fatal (const char *fmt, ...) {
  fatal_message_start ();
  va_list ap;
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fatal_message_end ();
  abort ();
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
