// file      : examples/roundtrip/driver.cxx
// copyright : not copyrighted - public domain

#include <string>
#include <fstream>
#include <iostream>

#include <libstudxml/parser.hxx>
#include <libstudxml/serializer.hxx>

using namespace std;
using namespace xml;

int
main (int argc, char* argv[])
{
  if (argc != 2)
  {
    cerr << "usage: " << argv[0] << " <xml-file>" << endl;
    return 1;
  }

  try
  {
    // Enable stream exceptions so that io failures are reported
    // as stream rather than as parsing exceptions.
    //
    ifstream ifs;
    ifs.exceptions (ifstream::badbit | ifstream::failbit);
    ifs.open (argv[1], ifstream::in | ifstream::binary);

    // Configure the parser to receive attributes as events as well
    // as to receive prefix-namespace mappings (namespace declarations
    // in XML terminology).
    //
    parser p (ifs,
              argv[1],
              parser::receive_default |
              parser::receive_attributes_event |
              parser::receive_namespace_decls);

    // Configure serializer not to perform indentation. Existing
    // indentation, if any, will be preserved.
    //
    serializer s (cout, "out", 0);

    for (parser::event_type e (p.next ()); e != parser::eof; e = p.next ())
    {
      switch (e)
      {
      case parser::start_element:
        {
          s.start_element (p.qname ());
          break;
        }
      case parser::end_element:
        {
          s.end_element ();
          break;
        }
      case parser::start_namespace_decl:
        {
          s.namespace_decl (p.namespace_ (), p.prefix ());
          break;
        }
      case parser::end_namespace_decl:
        {
          // There is nothing in XML that indicates the end of namespace
          // declaration since it is scope-based.
          //
          break;
        }
      case parser::start_attribute:
        {
          s.start_attribute (p.qname ());
          break;
        }
      case parser::end_attribute:
        {
          s.end_attribute ();
          break;
        }
      case parser::characters:
        {
          s.characters (p.value ());
          break;
        }
      case parser::eof:
        {
          // Handled in the for loop.
          //
          break;
        }
      }
    }
  }
  catch (const ios_base::failure&)
  {
    cerr << "io failure" << endl;
    return 1;
  }
  catch (const xml::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
