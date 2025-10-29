// file      : examples/performance/driver.cxx
// copyright : not copyrighted - public domain

#include <string>
#include <fstream>
#include <iostream>

#include <libstudxml/parser.hxx>

#include "time.hxx"

#undef NDEBUG
#include <cassert>

using namespace std;
using namespace xml;

const unsigned long iterations = 1000;

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
    ifs.exceptions (ios_base::failbit);
    ifs.open (argv[1], ios::in | ios::ate);

    size_t size (static_cast<size_t> (ifs.tellg ()));
    ifs.seekg (0, ios::beg);

    char* buf = new char[size];
    ifs.read (buf, size);

    cerr << "  document size:  " << size << " bytes" << endl;

    unsigned long start_count;
    unsigned long end_count;

    // Warmup.
    //
    for (unsigned long i (0); i < 10; ++i)
    {
      start_count = 0;
      end_count = 0;

      // Configure the parser to receive attributes as events without
      // creating the attribute map.
      //
      parser p (buf,
                size,
                argv[1],
                parser::receive_default | parser::receive_attributes_event);

      for (parser::event_type e (p.next ()); e != parser::eof; e = p.next ())
      {
        switch (e)
        {
        case parser::start_element:
          start_count++;
          break;
        case parser::end_element:
          end_count++;
          break;
        default:
          break;
        }
      }

      assert (start_count == end_count);
    }

    cerr << "  elements:       " << start_count << endl;

    os::time start;

    for (unsigned long i (0); i < iterations; ++i)
    {
      start_count = 0;
      end_count = 0;

      // Configure the parser to receive attributes as events without
      // creating the attribute map.
      //
      parser p (buf,
                size,
                argv[1],
                parser::receive_default | parser::receive_attributes_event);

      for (parser::event_type e (p.next ()); e != parser::eof; e = p.next ())
      {
        switch (e)
        {
        case parser::start_element:
          start_count++;
          break;
        case parser::end_element:
          end_count++;
          break;
        default:
          break;
        }
      }

      assert (start_count == end_count);
    }

    os::time end;

    delete[] buf;

    os::time time (end - start);
    double ms (
      static_cast<double> (
        time.sec () * 1000000ULL + time.nsec () / 1000ULL));

    cerr << "  time:           " << time << " sec" << endl;

    // Calculate throughput in documents/sec.
    //
    double tpd ((iterations / ms) * 1000000);
    cerr << "  throughput:     " << tpd << " documents/sec" << endl;

    // Calculate throughput in MBytes/sec.
    //
    double tpb (((size * iterations) / ms) * 1000000/(1024*1024));
    cerr << "  throughput:     " << tpb << " MBytes/sec" << endl;
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
