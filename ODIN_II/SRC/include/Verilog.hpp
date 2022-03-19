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

/* Yosys models attributes to be printed in a Verilog file */
#define BLACKBOX_ATTR "(* blackbox *)"
#define KEEP_HIERARCHY_ATTR "(* keep_hierarchy *)"
/* useful aliases and fixed comment messages */
#define TAB "\t"
#define NEWLINE "\n"
#define HARD_BLOCK_COMMENT "/* the body of the hardblock model is empty since it should be seen as a blackbox */"
/* some fix keywords in the Verilog standard */
#define MODULE "module"
#define OPEN_PARENTHESIS "("
#define CLOSE_PARENTHESIS ")"
#define OPEN_SQUARE_BRACKET "["
#define CLOSE_SQUARE_BRACKET "]"
#define SEMICOLON ";"
#define COLON ":"
#define COMMA ","
#define SPACE " "
#define INPUT_PORT "input"
#define OUTPUT_PORT "output"
#define WIRE_PORT "wire"
#define REG_PORT "reg"
#define END_MODULE "endmodule"

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
        void _create_file(const char* file_name, const file_type_e file_type = _VERILOG);

        /**
         *-------------------------------------------------------------------------------------------
         * (function: declare_blackbox)
         * 
         * @brief find the corresponding blackbox with the given 
         * name in the given target arhitecture, then add its 
         * Verilog declartion to this->models string cache.
         * 
         * @param bb_name the blackbox(DSP) name
         * 
         * @return a long value, which is representing the index of 
         * the declartion in models string cache. Will return -1 if 
         * a DSP with the given name does not exist in the architecture.
         *-------------------------------------------------------------------------------------------
         */
        long declare_blackbox(const char* bb_name);

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
        /**
         *-------------------------------------------------------------------------------------------
         * (function: declare_ports)
         * 
         * @brief generate a string that includes the declaration 
         * of input/output ports of a given t_model
         * 
         * @param model the DSP t_model pointer
         * 
         * @return a string value including the declaration of all 
         * input/output ports related to the given DSP model
         *-------------------------------------------------------------------------------------------
         */
        std::string declare_ports(t_model* model);
    };
};

#endif //__VERILOG_H__
