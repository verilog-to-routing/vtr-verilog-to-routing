// file      : examples/inheritance/position.hxx
// copyright : not copyrighted - public domain

#ifndef POSITION_HXX
#define POSITION_HXX

#include <string>
#include <vector>
#include <iosfwd>

#include <libstudxml/forward.hxx> // xml::{parser,serializer} forward declarations.

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
  serialize_attributes (xml::serializer&) const;

  void
  serialize_content (xml::serializer&) const;

  void
  serialize (xml::serializer& s) const
  {
    serialize_attributes (s);
    serialize_content (s);
  }

private:
  std::string name_;
  unsigned int id_;
  positions_type positions_;
};

class elevation
{
public:
  // Constructors as well as accessor and modifiers not shown.

  // XML persistence.
  //
public:
  elevation (xml::parser&);

  void
  serialize (xml::serializer&) const;

private:
  float value_;
};

class elevated_object: public object
{
public:
  typedef std::vector<elevation> elevations_type;

  // Constructors as well as accessor and modifiers not shown.

  // XML persistence.
  //
public:
  elevated_object (xml::parser&);

  void
  serialize_attributes (xml::serializer&) const;

  void
  serialize_content (xml::serializer&) const;

  void
  serialize (xml::serializer& s) const
  {
    serialize_attributes (s);
    serialize_content (s);
  }

private:
  std::string units_;
  elevations_type elevations_;
};

class objects
{
public:
  typedef std::vector<object> simple_objects_type;
  typedef std::vector<elevated_object> elevated_objects_type;

  // Constructors as well as accessor and modifiers not shown.

  // XML persistence.
  //
public:
  objects (xml::parser&);

  void
  serialize (xml::serializer&) const;

private:
  simple_objects_type simple_objects_;
  elevated_objects_type elevated_objects_;
};

#endif // POSITION_HXX
