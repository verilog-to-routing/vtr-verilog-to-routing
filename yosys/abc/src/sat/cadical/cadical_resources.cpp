#include "global.h"

#include "internal.hpp"

/*------------------------------------------------------------------------*/

// This is operating system specific code.  We mostly develop on Linux and
// there it should be fine and mostly works out-of-the-box on MacOS too but
// Windows needs special treatment (as probably other operating systems
// too).

extern "C" {

#ifdef WIN32

#ifndef __WIN32_WINNT
#define __WIN32_WINNT 0x0600
#endif

// Clang-format would reorder the includes which breaks the Windows code
// as it expects 'windows.h' to be included first.  So disable it here.

// clang-format off

#include <windows.h>
#include <psapi.h>

// clang-format on

#else

#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#endif

#include <string.h>
}

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

#ifdef WIN32

double absolute_real_time () {
  FILETIME f;
  GetSystemTimeAsFileTime (&f);
  ULARGE_INTEGER t;
  t.LowPart = f.dwLowDateTime;
  t.HighPart = f.dwHighDateTime;
  double res = (__int64) t.QuadPart;
  res *= 1e-7;
  return res;
}

double absolute_process_time () {
  double res = 0;
  FILETIME fc, fe, fu, fs;
  if (GetProcessTimes (GetCurrentProcess (), &fc, &fe, &fu, &fs)) {
    ULARGE_INTEGER u, s;
    u.LowPart = fu.dwLowDateTime;
    u.HighPart = fu.dwHighDateTime;
    s.LowPart = fs.dwLowDateTime;
    s.HighPart = fs.dwHighDateTime;
    res = (__int64) u.QuadPart + (__int64) s.QuadPart;
    res *= 1e-7;
  }
  return res;
}

#else

double absolute_real_time () {
  struct timeval tv;
  if (gettimeofday (&tv, 0))
    return 0;
  return 1e-6 * tv.tv_usec + tv.tv_sec;
}

// We use 'getrusage' for 'process_time' and 'maximum_resident_set_size'
// which is pretty standard on Unix but probably not available on Windows
// etc.  For different variants of Unix not all fields are meaningful.

double absolute_process_time () {
  double res;
  struct rusage u;
  if (getrusage (RUSAGE_SELF, &u))
    return 0;
  res = u.ru_utime.tv_sec + 1e-6 * u.ru_utime.tv_usec;  // user time
  res += u.ru_stime.tv_sec + 1e-6 * u.ru_stime.tv_usec; // + system time
  return res;
}

#endif

double Internal::real_time () const {
  return absolute_real_time () - stats.time.real;
}

double Internal::process_time () const {
  return absolute_process_time () - stats.time.process;
}

/*------------------------------------------------------------------------*/

#ifdef WIN32

uint64_t current_resident_set_size () {
  PROCESS_MEMORY_COUNTERS pmc;
  if (GetProcessMemoryInfo (GetCurrentProcess (), &pmc, sizeof (pmc))) {
    return pmc.WorkingSetSize;
  } else
    return 0;
}

uint64_t maximum_resident_set_size () {
  PROCESS_MEMORY_COUNTERS pmc;
  if (GetProcessMemoryInfo (GetCurrentProcess (), &pmc, sizeof (pmc))) {
    return pmc.PeakWorkingSetSize;
  } else
    return 0;
}

#else

// This seems to work on Linux (man page says since Linux 2.6.32).

uint64_t maximum_resident_set_size () {
#ifdef __wasm
  return 0;
#else
  struct rusage u;
  if (getrusage (RUSAGE_SELF, &u))
    return 0;
  return ((uint64_t) u.ru_maxrss) << 10;
#endif
}

// Unfortunately 'getrusage' on Linux does not support current resident set
// size (the field 'ru_ixrss' is there but according to the man page
// 'unused'). Thus we fall back to use the '/proc' file system instead.  So
// this is not portable at all and needs to be replaced on other systems
// The code would still compile though (assuming 'sysconf' and
// '_SC_PAGESIZE' are available).

uint64_t current_resident_set_size () {
  char path[64];
  snprintf (path, sizeof path, "/proc/%" PRId64 "/statm",
            (int64_t) getpid ());
  FILE *file = fopen (path, "r");
  if (!file)
    return 0;
  uint64_t dummy, rss;
  int scanned = fscanf (file, "%" PRIu64 " %" PRIu64 "", &dummy, &rss);
  fclose (file);
  return scanned == 2 ? rss * sysconf (_SC_PAGESIZE) : 0;
}

#endif

/*------------------------------------------------------------------------*/

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
