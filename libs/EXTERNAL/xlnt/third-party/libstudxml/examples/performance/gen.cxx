#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;

static const char* enums[] =
{
  "romance",
  "fiction",
  "horror",
  "history",
  "philosophy"
};

int
main (int argc, char* argv[])
{
  if (argc != 3)
  {
    cerr << "usage: " << argv[0] << " <count> <output-file>" << endl;
    return 1;
  }

  unsigned long n (0);
  istringstream is (argv[1]);
  is >> n;

  if (n == 0)
  {
    cerr << "record count argument should be a positive number" << endl;
    return 1;
  }

  ofstream ofs (argv[2]);

  if (!ofs.is_open ())
  {
    cerr << "unable to open '" << argv[2] << "' in write mode" << endl;
    return 1;
  }

  ofs << "<t:root xmlns:t='test' " <<
    "xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' " <<
    "xsi:schemaLocation='test test.xsd'>";

  unsigned short ch (1), en (0);

  for (unsigned long i (0); i < n; ++i)
  {
    ofs << "<record orange=\"" << i << "\"";

    if (i % 2 == 0)
      ofs << " apple=\"true\"";

    ofs << ">"
        << "<int>42</int>"
        << "<double>42345.4232</double>"
        << "<name>name123_45</name>";

    if (i % 2 == 1)
      ofs << "<string>one two three</string>";

    ofs << "<choice" << ch << ">" << ch << " choice</choice" << ch << ">"
        << "<enum>" << enums[en] << "</enum>"
        << "</record>";

    if (++ch > 4)
      ch = 1;

    if (++en > 4)
      en = 0;
  }

  ofs << "</t:root>";
}
