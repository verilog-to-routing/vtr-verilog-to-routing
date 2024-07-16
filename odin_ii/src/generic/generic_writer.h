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

#ifndef __GENERIC_WRITER_H__
#define __GENERIC_WRITER_H__

#include "generic_io.h"

/**
 * @brief A class to provide the general object of an input file writer
 */
class generic_writer : public generic_io {
  public:
    /**
     * @brief Construct the generic_writer object
     * required by compiler
     */
    generic_writer();
    /**
     * @brief Destruct the generic_writer object
     * to avoid memory leakage
     */
    ~generic_writer();

    /* No need to have reader in Generic Writer */
    void* _read() {
        error_message(UTIL, unknown_location, "%s is not available in Generic Writer\n", __PRETTY_FUNCTION__);
        return (NULL);
    }

    void _write(const netlist_t* netlist);
    void write_blif(const netlist_t* netlist);
    void write_verilog(const netlist_t* netlist);
    /**
     * [TODO]
     * void  write_systemverilog(const netlist_t* netlist, FILE* output_file);
     * void  write_ilang(const netlist_t* netlist, FILE* output_file); 
     */

    /* to create the output file */
    void _create_file(const char* file_name, const file_type_e file_type = file_type_e_END);

  protected:
    FILE* output_file;

  private:
    generic_writer* blif_writer;
    generic_writer* verilog_writer;
    /**
     * [TODO]
     * generic_writer*  systemverilog_writer;
     * generic_writer*  ilang_writer;
     */
};

#endif //__GENERIC_WRITER_H__
