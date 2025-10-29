// file      : examples/performance/time.cxx
// copyright : not copyrighted - public domain

#include "time.hxx"

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>  // GetSystemTimeAsFileTime
#else
#  include <time.h>     // gettimeofday
#  include <sys/time.h> // timeval
#endif

#include <ostream> // std::ostream
#include <iomanip> // std::setfill, std::setw

namespace os
{
  time::
  time ()
  {
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime (&ft);
    unsigned long long v (
      ((unsigned long long) (ft.dwHighDateTime) << 32) + ft.dwLowDateTime);

    sec_  = static_cast<unsigned long> (v / 10000000ULL);
    nsec_ = static_cast<unsigned long> ((v % 10000000ULL) * 100);
#else
    timeval tv;
    if (gettimeofday(&tv, 0) != 0)
      throw failed ();

    sec_  = static_cast<unsigned long> (tv.tv_sec);
    nsec_ = static_cast<unsigned long> (tv.tv_usec * 1000);
#endif
  }

  std::ostream&
  operator<< (std::ostream& o, time const& t)
  {
    return o << t.sec () << '.'
             << std::setfill ('0') << std::setw (9) << t.nsec ();
  }
}
