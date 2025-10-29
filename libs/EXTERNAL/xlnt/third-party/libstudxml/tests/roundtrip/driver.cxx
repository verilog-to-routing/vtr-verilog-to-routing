// file      : tests/roundtrip/driver.cxx
// license   : MIT; see accompanying LICENSE file

#include <string>
#include <fstream>
#include <iostream>

#include <libstudxml/parser.hxx>
#include <libstudxml/serializer.hxx>

#undef NDEBUG
#include <cassert>

using namespace std;
using namespace xml;

const bool trace = false;

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
    ifstream ifs;
    ifs.exceptions (ifstream::badbit | ifstream::failbit);
    ifs.open (argv[1], ifstream::in | ifstream::binary);

    parser p (ifs,
              argv[1],
              parser::receive_default |
              parser::receive_attributes_event |
              parser::receive_namespace_decls);

    serializer s (cout, "out", 0);

    bool in_attr (false);
    for (parser::event_type e (p.next ()); e != parser::eof; e = p.next ())
    {
      switch (e)
      {
      case parser::start_element:
        {
          if (trace)
            cerr << p.line () << ':' << p.column () << ": " << e << " "
                 << p.namespace_() << (p.namespace_().empty () ? "" : "#")
                 << p.prefix () << (p.prefix ().empty () ? "" : ":")
                 << p.name () << endl;

          s.start_element (p.qname ());
          break;
        }
      case parser::end_element:
        {
          if (trace)
            cerr << p.line () << ':' << p.column () << ": " << e << " "
                 << p.namespace_() << (p.namespace_().empty () ? "" : "#")
                 << p.prefix () << (p.prefix ().empty () ? "" : ":")
                 << p.name () << endl;

          s.end_element ();
          break;
        }
      case parser::start_namespace_decl:
        {
          if (trace)
            cerr << "  " << p.prefix () << "->" << p.namespace_ () << endl;

          s.namespace_decl (p.namespace_ (), p.prefix ());
          break;
        }
      case parser::end_namespace_decl:
        {
          if (trace)
            cerr << "  " << p.prefix () << "-x" << endl;

          break;
        }
      case parser::start_attribute:
        {
          if (trace)
            cerr << "  " << p.qname () << "=";

          s.start_attribute (p.qname ());
          in_attr = true;
          break;
        }
      case parser::end_attribute:
        {
          s.end_attribute ();
          in_attr = false;
          break;
        }
      case parser::characters:
        {
          if (trace)
          {
            if (!in_attr)
              cerr << p.line () << ':' << p.column () << ": " << e << " ";

            cerr << "'" << p.value () << "'" << endl;
          }

          s.characters (p.value ());
          break;
        }
      default:
        break;
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
