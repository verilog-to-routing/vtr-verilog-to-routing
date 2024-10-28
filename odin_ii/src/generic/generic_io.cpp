/*
 * Copyright 2023 CAS—Atlantic (University of New Brunswick, CASA)
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "generic_io.h"
#include "odin_error.h"

generic_io::generic_io() = default;

generic_io::~generic_io() = default;

void* generic_io::_read() {
    error_message(UTIL, unknown_location,
                  "Function \"%s\" is called for reading the input file without definition provided!\n",
                  __PRETTY_FUNCTION__);
    return NULL;
}

void generic_io::_write(const netlist_t* /* netlist */) {
    error_message(UTIL, unknown_location,
                  "Function \"%s\" is called for reading the input file without definition provided!\n",
                  __PRETTY_FUNCTION__);
}

void generic_io::_create_file(const char* /* file_name */, const file_type_e /* file_type */) {
    error_message(UTIL, unknown_location,
                  "Function \"%s\" is called for reading the input file without definition provided!\n",
                  __PRETTY_FUNCTION__);
}
