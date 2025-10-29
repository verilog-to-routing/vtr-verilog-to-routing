// file      : examples/performance/expat.cxx
// license   : not copyrighted - public domain

#include <iostream>
#include <fstream>

#include <expat.h>

#include "time.hxx"

using namespace std;

const unsigned long iterations = 1000;

unsigned long start_count;
unsigned long end_count;

void XMLCALL
start_element (void* data, const XML_Char* ns_name, const XML_Char** atts)
{
  start_count++;
}

void XMLCALL
end_element (void* data, const XML_Char* ns_name)
{
  end_count++;
}

void XMLCALL
characters (void* data, const XML_Char* s, int n)
{
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
    ifstream ifs;
    ifs.exceptions (ios_base::failbit);
    ifs.open (argv[1], ios::in | ios::ate);

    size_t size (ifs.tellg ());
    ifs.seekg (0, ios::beg);

    char* buf = new char[size];
    ifs.read (buf, size);

    cerr << "  document size:  " << size << " bytes" << endl;

    // Warmup.
    //
    bool failed (false);
    for (unsigned long i (0); !failed && i < 10; ++i)
    {
      start_count = 0;
      end_count = 0;

      XML_Parser p (XML_ParserCreateNS (0, ' '));
      XML_SetStartElementHandler (p, start_element);
      XML_SetEndElementHandler (p, end_element);
      XML_SetCharacterDataHandler (p, characters);
      XML_Parse (p, buf, size, 1);
      XML_ParserFree (p);

      if (start_count != end_count)
        failed = true;
    }

    if (failed)
    {
      cerr << "failed" << endl;
      return 1;
    }

    cerr << "  elements:       " << start_count << endl;

    os::time start;

    for (unsigned long i (0); !failed && i < 1000; ++i)
    {
      start_count = 0;
      end_count = 0;

      XML_Parser p (XML_ParserCreateNS (0, ' '));
      XML_SetStartElementHandler (p, start_element);
      XML_SetEndElementHandler (p, end_element);
      XML_SetCharacterDataHandler (p, characters);
      XML_Parse (p, buf, size, 1);
      XML_ParserFree (p);

      if (start_count != end_count)
        failed = true;
    }

    os::time end;
    delete[] buf;

    if (failed)
    {
      cerr << "failed" << endl;
      return 1;
    }

    os::time time (end - start);
    double ms (time.sec () * 1000000ULL + time.nsec () / 1000ULL);

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
  catch (ios_base::failure const&)
  {
    cerr << "io failure" << endl;
    return 1;
  }
}
