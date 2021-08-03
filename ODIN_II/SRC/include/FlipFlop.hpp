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
#ifndef __FLIPFLOP_H__
#define __FLIPFLOP_H__

#include "odin_types.h"

extern void resolve_dff_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
extern void resolve_sdff_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
extern void resolve_dffe_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
extern void resolve_sdffe_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
extern void resolve_sdffce_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
extern void resolve_dffsr_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);
extern void resolve_dffsre_node(nnode_t* node, uintptr_t traverse_mark_number, netlist_t* netlist);

#endif //__FLIPFLOP_H__
