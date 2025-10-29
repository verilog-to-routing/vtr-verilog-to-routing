// file      : examples/processing/driver.cxx
// copyright : not copyrighted - public domain

#include <string>
#include <fstream>
#include <iostream>

#include <libstudxml/parser.hxx>
#include <libstudxml/serializer.hxx>
#include <libstudxml/value-traits.hxx>

using namespace std;
using namespace xml;

enum object_type {building, mountain};

// Specialization of xml::value_traits for object_type. This mechanism is
// used to implements XML-specific value type parsing and serialization.
// By default the parser and serializer fall back onto iostream insertion
// and extraction operators.
//
// This code also shows how we can reuse the parsing exception to implement
// our own diagnostics.
//
namespace xml
{
  template <>
  struct value_traits<object_type>
  {
    static object_type
    parse (string s, const parser& p)
    {
      if (s == "building")
        return building;
      else if (s == "mountain")
        return mountain;
      else
        throw parsing (p, "invalid object type '" + s + "'");
    }

    static string
    serialize (object_type x, const serializer&)
    {
      if (x == building)
        return "building";
      else
        return "mountain";
    }
  };
}

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
    // Parse the input document and compute the average object position.
    //
    ifstream ifs (argv[1]);
    parser p (ifs, argv[1]);

    p.next_expect (parser::start_element, "object", content::complex);

    unsigned int id (p.attribute<unsigned int> ("id"));
    string name (p.element ("name"));
    object_type type (p.element<object_type> ("type"));

    float alat (0), alon (0);
    unsigned int count (0);

    do
    {
      p.next_expect (parser::start_element, "position", content::empty);

      float lat (p.attribute<float> ("lat"));
      float lon (p.attribute<float> ("lon"));

      p.next_expect (parser::end_element); // position

      alat += lat;
      alon += lon;
      count++;

    } while (p.peek () == parser::start_element);

    // Here is another example of re-using the parsing exception to
    // implement application-level diagnostics. Every object in our
    // vocabulary should have at least two position samples.
    //
    if (count < 2)
      throw parsing (p, "at least two position samples required");

    alat /= count;
    alon /= count;

    p.next_expect (parser::end_element); // object

    // Serialize an XML document with simulated positions based on the
    // average we just calculated.
    //
    serializer s (cout, "output");
    s.start_element ("object");

    s.attribute ("id", id);
    s.element ("name", name);
    s.element ("type", type);

    for (unsigned int i (0); i < 2; ++i)
    {
      for (unsigned int j (0); j < 2; ++j)
      {
        s.start_element ("position");

        float lat (alat + (i % 2 ? 0.0001F : -0.0001F));
        float lon (alon + (j % 2 ? -0.0001F : 0.0001F));

        s.attribute ("lat", lat);
        s.attribute ("lon", lon);

        s.end_element (); // position
      }
    }

    s.end_element (); // object
  }
  // This handler will handle both parsing (xml::parsing) and serialization
  // (xml::serialization) exceptions.
  //
  catch (const xml::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
