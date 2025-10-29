// file      : examples/hybrid/dom.cxx
// copyright : not copyrighted - public domain

#include <libstudxml/parser.hxx>
#include <libstudxml/serializer.hxx>

#include "dom.hxx"

using namespace std;
using namespace xml;

static bool
whitespace (const string& s)
{
  for (string::size_type i (0); i < s.size (); ++i)
  {
    char c (s[i]);
    if (c != 0x20 && c != 0x0A && c != 0x0D && c != 0x09)
      return false;
  }

  return true;
}

element::
element (parser& p, bool se)
{
  if (se)
    p.next_expect (parser::start_element);

  name_ = p.qname ();

  // Extract attributes.
  //
  const parser::attribute_map_type& m (p.attribute_map ());
  for (parser::attribute_map_type::const_iterator i (m.begin ());
       i != m.end ();
       ++i)
    attributes_[i->first] = i->second.value;

  // Parse content (nested elements or text).
  //
  while (p.peek () != parser::end_element)
  {
    switch (p.next ())
    {
    case parser::start_element:
      {
        if (!text_.empty ())
        {
          if (!whitespace (text_))
            throw parsing (p, "element in simple content");

          text_.clear ();
        }

        elements_.push_back (element (p, false));
        p.next_expect (parser::end_element);

        break;
      }
    case parser::characters:
      {
        if (!elements_.empty ())
        {
          if (!whitespace (p.value ()))
            throw parsing (p, "characters in complex content");

          break; // Ignore whitespaces.
        }

        text_ += p.value ();
        break;
      }
    default:
      break; // Ignore any other events.
    }
  }

  if (se)
    p.next_expect (parser::end_element);
}

void element::
serialize (serializer& s, bool se) const
{
  if (se)
    s.start_element (name_);

  // Add attributes.
  //
  for (attributes_type::const_iterator i (attributes_.begin ());
       i != attributes_.end ();
       ++i)
  {
    s.attribute (i->first, i->second);
  }

  // Serialize content (nested elements or text).
  //
  if (!elements_.empty ())
  {
    for (elements_type::const_iterator i (elements_.begin ());
         i != elements_.end ();
         ++i)
    {
      i->serialize (s);
    }
  }
  else if (!text_.empty ())
    s.characters (text_);

  if (se)
    s.end_element ();
}
