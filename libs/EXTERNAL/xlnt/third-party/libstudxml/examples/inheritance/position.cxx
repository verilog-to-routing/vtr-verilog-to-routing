// file      : examples/inheritance/position.cxx
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
    : name_ (p.attribute ("name")),
      id_ (p.attribute<unsigned int> ("id"))
{
  p.content (content::complex);

  do
  {
    p.next_expect (parser::start_element, "position");
    positions_.push_back (position (p));
    p.next_expect (parser::end_element);

    // Note that here we have to also check the name to make sure
    // it is the position element and not something that should be
    // handled by a derived class.
    //
  } while (p.peek () == parser::start_element && p.name () == "position");
}

void object::
serialize_attributes (serializer& s) const
{
  s.attribute ("name", name_);
  s.attribute ("id", id_);
}

void object::
serialize_content (serializer& s) const
{
  for (positions_type::const_iterator i (positions_.begin ());
       i != positions_.end ();
       ++i)
  {
    s.start_element ("position");
    i->serialize (s);
    s.end_element ();
  }
}

// elevation
//
elevation::
elevation (parser& p)
    : value_ (p.attribute<float> ("val"))
{
  p.content (content::empty);
}

void elevation::
serialize (serializer& s) const
{
  s.attribute ("val", value_);
}

// object
//
elevated_object::
elevated_object (parser& p)
    : object (p), // First parse our base.
      units_ (p.attribute ("units"))
{
  // If we are happy with the content model used by our base (complex
  // in this case), then we don't need to set one ourselves. We, could,
  // however "upgrade" our content model from empty to either simple or
  // complex.

  do
  {
    p.next_expect (parser::start_element, "elevation");
    elevations_.push_back (elevation (p));
    p.next_expect (parser::end_element);

  } while (p.peek () == parser::start_element && p.name () == "elevation");
}

void elevated_object::
serialize_attributes (serializer& s) const
{
  object::serialize_attributes (s); // First serialize our base attributes.

  s.attribute ("units", units_);
}

void elevated_object::
serialize_content (serializer& s) const
{
  object::serialize_content (s); // First serialize our base content.

  for (elevations_type::const_iterator i (elevations_.begin ());
       i != elevations_.end ();
       ++i)
  {
    s.start_element ("elevation");
    i->serialize (s);
    s.end_element ();
  }
}

// objects
//
objects::
objects (parser& p)
{
  // Note that for the root of the object model we parse the start/end
  // element ourselves instead of expecting the caller to do so. This
  // makes the client code nice and simple.
  //
  p.next_expect (parser::start_element, "objects", content::complex);

  // First parse all the objects, if any.
  //
  while (p.peek () == parser::start_element && p.name () == "object")
  {
    p.next (); // "Swallow" the start_element event.
    simple_objects_.push_back (object (p));
    p.next_expect (parser::end_element);
  }

  // Then parse all the elevated object, if any.
  //
  while (p.peek () == parser::start_element && p.name () == "elevated-object")
  {
    p.next (); // "Swallow" the start_element event.
    elevated_objects_.push_back (elevated_object (p));
    p.next_expect (parser::end_element);
  }

  // We should have at least one object.
  //
  if (simple_objects_.empty () && elevated_objects_.empty ())
    throw parsing (p, "at least one object or elevated object required");

  p.next_expect (xml::parser::end_element); // objects
}

void objects::
serialize (serializer& s) const
{
  // Note that for the root of the object model we serialize the
  // start/end element ourselves instead of expecting the caller
  // to do so. This makes the client code nice and simple.
  //
  s.start_element ("objects");

  // First serialize all the objects.
  //
  for (simple_objects_type::const_iterator i (simple_objects_.begin ());
       i != simple_objects_.end ();
       ++i)
  {
    s.start_element ("object");
    i->serialize (s);
    s.end_element ();
  }

  // Then serialize all the elevated objects.
  //
  for (elevated_objects_type::const_iterator i (elevated_objects_.begin ());
       i != elevated_objects_.end ();
       ++i)
  {
    s.start_element ("elevated-object");
    i->serialize (s);
    s.end_element ();
  }

  s.end_element (); // objects
}
