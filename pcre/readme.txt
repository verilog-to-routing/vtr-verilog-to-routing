PERL-Compatible Regular Expresion Library (PCRE) is an open source project that provides a regular expression library for C/C++ programs.
The regular expression it implements follows PERL syntax/semantics.

The library is written by: Philip Hazel <ph10@cam.ac.uk> from the University of Cambridge

I (Jason Luu) have modified the pcre library so that it fits into our VTR environment.

To build the static library (libpcre.a), enter "make".

To test if the library works after you build it, enter:
	pcredemo "[aA]b.*c" "My Abacus"

This should return "Match succeeded at offset 3"

