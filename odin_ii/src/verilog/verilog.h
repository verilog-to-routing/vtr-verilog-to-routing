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

#ifndef __verilog_H__
#define __verilog_H__

#include "generic_reader.h"
#include "generic_writer.h"
#include "ast_util.h"

#define MODULE "module"

/**
 * @brief A class to provide the general object of an input verilog file reader
 */
class verilog {
  public:
    /**
     * @brief Construct the object
     * required by compiler
     */
    verilog();
    /**
     * @brief Destruct the object
     * to avoid memory leakage
     */
    ~verilog();

    class reader : public generic_reader {
      public:
        /**
         * @brief Construct the reader object
         * required by compiler
         */
        reader();
        /**
         * @brief Destruct the reader object
         * to avoid memory leakage
         */
        ~reader();

        void* _read();

        /* No need to have writer in Generic reader */
        void _write(const netlist_t* /* netlist */) {
            error_message(UTIL, unknown_location, "%s is not available in Generic reader\n", __PRETTY_FUNCTION__);
        }

      protected:
      private:
    };

    class writer : public generic_writer {
      public:
        /**
         * @brief Construct the writer object
         * required by compiler
         */
        writer();
        /**
         * @brief Destruct the writer object
         * to avoid memory leakage
         */
        ~writer();

        /* No need to have reader in Generic writer */
        void* __read() {
            error_message(UTIL, unknown_location, "%s is not available in Generic writer\n", __PRETTY_FUNCTION__);
            return NULL;
        }

        void _write(const netlist_t* netlist);
        void _create_file(const char* file_name, const file_type_e file_type = VERILOG);

      protected:
        STRING_CACHE* models_declaration;

        /**
         *-------------------------------------------------------------------------------------------
         * (function: create_verilog)
         * 
         * @brief initiate a new output file stream
         * 
         * @param file_name the path to the verilog file
         * 
         * @return a FILE pointer to the verilog file
         *-------------------------------------------------------------------------------------------
         */
        FILE* create_verilog(const char* file_name);
    };
};

#endif //__verilog_H__
