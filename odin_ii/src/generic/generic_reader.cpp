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

#include "generic_reader.h"
#include "verilog.h"
#include "blif.h"
#include "config_t.h"
#include "odin_ii.h"

generic_reader::generic_reader()
    : generic_io() {
    this->verilog_reader = NULL;
    this->blif_reader = NULL;
}

generic_reader::~generic_reader() = default;

inline void* generic_reader::_read() {
    void* netlist = NULL;

    switch (configuration.input_file_type) {
        case (file_type_e::VERILOG): // fallthrough
        case (file_type_e::VERILOG_HEADER): {
            netlist = this->read_verilog();
            break;
        }
        case (file_type_e::BLIF): {
            netlist = this->read_blif();
            break;
        }
        /**
         * [TODO]
         *  case (file_type_e::EBLIF): {
         * this->read_systemverilog();
         * break;
         * }
         *  case (file_type_e::SYSTEM_VERILOG): {
         * this->read_systemverilog();
         * break;
         * }
         */
        default: {
            error_message(PARSE_ARGS, unknown_location, "%s", "Unknown input file type!\n");
            exit(ERROR_PARSE_ARGS);
        }
    }

    return static_cast<void*>(netlist);
}

inline void* generic_reader::read_verilog() {
    this->verilog_reader = new verilog::reader();
    void* to_return = this->verilog_reader->_read();

    delete this->verilog_reader;

    return to_return;
}

inline void* generic_reader::read_blif() {
    this->blif_reader = new blif::reader();
    void* to_return = this->blif_reader->_read();

    delete this->blif_reader;

    return to_return;
}
