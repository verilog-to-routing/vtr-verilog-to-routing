/*
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

#include "GenericIO.hh"
#include "odin_error.h"

GenericIO::GenericIO() = default;

GenericIO::~GenericIO() = default;

void* GenericIO::__read() {
    error_message(UTIL, unknown_location, 
                 "Function \"%s\" is called for reading the input file without definition provided!\n", __PRETTY_FUNCTION__); 
    return NULL;     
}

void GenericIO::__write (const netlist_t* /* netlist */, FILE* /* output_file */) {
    error_message(UTIL, unknown_location, 
                 "Function \"%s\" is called for reading the input file without definition provided!\n", __PRETTY_FUNCTION__); 
}
