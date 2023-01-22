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

#include "generic_writer.h"
#include "verilog.h"
#include "blif.h"
#include "config_t.h"
#include "odin_ii.h"

generic_writer::generic_writer()
    : generic_io() {
    this->output_file = NULL;
    this->blif_writer = NULL;
    this->verilog_writer = NULL;
}

generic_writer::~generic_writer() {
    if (this->output_file)
        fclose(this->output_file);
    if (this->blif_writer)
        delete this->blif_writer;
    if (this->verilog_writer)
        delete this->verilog_writer;
}

inline void generic_writer::_write(const netlist_t* netlist) {
    switch (configuration.output_file_type) {
        case (file_type_e::BLIF): {
            this->write_blif(netlist);
            break;
        }
        case (file_type_e::VERILOG): {
            this->write_verilog(netlist);
            break;
        }
        /**
         * [TODO]
         *  case (file_type_e::EBLIF): {
         * netlist = this->write_verilog();
         * break;
         * }
         *  case (file_type_e::SYSTEM_VERILOG): {
         * this->write_systemverilog();
         * break;
         * }
         */
        default: {
            error_message(PARSE_ARGS, unknown_location, "%s", "Unknown input file type!\n");
            exit(ERROR_INITIALIZATION);
        }
    }
}

inline void generic_writer::write_blif(const netlist_t* netlist) {
    oassert(this->blif_writer);
    this->blif_writer->_write(netlist);
}

inline void generic_writer::write_verilog(const netlist_t* netlist) {
    oassert(this->verilog_writer);
    this->verilog_writer->_write(netlist);
}

inline void generic_writer::_create_file(const char* file_name, const file_type_e file_type) {
    // validate the file_name pointer
    oassert(file_name);

    switch (file_type) {
        case (file_type_e::BLIF): {
            if (!this->blif_writer) {
                this->blif_writer = new blif::writer();
                this->blif_writer->_create_file(file_name, file_type);
            }
            break;
        }
        case (file_type_e::VERILOG): {
            this->verilog_writer = new verilog::writer();
            this->verilog_writer->_create_file(file_name, file_type);
            break;
        }
        /**
         * [TODO]
         *  case (file_type_e::EBLIF): {
         * this->eblif_writer = new EBLIF::Writer();
         * this->eblif_writer->_create_file();
         * break;
         * }
         *  case (file_type_e::SYSTEM_VERILOG): {
         * this->sverilog_writer = new SVERILOG::Writer();
         * this->sverilog_writer->_create_file();
         * break;
         * }
         */
        default: {
            error_message(PARSE_ARGS, unknown_location, "%s", "Unknown or invalid output file format!\n");
            exit(ERROR_INITIALIZATION);
        }
    }
}
