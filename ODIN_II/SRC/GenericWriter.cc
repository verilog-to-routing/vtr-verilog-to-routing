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


#include "GenericWriter.hh"
#include "VerilogReader.hh"
#include "BLIF.hh"
#include "config_t.h"
#include "odin_ii.h"

GenericWriter::GenericWriter(): GenericIO() {}

GenericWriter::~GenericWriter() {    
    if (this->blif_writer)
        static_cast<BLIF::Writer*>(this->blif_writer)->~Writer();
        
    if (this->output_file)
        fclose(this->output_file);
}

inline void GenericWriter::__write(const netlist_t* netlist) { 
    switch(configuration.output_file_type) {
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
        default : {
            error_message(PARSE_ARGS, unknown_location, "%s", "Unknown input file format! Should have specified in command line arguments\n");
            exit(ERROR_PARSE_ARGS);
        }
    }
}

inline void GenericWriter::_write_blif(const netlist_t* netlist) {
    this->blif_writer = new BLIF::Writer();
    this->blif_writer->__write(netlist);
}