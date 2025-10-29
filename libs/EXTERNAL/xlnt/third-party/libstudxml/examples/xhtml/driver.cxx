// file      : examples/xhtml/driver.cxx
// copyright : not copyrighted - public domain

#include <iostream>

#include <libstudxml/serializer.hxx>

using namespace std;
using namespace xml;

namespace xhtml
{
  // "Canonical" XHTML5 vocabulary.
  //
  const char* xmlns = "http://www.w3.org/1999/xhtml";

  inline void _html (serializer& s)
  {
    s.doctype_decl ("html");
    s.start_element (xmlns, "html");
    s.namespace_decl (xmlns, "");
  }
  inline void html_ (serializer& s) {s.end_element (xmlns, "html");}

  inline void _head (serializer& s)
  {
    s.start_element (xmlns, "head");
    s.start_element (xmlns, "meta");
    s.attribute ("charset", "UTF-8");
    s.end_element ();
  }
  inline void head_ (serializer& s) {s.end_element (xmlns, "head");}

  inline void _title (serializer& s) {s.start_element (xmlns, "title");}
  inline void title_ (serializer& s) {s.end_element (xmlns, "title");}

  inline void _body (serializer& s) {s.start_element (xmlns, "body");}
  inline void body_ (serializer& s) {s.end_element (xmlns, "body");}

  inline void _p (serializer& s) {s.start_element (xmlns, "p");}
  inline void p_ (serializer& s) {s.end_element (xmlns, "p");}

  // "Inline" elements, i.e., those that are written without
  // indentation.
  //
  inline void _em (serializer& s)
  {
    s.suspend_indentation ();
    s.start_element (xmlns, "em");
  }
  inline void em_ (serializer& s)
  {
    s.end_element (xmlns, "em");
    s.resume_indentation ();
  }

  inline void _br_ (serializer& s)
  {
    s.suspend_indentation ();
    s.start_element (xmlns, "br");
    s.end_element ();
    s.resume_indentation ();
  }

  // Attributes.
  //
  template <typename T>
  struct attr_value
  {
    attr_value (const char* n, const T& v): name (n), value (v) {}

    void operator() (serializer& s) const {s.attribute (name, value);}
    const char* name;
    const T& value;
  };

  struct attr
  {
    const char* name;

    explicit
    attr (const char* n): name (n) {}

    // s << (attr = 123);
    //
    template <typename T>
    attr_value<T> operator= (const T& v) {return attr_value<T> (name, v);}

    // s << attr (123);
    //
    template <typename T>
    attr_value<T> operator() (const T& v) {return attr_value<T> (name, v);}

    // s << attr << 123 << ~attr;
    //
    typedef void serialize_function (serializer&);

    void operator() (serializer& s) const {s.start_attribute (name);}
    serialize_function* operator~ () const {return &end;}

    static void end (serializer& s) {s.end_attribute ();}
  };

  static attr id ("id");
}

int
main ()
{
  try
  {
    using namespace xhtml;

    serializer s (cout, "output");

    s << _html
      <<   _head
      <<     _title << "Example XHTML5 document" << title_
      <<   head_
      <<   _body << (id = 123)
      <<     _p << "Here be " << _em << "Dragons!" << em_ << _br_
      <<        "And " << 123 << p_
      <<   body_
      << html_;
  }
  catch (const xml::serialization& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
