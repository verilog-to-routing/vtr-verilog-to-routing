/*
ISC License

Copyright (C) 2025  Frederick Tombs <fred@zeroasic.com>, Zero Asic Corp.

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "kernel/sigtools.h"
#include "kernel/yosys.h"
#include <deque>

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

#include "zeroasic_dsp.h"

void zeroasic_dsp_pack(zeroasic_dsp_pm &pm) {
  auto &st = pm.st_zeroasic_dsp_pack;

  Cell *cell = st.dsp;
  // pack post-adder
  //
  if (st.postAdderStatic) {
    cell->setParam(ID(POST_ADDER_STATIC), State::S1);
    if (st.useFeedBack) {
      cell->setParam(ID(USE_FEEDBACK), State::S1);
      cell->setPort(ID(CDIN_FDBK_SEL), {State::S0, State::S1});
    } else {
      cell->setParam(ID(USE_FEEDBACK), State::S0);
      cell->setPort(ID::C, st.sigC);
    }

    pm.autoremove(st.postAdderStatic);
  }

  if (st.multHasReg) {
    cell->setParam(ID(MULT_HAS_REG), State::S1);
    pm.autoremove(st.ffMult);
  } else {
    cell->setParam(ID(MULT_HAS_REG), State::S0);
  }
  // pack registers
  //
  if (st.clock != SigBit()) {
    cell->setPort(ID::CLK, st.clock);

    // function to absorb a register
    auto f = [&pm, cell](SigSpec &A, Cell *ff, IdString ceport,
                         IdString rstport, IdString bypass,
                         IdString bypass_param) {
      // input/output ports
      SigSpec D = ff->getPort(ID::D);
      SigSpec Q = pm.sigmap(ff->getPort(ID::Q));

      if (!A.empty())
        A.replace(Q, D);

      if (ff->type.in(ID($dffe), ID($sdffe), ID($adffe))) {
        SigSpec ce = ff->getPort(ID::EN);
        bool cepol = ff->getParam(ID::EN_POLARITY).as_bool();
        // enables are all active high
        cell->setPort(ceport, cepol ? ce : pm.module->Not(NEW_ID, ce));
      } else {
        // enables are all active high
        cell->setPort(ceport, State::S1);
      }

      // bypass set to 0
      cell->setParam(bypass_param, State::S1);

      for (auto c : Q.chunks()) {
        auto it = c.wire->attributes.find(ID::init);
        if (it == c.wire->attributes.end())
          continue;
        for (int i = c.offset; i < c.offset + c.width; i++) {
          log_assert(it->second[i] == State::S0 || it->second[i] == State::Sx);
#if YOSYS_MAJOR == 0 && YOSYS_MINOR <= 57
          it->second.bits()[i] = State::Sx;
#else
	  it->second.set(i, State::Sx);
#endif
        }
      }
    };

    if (st.ffA && st.ffB) { // both A and B have to be registered
      SigSpec A = cell->getPort(ID::A);
      if (st.ffA) {
        f(A, st.ffA, ID(A_EN), ID(A_ARST_N), ID(ALLOW_A_REG), ID(A_REG));
      }
      pm.add_siguser(A, cell);
      cell->setPort(ID::A, A);

      SigSpec B = cell->getPort(ID::B);
      if (st.ffB) {
        f(B, st.ffB, ID(B_EN), ID(B_ARST_N), ID(ALLOW_B_REG), ID(B_REG));
      }
      pm.add_siguser(B, cell);
      cell->setPort(ID::B, B);

      // set the reset port to the reset (which has to be shared by both A and
      // B)
      if (st.ffA->type.in(ID($sdff), ID($sdffe))) {
        log("Error: synchronous DSPs not packable on MAE\n");
        log_assert(not st.ffA->type.in(ID($sdff), ID($sdffe)));
      } else if (st.ffA->type.in(ID($adff), ID($adffe))) {
        SigSpec arst = st.ffA->getPort(ID::ARST);
        bool rstpol_n = !st.ffA->getParam(ID::ARST_POLARITY).as_bool();
        // active low async rst
        cell->setPort(ID(resetn),
                      rstpol_n ? arst : pm.module->Not(NEW_ID, arst));
      } else {
        // active low async/sync rst
        cell->setPort(ID(resetn), State::S1);
      }
    } else {
      cell->setParam(ID(A_REG), State::S0);
      cell->setParam(ID(B_REG), State::S0);
    }

    if (st.ffC) {
      SigSpec C = cell->getPort(ID::C);
      f(C, st.ffC, ID(C_EN), ID(C_ARST_N), ID(ALLOW_C_REG), ID(C_REG));

      pm.add_siguser(C, cell);
      cell->setPort(ID::C, C);
    }
    if (st.ffP) {
      SigSpec P; // unused
      f(P, st.ffP, ID(P_EN), ID(P_ARST_N), ID(ALLOW_P_REG), ID(P_REG));
      st.ffP->connections_.at(ID::Q).replace(
          st.sigP, pm.module->addWire(NEW_ID, GetSize(st.sigP)));
    }

    log("  clock: %s (%s)\n", log_signal(st.clock), "posedge");

    if (st.ffA)
      log(" \t ffA:%s\n", log_id(st.ffA));
    if (st.ffB)
      log(" \t ffB:%s\n", log_id(st.ffB));
    if (st.ffC)
      log(" \t ffC:%s\n", log_id(st.ffC));
    if (st.ffP)
      log(" \t ffP:%s\n", log_id(st.ffP));
  }
  log("\n");

  SigSpec P = st.sigP;
  if (GetSize(P) < 40)
    P.append(pm.module->addWire(NEW_ID, 40 - GetSize(P)));
  cell->setPort(ID::P, P);

  pm.blacklist(cell);
}

struct ZeroAsicDspPass : public Pass {
  ZeroAsicDspPass()
      : Pass("zeroasic_dsp", "ZEROASIC: pack resources into DSPs") {}
  void help() override {
    //   |---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|
    log("\n");
    log("    zeroasic_dsp [options] [selection]\n");
    log("\n");
    log("Pack input registers 'A', 'B', 'C', and 'D' (with optional "
        "enable/reset),\n");
    log("output register 'P' (with optional enable/reset), pre-adder and/or "
        "post-adder into\n");
    log("ZeroAsic DSP resources.\n");
    log("\n");
  }

  void execute(std::vector<std::string> args, RTLIL::Design *design) override {
    log_header(design, "Executing ZEROASIC_DSP pass (pack DFFs into DSPs).\n");

    size_t argidx;
    for (argidx = 1; argidx < args.size(); argidx++) {
      break;
    }
    extra_args(args, argidx, design);

    for (auto module : design->selected_modules()) {

      if (design->scratchpad_get_bool("zeroasic_dsp.multonly"))
        continue;

      {
        zeroasic_dsp_pm pm(module, module->selected_cells());
        pm.run_zeroasic_dsp_pack(zeroasic_dsp_pack);
      }
    }
  }
} ZeroAsicDspPass;

PRIVATE_NAMESPACE_END
