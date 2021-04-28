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


#include "GenericReader.hh"
#include "VerilogReader.hh"
#include "BLIFReader.hh"
#include "config_t.h"
#include "odin_ii.h"

GenericReader::GenericReader(): GenericIO() {}

GenericReader::~GenericReader() {
    if (this->verilog_reader)
        this->verilog_reader->~VerilogReader();
    
    if (this->blif_reader)
        this->blif_reader->~BLIFReader();
}

inline void* GenericReader::read() {
    void* netlist = NULL;
    
    switch(configuration.input_file_type) {
        case (file_type_e::_VERILOG): {
            netlist = this->read_verilog();
            break;
        }
        /**
         * [TODO]
         *  case (file_type_e::_SYSTEM_VERILOG): {
                this->read_systemverilog();
                break;
            }
         */ 
        case (file_type_e::_BLIF): {
            netlist = this->read_blif();
            break;
        }
        default : {
            error_message(PARSE_ARGS, unknown_location, "%s", "Unknown input file format! Should have specified in command line arguments\n");
            exit(ERROR_PARSE_ARGS);
        }
    }

    return static_cast<void*>(netlist);   
}

inline void* GenericReader::read_verilog() {
    this->verilog_reader = new VerilogReader();
    return static_cast<void*>(this->verilog_reader->read());
}

inline void* GenericReader::read_blif() {
    this->blif_reader = new BLIFReader();
    return static_cast<void*>(this->blif_reader->read());
}