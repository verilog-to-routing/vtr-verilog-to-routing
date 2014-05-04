+-----------------+
|  libcommon_c++  |
+-----------------+

This directory contains C++ library code that is common to VPR and Toro.

The library source code enables a number of Toro-specific features
within VPR. These features include:

   - Fabric descriptions (blocks, switchboxes, connection blocks)
   - Relative placement macros
   - Region-based placement
   - Pre-placed blocks
   - Pre-routed nets

Although VPR needs to link this common C++ library when building, the
various features enabled by this library are currently only accessible
when using Toro.

To build on Linux RedHat...

   1. "setenv BUILD_TYPE release"
      -or-
      "setenv BUILD_TYPE debug"
   2. "make"

To build on Linux ubuntu...

   1. "export BUILD_TYPE=release"
      -or-
      "export BUILD_TYPE=debug"
   2. "make"

To build on Windows VisualC++...

   1. run VisualC++ (2010 or 2012)
   2. open "libcommon_c++.vcxproj"
   3. build
