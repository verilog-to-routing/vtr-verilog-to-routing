+---------------+
|  libcommon_c  |
+---------------+

This directory contains C library code that is common to VPR and Toro.

The library source code is currently limited to "pcre", a Perl-based
regular expression library.

To build on Linux RedHat...

   1. "setenv BUILD_TYPE release"
      -or-
      "setenv BUILD_TYPE debug"
   2. "cd pcre"
   3. "make"
   4. "cd .."

To build on Linux ubuntu...

   1. "export BUILD_TYPE=release"
      -or-
      "export BUILD_TYPE=debug"
   2. "cd pcre"
   3. "make"
   4. "cd .."

To build on Windows VisualC++...

   1. run VisualC++ (2010 or 2012)
   2. open "pcre/libpcre.vcxproj"
   3. build
