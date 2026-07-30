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

//
#include "kernel/celltypes.h"
#include "kernel/sigtools.h"
#include "kernel/yosys.h"
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

static bool dot = false;

struct MaxHeigthWorker {
  RTLIL::Design *design;
  RTLIL::Module *module;
  SigMap sigmap;

  // Bit info attached to a SigBit:
  //    - level
  //    - sigBit
  //    - driven cell from the sigbit
  //    - height
  //
  dict<SigBit, tuple<pool<SigBit> *, Cell *, int>> bits;

#define INPUTS(x) get<0>(x)
#define CELL(x) get<1>(x)
#define HEIGTH(x) get<2>(x)

  pool<SigBit> visited_bits;

  pool<SigBit> cps;
  pool<SigBit> luts;
  dict<SigBit, int> fanout;

  // ---------------------
  // MaxHeigthWorker
  // ---------------------
  //
  MaxHeigthWorker(RTLIL::Module *module)
      : design(module->design), module(module), sigmap(module) {

    for (auto wire : module->selected_wires()) {

      for (auto bit : sigmap(wire)) {

        bits[bit] = tuple<pool<SigBit> *, Cell *, int>(nullptr, nullptr, -1);
      }
    }

    for (auto cell : module->selected_cells()) {

      if ((cell->type != "$lut") &&

          // Handle Ice40 cells
          //
          (cell->type != "\\SB_LUT4") && (cell->type != "\\SB_CARRY") &&

          // Handle also Xilinx lut cells
          //
          (cell->type != "\\MUXF5") && (cell->type != "\\MUXF6") &&
          (cell->type != "\\MUXF7") && (cell->type != "\\MUXF8") &&
          (cell->type != "\\LUT1") && (cell->type != "\\LUT2") &&
          (cell->type != "\\LUT3") && (cell->type != "\\LUT4") &&
          (cell->type != "\\LUT5") && (cell->type != "\\LUT6")) {

        continue;
      }

      SigBit out;
      bool out_exists = false;

      pool<SigBit> *inputs = new pool<SigBit>;

      for (auto &conn : cell->connections()) {

        for (auto bit : sigmap(conn.second)) {

          if (cell->input(conn.first)) {
            inputs->insert(bit);
            continue;
          }

          if (cell->output(conn.first)) {
            out = bit;
            out_exists = true;
          }
        }
      }

      if (!out_exists) {
        log_warning("It seems Lut cell '%s' is undriven.\n",
                    log_id(cell->name));
        continue;
      }

      luts.insert(out);

      auto &bitinfo = bits.at(out);

      INPUTS(bitinfo) = inputs;
      CELL(bitinfo) = cell;
    }
  }

  // ---------------------
  // get_cp_logic_rec
  // ---------------------
  void get_cp_logic_rec(SigBit bit, int height) {
    if (bits.count(bit) == 0) { // constant case
      return;
    }

    auto &bitinfo = bits.at(bit);

    // Bit already in 'cps' so already traversed and TFI already added in 'cps'
    //
    if (cps.count(bit)) {
      return;
    }

    // return if a PI
    //
    if (HEIGTH(bitinfo) == 0) {
      return;
    }

    // If on the CP then add it
    //
    if (HEIGTH(bitinfo) == height) {
      assert(CELL(bitinfo)); // make sure it is driven
      cps.insert(bit);
    }

    // return if not on the CP
    //
    if (HEIGTH(bitinfo) < height) {
      return;
    }

    if (HEIGTH(bitinfo) > height) {
      log_error("Problem in the CP extractor : Height %d must be less than "
                "height %d\n",
                HEIGTH(bitinfo), height);
    }

    pool<SigBit> *inputs = INPUTS(bitinfo);

    for (auto from : *inputs) {

      // Collect recursivelet from the TFI from 'from' with height 'height-1'
      //
      get_cp_logic_rec(from, height - 1);
    }
  }

  // ---------------------
  // get_cp_logic
  // ---------------------
  void get_cp_logic(int max_height) {
    // Extract CP from the 'bits' starting with 'max_height'.
    //
    for (auto &it : bits) {

      if (HEIGTH(it.second) == max_height) {
        get_cp_logic_rec(it.first, max_height);
      }
    }
  }

  // ---------------------
  // legalize_dot_name
  // ---------------------
  void legalize_dot_name(string &s) {
    for (long unsigned int i = 0; i < s.size(); i++) {

      if (s[i] == '$') {
        s[i] = '_';
        continue;
      }
      if (s[i] == '\\') {
        s[i] = '_';
        continue;
      }
      if (s[i] == '[') {
        s[i] = '_';
        continue;
      }
      if (s[i] == ']') {
        s[i] = '_';
        continue;
      }
      if (s[i] == '.') {
        s[i] = '_';
        continue;
      }
      if (s[i] == ':') {
        s[i] = '_';
        continue;
      }
    }
  }

  // ---------------------
  // dot_cell
  // ---------------------
  void dot_cell(std::ofstream &cells_dot, Cell *cell) {
    for (auto &conn : cell->connections()) {

      string cell_name = log_id(cell->name);
      legalize_dot_name(cell_name);

      for (auto bit : sigmap(conn.second)) {

        string name = log_signal(bit);
        legalize_dot_name(name);

        if (cell->input(conn.first)) {
          cells_dot << cell_name << " -> " << name << "\n";
          continue;
        }

        if (cell->output(conn.first)) {
          cells_dot << name << " -> " << cell_name << "\n";
          continue;
        }
      }
    }
  }

  // ---------------------
  // dump_cp_dot
  // ---------------------
  void dump_cp_dot() {
    std::ofstream cells_dot;

    cells_dot.open("cp.dot");

    cells_dot << "digraph cp {\n";

    // Dump definition and collect Cells in CP
    //
    pool<Cell *> cells;

    for (auto bit : cps) {

      auto &bitinfo = bits.at(bit);

      Cell *cell = CELL(bitinfo);
      int height = HEIGTH(bitinfo);
      pool<SigBit> *inputs = INPUTS(bitinfo);

      cells.insert(cell);

      string cell_name = log_id(cell->name);
      legalize_dot_name(cell_name);
      int fo = fanout[bit];
      long unsigned int nb = inputs->size();
      cells_dot << cell_name << " [shape=box, label=\"" << cell_name << "\nLUT"
                << nb << "\nheight=" << height << "\nfo=" << fo << "\"]\n";
    }

    // Dump definition and Collect cells periphery bits
    //
    pool<SigBit> cp_bits;

    for (auto cell : cells) {

      for (auto &conn : cell->connections()) {

        for (auto bit : sigmap(conn.second)) {

          cp_bits.insert(bit);

          string name = log_signal(bit);
          legalize_dot_name(name);

          auto &bitinfo = bits.at(bit);
          int height = HEIGTH(bitinfo);

          if (cps.count(bit)) {
            cells_dot << name << " [color=\"red\" style=filled label=\"" << name
                      << "\nheight=" << height << "\"]\n";
          } else {
            cells_dot << name << " [color=\"green\" style=filled label=\""
                      << name << "\nheight=" << height << "\"]\n";
          }
        }
      }
    }

    for (auto cell : cells) {
      dot_cell(cells_dot, cell);
    }

    cells_dot << "}\n";
    cells_dot.close();

    log("\nDumped 'cp.dot' file. Use 'xdot' to view it\n");
    system("xdot cp.dot");
  }

  // ---------------------
  // get_height_rec
  // ---------------------
  int get_height_rec(SigBit bit) {
    if (bits.count(bit) == 0) { // constant case
      return 0;
    }

    auto &bitinfo = bits.at(bit);

    fanout[bit] += 1;

    int height = HEIGTH(bitinfo);

    if (height >= 0) {
      return height;
    }

    if (!CELL(bitinfo)) { // if 'bit' has no cell driving it it is a PI or
                          // undriven wire
      HEIGTH(bitinfo) = 0;
      return 0;
    }

    if (visited_bits.count(bit) > 0) {
      HEIGTH(bitinfo) = 0;
      log_warning("Detected loop at %s in %s\n", log_signal(bit),
                  log_id(module));
      return 0;
    }

    visited_bits.insert(bit);

    pool<SigBit> *inputs = INPUTS(bitinfo);

    int max_height = -1;

    for (auto in : *inputs) {

      int in_height = get_height_rec(in);

      if (in_height > max_height) {
        max_height = in_height;
      }
    }

    HEIGTH(bitinfo) = max_height + 1;

    return max_height + 1;
  }

  // ---------------------
  // get_max_height
  // ---------------------
  int get_max_height() {
    int max_height = -1;

    for (auto &it : bits) {
      fanout[it.first] = 0;
    }

    for (auto &it : bits) {

      if (HEIGTH(it.second) < 0) { // if Heigth not computed yet

        visited_bits.clear();

        int height = get_height_rec(it.first);

        if (height > max_height) {
          max_height = height;
        }
      }
    }

    return max_height;
  }

  // ---------------------
  // run
  // ---------------------
  void run() {
    auto startTime = std::chrono::high_resolution_clock::now();

    // get the max height of the whole LUT logic
    //
    int max_height = get_max_height();

    design->scratchpad_set_int("max_height.max_height", max_height);

    // get the logic on the critical paths starting with height 'max_height'
    //
    get_cp_logic(max_height);

    // Eventually dump the dot file fo the CP logic
    // NOTE: we can use 'xdot' to view the dot file
    //
    if (dot) {
      dump_cp_dot();
    }

    log("\n");
    log("   Max height / Max levels    = %d\n", max_height);
    log("   CP bits size               = %ld\n", cps.size());
    log("   Total lut output bits      = %ld\n", luts.size());

    auto endTime = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
        endTime - startTime);

    float totalTime = 1 + elapsed.count() * 1e-9;

    log("   [Run Time = %.1f sec.]\n", totalTime);
  }
};

struct MaxHeigthPass : public ScriptPass {

  RTLIL::Design *G_design = NULL;

  MaxHeigthPass() : ScriptPass("max_height", "print max logic height") {}

  // ---------------------
  // help
  // ---------------------
  void help() override {
    //   |---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|
    log("\n");
    log("    max_height [options] \n");
    log("\n");
    log("This command prints the max logic height in the design. (Only "
        "considers\n");
    log("paths within a single module, so the design must be flattened to get "
        "the)\n");
    log("overall longest path in the design.\n");
    log("\n");
    log("    -dot\n");
    log("        Dump a 'dot' file representing only the critical logic.\n");
    log("\n");
  }

  // ---------------------
  // clear_flags
  // ---------------------
  void clear_flags() override { dot = false; }

  // ---------------------
  // execute
  // ---------------------
  void execute(std::vector<std::string> args, RTLIL::Design *design) override {
    string run_from, run_to;

    clear_flags();

    log_header(design,
               "Executing 'max_height' command (find max logic level).\n");

    size_t argidx;

    G_design = design;

    for (argidx = 1; argidx < args.size(); argidx++) {
      if (args[argidx] == "-dot") {
        dot = true;
        continue;
      }
    }

    extra_args(args, argidx, design);

    run_script(design, run_from, run_to);
  }

  // -------------------------
  // load_LUT_models
  // -------------------------
  void load_LUT_models() {
    run("read_verilog +/plugins/wildebeest/lut_models/LUTs.v");

    run("hierarchy -auto-top");
  }

  void script() override {
    if (!G_design) {
      log_warning("Design seems empty !\n");
      return;
    }

    if (dot) {

      run("design -push-copy");

      run("splitnets -ports");

      run(stringf("write_verilog -noexpr -nohex -nodec -norename -simple-lhs "
                  "dot_cells.vlg"));
      run(stringf(
          "write_verilog -nohex -nodec -norename -simple-lhs dot_expr.vlg"));
    }

    // Usefull to extract Xilinx levels
    //
    load_LUT_models();

    for (Module *module : G_design->selected_modules()) {
      if (module->has_processes_warn()) {
        continue;
      }

      MaxHeigthWorker worker(module);
      worker.run();
    }

    if (dot) {
      run("design -pop");
    }
  }

} MaxHeigthPass;

PRIVATE_NAMESPACE_END
