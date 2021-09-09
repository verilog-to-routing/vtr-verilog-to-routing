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
 * @file: Odin-II Generic Reader class declaration
 */

#ifndef __GENERIC_READER_H__
#define __GENERIC_READER_H__

#include "GenericIO.hpp"
#include <string>

/**
 * @brief A class to provide the general object of an input file reader
 */
class GenericReader : public GenericIO {
  public:
    /**
     * @brief Construct the GenericReader object
     * required by compiler
     */
    GenericReader();
    /**
     * @brief Destruct the GenericReader object
     * to avoid memory leakage
     */
    ~GenericReader();

    void* _read();
    void* read_verilog();
    void* read_blif();
    /**
     * [TODO]
     * void* read_systemverilog();
     * void* read_ilang(); 
     */

    /* No need to have writer in Generic Reader */
    void _write(const netlist_t* /* netlist */) {
        error_message(UTIL, unknown_location, "%s is not available in Generic Reader\n", __PRETTY_FUNCTION__);
    }

  private:
    GenericReader* verilog_reader;
    GenericReader* blif_reader;
    /**
     * [TODO]
     * GenericReader* systemverilog_reader;
     * GenericReader* ilang_reader;
     */
};

#endif //__GENERIC_READER_H__
