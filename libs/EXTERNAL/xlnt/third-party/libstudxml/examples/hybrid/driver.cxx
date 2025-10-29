// file      : examples/hybrid/driver.cxx
// copyright : not copyrighted - public domain

#include <fstream>
#include <iostream>

#include <libstudxml/parser.hxx>
#include <libstudxml/serializer.hxx>

#include "dom.hxx"

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
    // Parse the input document and construct an in-memory, DOM-like
    // raw XML representation for each object element. Also serialize
    // the data back to XML as we parse.
    //
    ifstream ifs (argv[1]);
    parser p (ifs, argv[1]);
    serializer s (cout, "output");

    p.next_expect (parser::start_element, "objects", content::complex);
    s.start_element ("objects");

    while (p.peek () == parser::start_element)
    {
      element e (p);
      e.serialize (s);
    }

    p.next_expect (parser::end_element); // objects
    s.end_element ();
  }
  catch (const xml::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
