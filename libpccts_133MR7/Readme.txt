+-------------------+
|  libpccts_133MR7  |
+-------------------+

This directory contains the Purdue Compiler Construction Tool Set C++
library code for Toro (Linux version).

To build on Linux RedHat...

   1. "setenv ISP linux_x86_64"
   2. "cd h"
   3. "make"
   4. "cp libh.a ../linux_x86_64"
   5. "cd .."
   6. "cd pcctsGeneric"
   7. "make"
   8. "cp libpcctsGeneric.a ../linux_x86_64"
   9. "cd .."

To build on Linux ubuntu...

   1. "export ISP=linux_i686"
   2. "cd h"
   3. "make"
   4. "cp libh.a ../linux_i686"
   5. "cd .."
   6. "cd pcctsGeneric"
   7. "make"
   8. "cp libpcctsGeneric.a ../linux_i686"
   9. "cd .."
