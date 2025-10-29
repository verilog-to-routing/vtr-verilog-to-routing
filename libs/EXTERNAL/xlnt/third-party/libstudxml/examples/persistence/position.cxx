// file      : examples/persistence/position.cxx
// copyright : not copyrighted - public domain

#include <iostream>

#include <libstudxml/parser.hxx>
#include <libstudxml/serializer.hxx>

#include "position.hxx"

using namespace std;
using namespace xml;

// position
//
position::
position (parser& p)
    : lat_ (p.attribute<float> ("lat")),
      lon_ (p.attribute<float> ("lon"))
{
    p.content (content::empty);
}

void position::
serialize (serializer& s) const
{
  s.attribute ("lat", lat_);
  s.attribute ("lon", lon_);
}

// object
//
object::
object (parser& p)
{
  // Note that for the root of the object model we parse the start/end
  // element ourselves instead of expecting the caller to do so. This
  // makes the client code nice and simple.
  //
  p.next_expect (parser::start_element, "object", content::complex);

  // Because of the above special case, this constructor is called
  // without start_element yet being parsed. As a result, we cannot
  // parse attributes or nested elements in the initializer list.
  //
  name_ = p.attribute ("name");
  type_ = p.attribute<object_type> ("type");
  id_ = p.attribute<unsigned int> ("id");

  do
  {
    p.next_expect (parser::start_element, "position");
    positions_.push_back (position (p));
    p.next_expect (parser::end_element);

  } while (p.peek () == parser::start_element);

  p.next_expect (xml::parser::end_element); // object
}

void object::
serialize (serializer& s) const
{
  // Note that for the root of the object model we serialize the
  // start/end element ourselves instead of expecting the caller
  // to do so. This makes the client code nice and simple.
  //
  s.start_element ("object");

  s.attribute ("name", name_);
  s.attribute ("type", type_);
  s.attribute ("id", id_);

  for (positions_type::const_iterator i (positions_.begin ());
       i != positions_.end ();
       ++i)
  {
    s.start_element ("position");
    i->serialize (s);
    s.end_element ();
  }

  s.end_element (); // object
}

// object_type
//
ostream&
operator<< (ostream& os, object_type x)
{
  if (x == building)
    os << "building";
  else
    os << "mountain";

  return os;
}

istream&
operator>> (istream& is, object_type& x)
{
  string s;
  if (is >> s)
  {
    if (s == "building")
      x = building;
    else if (s == "mountain")
      x = mountain;
    else
      is.setstate (istream::failbit);
  }

  return is;
}
