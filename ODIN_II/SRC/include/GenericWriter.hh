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

#ifndef __GENERIC_WRITER_H__
#define __GENERIC_WRITER_H__

#include "GenericIO.hh"

/**
 * @brief A class to provide the general object of an input file reader
*/
class GenericWriter : public GenericIO {

    public:
        /**
         * @brief Construct the GenericWriter object
         * required by compiler
         */
        GenericWriter();
        /**
         * @brief Destruct the GenericWriter object
         * to avoid memory leakage
         */
        ~GenericWriter();

        /* No need to have reader in Generic Writer */
        void* __read() {
            error_message(UTIL, unknown_location, "%s is not available in Generic Writer\n", __PRETTY_FUNCTION__);
            return (NULL);
        }

        void __write (const netlist_t* netlist, FILE* output_file);
        void _write_blif(const netlist_t* netlist, FILE* output_file);
        /**
         * [TODO]
         * void  write_verilog(const netlist_t* netlist, FILE* output_file);
         * void  write_systemverilog(const netlist_t* netlist, FILE* output_file);
         * void  write_ilang(const netlist_t* netlist, FILE* output_file); 
         */

    private:
        GenericWriter*  blif_writer;
        /**
         * [TODO]
         * GenericWriter*  verilog_writer;
         * GenericWriter*  systemverilog_writer;
         * GenericWriter*  ilang_writer;
        */

};

#endif