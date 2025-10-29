// file      : examples/persistence/position.hxx
// copyright : not copyrighted - public domain

#ifndef POSITION_HXX
#define POSITION_HXX

#include <string>
#include <vector>
#include <iosfwd>

#include <libstudxml/forward.hxx> // xml::{parser,serializer} forward declarations.

enum object_type {building, mountain};

// XML parser and serializer will use these operators to convert
// object_type to/from a string representation unless we provide
// an xml::value_traits specialization.
//
std::ostream&
operator<< (std::ostream&, object_type);

std::istream&
operator>> (std::istream&, object_type&);

class position
{
public:
  // Constructors as well as accessor and modifiers not shown.

  // XML persistence.
  //
public:
  position (xml::parser&);

  void
  serialize (xml::serializer&) const;

private:
  float lat_;
  float lon_;
};

class object
{
public:
  typedef std::vector<position> positions_type;

  // Constructors as well as accessor and modifiers not shown.

  // XML persistence.
  //
public:
  object (xml::parser&);

  void
  serialize (xml::serializer&) const;

private:
  std::string name_;
  object_type type_;
  unsigned int id_;
  positions_type positions_;
};

#endif // POSITION_HXX
