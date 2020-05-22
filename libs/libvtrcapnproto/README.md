Capnproto usage in VTR
======================

Capnproto is a data serialization framework designed for portabliity and speed.
In VPR, capnproto is used to provide binary formats for internal data
structures that can be computed once, and used many times.  Specific examples:
 - rrgraph
 - Router lookahead data
 - Place matrix delay estimates

What is capnproto?
==================

capnproto can be broken down into 3 parts:
 - A schema language
 - A code generator
 - A library

The schema language is used to define messages.  Each message must have an
explcit capnproto schema, which are stored in files suffixed with ".capnp".
The capnproto documentation for how to write these schema files can be found
here: https://capnproto.org/language.html

The schema by itself is not especially useful.  In order to read and write
messages defined by the schema in a target language (e.g. C++), a code
generation step is required.  Capnproto provides a cmake function for this
purpose, `capnp_generate_cpp`.  This generates C++ source and header files.
These source and header files combined with the capnproto C++ library, enables
C++ code to read and write the messages matching a particular schema.  The C++
library API can be found here: https://capnproto.org/cxx.html

Contents of libvtrcapnproto
===========================

libvtrcapnproto should contain two elements:
 - Utilities for working capnproto messages in VTR
 - Generate source and header files of all capnproto messages used in VTR

I/O Utilities
-------------

Capnproto does not provide IO support, instead it works from arrays (or file
descriptors).  To avoid re-writing this code, libvtrcapnproto provides two
utilities that should be used whenever reading or writing capnproto message to
disk:
 - `serdes_utils.h` provides the writeMessageToFile function - Writes a
   capnproto message to disk.
 - `mmap_file.h` provides MmapFile object - Maps a capnproto message from the
   disk as a flat array.

NdMatrix Utilities
------------------

A common datatype which appears in many data structures that VPR might want to
serialize is the generic type `vtr::NdMatrix`.  `ndmatrix_serdes.h` provides
generic functions ToNdMatrix and FromNdMatrix, which can be used to generically
convert between the provideid capnproto message `Matrix` and `vtr::NdMatrix`.

Capnproto schemas
-----------------

libvtrcapnproto should contain all capnproto schema definitions used within
VTR.  To add a new schema:
1. Add the schema to git in `libs/libvtrcapnproto/`
2. Add the schema file name to `capnp_generate_cpp` invocation in
   `libs/libvtrcapnproto/CMakeLists.txt`.

The schema will be available in the header file `schema filename>.h`.  The
actual header file will appear in the CMake build directory
`libs/libvtrcapnproto` after `libvtrcapnproto` has been rebuilt.

Writing capnproto binary files to text
======================================

The `capnp` tool (found in the CMake build directiory
`libs/EXTERNAL/capnproto/c++/src/capnp`) can be used to convert from a binary
capnp message to a textual form.

Example converting VprOverrideDelayModel from binary to text:

```
capnp convert binary:text place_delay_model.capnp VprOverrideDelayModel \
  < place_delay.bin > place_delay.txt
```
