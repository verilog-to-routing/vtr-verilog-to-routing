// file      : libstudxml/details/config.hxx
// license   : MIT; see accompanying LICENSE file

#ifndef LIBSTUDXML_DETAILS_CONFIG_HXX
#define LIBSTUDXML_DETAILS_CONFIG_HXX

// Note that MSVC 14.3 (1900) does not define suitable __cplusplus but
// supports C++11.
//
#if !defined(__cplusplus) || __cplusplus < 201103L
#  if !defined(_MSC_VER) || _MSC_VER < 1900
#    error C++11 is required
#  endif
#endif

#ifdef _MSC_VER
#  include <libstudxml/details/config-vc.h>
#else
#  include <libstudxml/details/config.h>
#endif

#endif // LIBSTUDXML_DETAILS_CONFIG_HXX
