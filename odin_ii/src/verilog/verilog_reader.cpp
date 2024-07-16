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

#include "verilog.h"
#include "ast_util.h"
#include "odin_globals.h"
#include "parse_making_ast.h"
#include "netlist_create_from_ast.h"

verilog::reader::reader()
    : generic_reader() {}

verilog::reader::~reader() = default;

void* verilog::reader::_read() {
    /* parse to abstract syntax tree */
    printf("Parser starting - we'll create an abstract syntax tree. Note this tree can be viewed using Grap Viz (see documentation)\n");
    verilog_ast = init_parser();
    parse_to_ast();

    /**
     *  Note that the entry point for ast optimzations is done per module with the
     * function void next_parsed_verilog_file(ast_node_t *file_items_list)
     */

    /* after the ast is made potentially do tagging for downstream links to verilog */
    if (global_args.high_level_block.provenance() == argparse::Provenance::SPECIFIED)
        add_tag_data(verilog_ast);

    /**
     *  Now that we have a parse tree (abstract syntax tree [ast]) of
     *	the verilog we want to make into a netlist.
     */
    printf("Converting AST into a Netlist. Note this netlist can be viewed using GraphViz (see documentation)\n");
    create_netlist(verilog_ast);

    return static_cast<void*>(syn_netlist);
}
