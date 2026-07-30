/*
 *  yosys -- Yosys Open SYnthesis Suite
 *
 *  Copyright (C) 2012  Claire Xenia Wolf <claire@yosyshq.com>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifndef SIMPLEMAP_H
#define SIMPLEMAP_H

#include "kernel/yosys.h"

YOSYS_NAMESPACE_BEGIN

extern void zsimplemap_not(RTLIL::Module *module, RTLIL::Cell *cell);
extern void zsimplemap_pos(RTLIL::Module *module, RTLIL::Cell *cell);
extern void zsimplemap_bitop(RTLIL::Module *module, RTLIL::Cell *cell);
extern void zsimplemap_reduce(RTLIL::Module *module, RTLIL::Cell *cell);
extern void zsimplemap_lognot(RTLIL::Module *module, RTLIL::Cell *cell);
extern void zsimplemap_logbin(RTLIL::Module *module, RTLIL::Cell *cell);
extern void zsimplemap_mux(RTLIL::Module *module, RTLIL::Cell *cell);
extern void zsimplemap_bwmux(RTLIL::Module *module, RTLIL::Cell *cell);
extern void zsimplemap_lut(RTLIL::Module *module, RTLIL::Cell *cell);
extern void zsimplemap_slice(RTLIL::Module *module, RTLIL::Cell *cell);
extern void zsimplemap_concat(RTLIL::Module *module, RTLIL::Cell *cell);
extern void zsimplemap_ff(RTLIL::Module *module, RTLIL::Cell *cell);
extern void zsimplemap(RTLIL::Module *module, RTLIL::Cell *cell);

extern void zsimplemap_get_mappers(
    dict<RTLIL::IdString, void (*)(RTLIL::Module *, RTLIL::Cell *)> &mappers);

YOSYS_NAMESPACE_END

#endif
