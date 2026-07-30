//
//  Copyright (C) 2025  Thierry Besson <thierry@zeroasic.com>, Zero Asic Corp.
//
/*
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

#include "kernel/ff.h"
#include "kernel/ffinit.h"
#include "kernel/log.h"
#include "kernel/modtools.h"
#include "kernel/register.h"
#include "kernel/rtlil.h"
#include "kernel/sigtools.h"
#include <stdio.h>
#include <stdlib.h>

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct ZOptConstDffOptions {
  bool nosdff;
  bool nodffe;
  bool simple_dffe;
  bool sat;
  bool keepdc;
};

struct ZOptConstDffWorker {
  const ZOptConstDffOptions &opt;

  Module *module;
  typedef std::pair<RTLIL::Cell *, int> cell_int_t;
  SigMap sigmap;
  FfInitVals initvals;
  dict<SigBit, int> bitusers;
  dict<SigBit, cell_int_t> bit2mux;

  typedef std::map<RTLIL::SigBit, bool> pattern_t;
  typedef std::set<pattern_t> patterns_t;
  typedef std::pair<RTLIL::SigBit, bool> ctrl_t;
  typedef std::set<ctrl_t> ctrls_t;

  // Used as a queue.
  std::vector<Cell *> dff_cells;

  ZOptConstDffWorker(const ZOptConstDffOptions &opt, Module *mod)
      : opt(opt), module(mod), sigmap(mod), initvals(&sigmap, mod) {
    // Gathering two kinds of information here for every sigmapped SigBit:
    //
    // - bitusers: how many users it has (muxes will only be merged into FFs
    // if this is 1, making the FF the only user)
    // - bit2mux: the mux cell and bit index that drives it, if any

    for (auto wire : module->wires()) {

      if (wire->port_output) {

        for (auto bit : sigmap(wire)) {
          bitusers[bit]++;
        }
      }
    }

    for (auto cell : module->cells()) {

      if (cell->type.in(ID($mux), ID($pmux), ID($_MUX_))) {

        RTLIL::SigSpec sig_y = sigmap(cell->getPort(ID::Y));

        for (int i = 0; i < GetSize(sig_y); i++) {
          bit2mux[sig_y[i]] = cell_int_t(cell, i);
        }
      }

      for (auto conn : cell->connections()) {
        bool is_output = cell->output(conn.first);
        if (!is_output || !cell->known()) {
          for (auto bit : sigmap(conn.second))
            bitusers[bit]++;
        }
      }

      if (module->design->selected(module, cell) &&
#if YOSYS_MAJOR == 0 && YOSYS_MINOR <= 57
          RTLIL::builtin_ff_cell_types().count(cell->type)) {
#else
          cell->is_builtin_ff()) {
#endif

        dff_cells.push_back(cell);
      }
    }
  }

  // Check for DFF driven by constants and try simplification
  // if possible.
  //
  bool run() {

    ModWalker modwalker(module->design, module);

    // Defer mutating cells by removing them/emiting new flip flops so that
    // cell references in modwalker are not invalidated
    //
    std::vector<RTLIL::Cell *> cells_to_remove;
    std::vector<FfData> ffs_to_emit;

    bool did_something = false;

    for (auto cell : module->selected_cells()) {

      if (((cell->type).substr(0, 5) != "$_DFF") &&
          ((cell->type).substr(0, 6) != "$_SDFF")) {
        continue;
      }

#if YOSYS_MAJOR == 0 && YOSYS_MINOR <= 57
      if (!RTLIL::builtin_ff_cell_types().count(cell->type)) {
#else
      if (!cell->is_builtin_ff()) {
#endif
        continue;
      }

      FfData ff(&initvals, cell);

      if (!ff.width) {
        continue;
      }

      pool<int> removed_sigbits;

      // Now check if any bit can be replaced by a constant.
      //
      for (int i = 0; i < ff.width; i++) {

        if ((ff.sig_d[i] != State::S1) && (ff.sig_d[i] != State::S0)) {
          continue;
        }

        State val = ff.val_init[i];

        if (ff.has_arst) {

          if (ff.sig_arst == (ff.pol_arst ? State::S0 : State::S1)) {
            // Always-inactive async. reset — ignore.

          } else if (ff.sig_d[i] == State::S1) {
            continue; // conflict : drive DFF with '1' but DFF has reset
          }
        }

        if (ff.has_srst) {

          if (ff.sig_srst == (ff.pol_srst ? State::S0 : State::S1)) {
            // Always-inactive sync. reset — ignore.

          } else if (ff.sig_d[i] == State::S1) {
            continue; // conflict : drive DFF with '1' but DFF has reset
          }
        }

        // Need to process corner cases with DFF with s/r
        //
        if (ff.has_sr) {
          log("NOTE: DFF with S/R is driven by constant\n");
          continue;
        }

        // Having clock enable should not impact the correctness
        // of replacing DFF by constant
        //
        if (ff.has_ce) {
          // continue;
        }

        // if constant driving DFF is different of init value we are not
        // supposed to replace the DFF by the driving constant.
        // Example is "logikbench/memory/axiram' where we cannot
        // simplify one DFF driven by '1' with initvalue '0'.
        //
        if (val && (ff.sig_d[i] == State::S0)) {
          log_warning("Conflicting init val '%d' for %s (%s) driven by '0'. "
                      "Cannot remove stuck-at-0 DFF !\n",
                      (val ? 1 : 0), log_id(cell), log_id(cell->type));
          continue;
        }

        if (!val && (ff.sig_d[i] == State::S1)) {
          log_warning("Conflicting init val '%d' for %s (%s) driven by '1'. "
                      "Cannot remove stuck-at-1 DFF !\n",
                      (val ? 1 : 0), log_id(cell), log_id(cell->type));
          continue;
        }

        log("Replacing DFF by constant '%d' at position %d on %s (%s)\n",
            (ff.sig_d[i] == State::S1) ? 1 : 0, i, log_id(cell),
            log_id(cell->type));

        initvals.remove_init(ff.sig_q[i]);
        module->connect(ff.sig_q[i], val);
        removed_sigbits.insert(i);
      }

      if (!removed_sigbits.empty()) {

        std::vector<int> keep_bits;

        for (int i = 0; i < ff.width; i++) {
          if (!removed_sigbits.count(i)) {
            keep_bits.push_back(i);
          }
        }

        if (keep_bits.empty()) {
          cells_to_remove.emplace_back(cell);
          did_something = true;
          continue;
        }

        ff = ff.slice(keep_bits);
        ff.cell = cell;
        ffs_to_emit.emplace_back(ff);
        did_something = true;
      }
    }

    for (auto *cell : cells_to_remove) {
      module->remove(cell);
    }

    for (auto &ff : ffs_to_emit) {
      ff.emit();
    }

    return did_something;
  }
};

struct ZOptConstDffPass : public Pass {

  ZOptConstDffPass()
      : Pass("zopt_const_dff",
             "perform DFF optimizations for DFF driven by constants") {}

  void help() override {
    //   |---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|
    log("\n");
    log("    opt_const_dff\n");
    log("\n");
    log("This pass tries to replace flip-flops driven by a logic constant "
        "by\n");
    log("this constant itself. It may create simulation differences because it "
        "may\n");
    log("ignore DFF init values in some cases especially when the driven "
        "constant \n");
    log("is different with the init value.\n");
    log("\n");
  }

  void execute(std::vector<std::string> args, RTLIL::Design *design) override {
    log_header(design, "Executing ZOPT_CONST_DFF pass (perform DFF "
                       "optimizations driven by constants).\n");

    ZOptConstDffOptions opt;
    opt.nodffe = false;
    opt.nosdff = false;
    opt.simple_dffe = false;
    opt.keepdc = false;
    opt.sat = false;

    size_t argidx;
    for (argidx = 1; argidx < args.size(); argidx++) {
    }

    extra_args(args, argidx, design);

    bool did_something = false;

    for (auto mod : design->selected_modules()) {

      ZOptConstDffWorker worker(opt, mod);

      if (worker.run()) {

        did_something = true;
      }
    }

    if (did_something) {

      design->scratchpad_set_bool("opt.did_something", true);
    }
  }
} ZOptConstDffPass;

PRIVATE_NAMESPACE_END
