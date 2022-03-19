/**
 * Copyright (c) 2021 Seyed Alireza Damghani (sdamghann@gmail.com)
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
 *
 * @file: Odin-II Generic IO abstract class declaration
 */

#ifndef __GENERIC_IO_H__
#define __GENERIC_IO_H__

#include "odin_types.h"

/**
 * @brief A class to provide the general object of an input file reader
 */
class GenericIO {
  public:
    /**
     * @brief Construct the GenericIO object
     * required by compiler
     */
    GenericIO();
    /**
     * @brief Destruct the GenericIO object
     * to avoid memory leakage
     */
    virtual ~GenericIO();

    virtual void* _read();
    virtual void _write(const netlist_t* netlist);

    /* to create the output file */
    virtual void _create_file(const char* file_name, const file_type_e file_type = _UNDEFINED);
};

#endif // __GENERIC_IO_H__
