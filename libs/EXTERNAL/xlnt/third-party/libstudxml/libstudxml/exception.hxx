// file      : libstudxml/exception.hxx -*- C++ -*-
// license   : MIT; see accompanying LICENSE file

#ifndef LIBSTUDXML_EXCEPTION_HXX
#define LIBSTUDXML_EXCEPTION_HXX

#include <libstudxml/details/pre.hxx>

#include <exception>

namespace xml
{
  class exception: public std::exception {};
}

#include <libstudxml/details/post.hxx>

#endif // LIBSTUDXML_EXCEPTION_HXX
