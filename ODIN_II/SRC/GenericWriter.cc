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
 */

#include "GenericWriter.hh"
#include "Verilog.hh"
#include "BLIF.hh"
#include "config_t.h"
#include "odin_ii.h"

GenericWriter::GenericWriter()
    : GenericIO() {
    this->output_file = NULL;
    this->blif_writer = NULL;
}

GenericWriter::~GenericWriter() {
    if (this->output_file)
        fclose(this->output_file);
}

inline void GenericWriter::__write(const netlist_t* netlist) {
    switch (configuration.output_file_type) {
        case (file_type_e::_BLIF): {
            this->_write_blif(netlist);
            break;
        }
        /**
         * [TODO]
         *  case (file_type_e::_VERILOG): {
                netlist = this->write_verilog();
                break;
            }
         *  case (file_type_e::_EBLIF): {
                netlist = this->write_verilog();
                break;
            }
         *  case (file_type_e::_SYSTEM_VERILOG): {
                this->write_systemverilog();
                break;
            }
         */
        default: {
            error_message(PARSE_ARGS, unknown_location, "%s", "Unknown input file format! Should have specified in command line arguments\n");
            exit(ERROR_INITIALIZATION);
        }
    }
}

inline void GenericWriter::_write_blif(const netlist_t* netlist) {
    oassert(this->blif_writer);
    this->blif_writer->__write(netlist);

    if (this->blif_writer)
        delete this->blif_writer;
}

inline void GenericWriter::__create_file(const file_type_e file_type) {
    switch (file_type) {
        case (file_type_e::_BLIF): {
            if (!this->blif_writer) {
                this->blif_writer = new BLIF::Writer();
                this->blif_writer->__create_file(file_type);
            }
            break;
        }
        /**
         * [TODO]
         *  case (file_type_e::_VERILOG): {
                this->verilog_writer = new VERILOG::Writer();
                this->verilog_writer->__create_file();
                break;
            }
         *  case (file_type_e::_EBLIF): {
                this->eblif_writer = new EBLIF::Writer();
                this->eblif_writer->__create_file();
                break;
            }
         *  case (file_type_e::_SYSTEM_VERILOG): {
                this->sverilog_writer = new SVERILOG::Writer();
                this->sverilog_writer->__create_file();
                break;
            }
         */
        default: {
            error_message(PARSE_ARGS, unknown_location, "%s", "Unknown or invalid output file format!\n");
            exit(ERROR_INITIALIZATION);
        }
    }
}