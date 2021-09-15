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
 * @file This file includes the definition of the basic structure 
 * used in Odin-II BLIF class to parse a Verilog file. Moreover, 
 * it provides the declaration of Verilog class routines
 */

#ifndef __VERILOG_H__
#define __VERILOG_H__

#include "GenericReader.hpp"
#include "GenericWriter.hpp"

#include "ast_util.h"

/**
 * @brief A class to provide the general object of an input Verilog file reader
 */
class Verilog {
  public:
    /**
     * @brief Construct the object
     * required by compiler
     */
    Verilog();
    /**
     * @brief Destruct the object
     * to avoid memory leakage
     */
    ~Verilog();

    class Reader : public GenericReader {
      public:
        /**
         * @brief Construct the Reader object
         * required by compiler
         */
        Reader();
        /**
         * @brief Destruct the Reader object
         * to avoid memory leakage
         */
        ~Reader();

        void* _read();

        /* No need to have writer in Generic Reader */
        void _write(const netlist_t* /* netlist */) {
            error_message(UTIL, unknown_location, "%s is not available in Generic Reader\n", __PRETTY_FUNCTION__);
        }

      protected:
      private:
    };

    class Writer : public GenericWriter {
      public:
        /**
         * @brief Construct the Writer object
         * required by compiler
         */
        Writer();
        /**
         * @brief Destruct the Writer object
         * to avoid memory leakage
         */
        ~Writer();

        /* No need to have reader in Generic Writer */
        void* __read() {
            error_message(UTIL, unknown_location, "%s is not available in Generic Writer\n", __PRETTY_FUNCTION__);
            return NULL;
        }

        void _write(const netlist_t* netlist);
        void _create_file(const file_type_e file_type);

      protected:
    };
};

#endif //__VERILOG_H__
