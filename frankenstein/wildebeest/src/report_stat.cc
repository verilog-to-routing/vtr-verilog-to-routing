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

#include "kernel/celltypes.h"
#include "kernel/log.h"
#include "kernel/register.h"
#include "kernel/rtlil.h"
#include <cassert>
#include <chrono>
#include <iomanip>

#define SYNTH_FPGA_VERSION "1.0"

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct ReportStatPass : public ScriptPass {
  // Global data
  //
  RTLIL::Design *G_design = NULL;
  string csv_stat_file;
  bool dot;

  // Methods
  //
  ReportStatPass()
      : ScriptPass("report_stat",
                   "Dump flattened netlist stats in file 'stat.csv'") {}

  // -------------------------
  // getNumberOfDffs
  // -------------------------
  int getNumberOfLuts() {

    int nb = 0;

    for (auto cell : G_design->top_module()->cells()) {

      // Generic Luts like for Zero Asic Z1000
      //
      if (cell->type.in(ID($lut))) {
        nb++;
        continue;
      }

      // Xilinx 'xc4v' Luts
      //
      if (cell->type.in(ID(LUT1), ID(LUT2), ID(LUT3), ID(LUT4), ID(INV))) {
        nb++;
        continue;
      }
      if (cell->type.in(ID(MUXF5), ID(MUXF6), ID(MUXF7))) {
        nb++;
        continue;
      }

      // Xilinx 'xc7' Luts
      //
      if (cell->type.in(ID(LUT1), ID(LUT2), ID(LUT3), ID(LUT4), ID(LUT5),
                        ID(LUT6), ID(INV))) {
        nb++;
        continue;
      }

      // Lattice 'Mach xo2' Luts
      //
      if (cell->type.in(ID(LUT1), ID(LUT2), ID(LUT3), ID(LUT4))) {
        nb++;
        continue;
      }

      // ice40 'hx' Luts, carry cells
      //
      if (cell->type.in(ID(SB_LUT4))) {
        nb++;
        continue;
      }
      if (cell->type.in(ID(SB_CARRY))) {
        nb++;
        continue;
      }

      // Quicklogic 'pp3' Luts
      //
      if (cell->type.in(ID(LUT1), ID(LUT2), ID(LUT3), ID(LUT4))) {
        nb++;
        continue;
      }

      // Quicklogic 'pp3' mux4x0
      //
      if (cell->type.in(ID(mux4x0))) {
        nb += 3; // equivalent to 3 LUT3 (Mux)
        continue;
      }

      // Quicklogic 'pp3' mux8x0
      //
      if (cell->type.in(ID(mux8x0))) {
        nb += 7; // equivalent to 7 LUT3 (Mux)
        continue;
      }

      // Microchip 'polarfire' Luts
      //
      if (cell->type.in(ID(CFG1), ID(CFG2), ID(CFG3), ID(CFG4))) {
        nb++;
        continue;
      }

      // Intel 'cycloneiv' Luts
      //
      if (cell->type.in(ID($not), ID(cycloneiv_lcell_comb))) {
        nb++;
        continue;
      }
    }

    return nb;
  }

  // -------------------------
  // getNumberOfDffs
  // -------------------------
  int getNumberOfDffs() {

    int nb = 0;

    for (auto cell : G_design->top_module()->cells()) {

      // Zero Asic Z1000/Z1010 DFFs
      //
      if (cell->type.in(ID(dffer), ID(dffes), ID(dffe), ID(dffr), ID(dffs),
                        ID(dff))) {
        nb++;
      }

      if (cell->type.in(ID(dffh), ID(dffeh), ID(dffl), ID(dffel), ID(dffhl),
                        ID(dffehl))) {
        nb++;
      }

      // Xilinx 'xc4v' DFFs, 'xc7'
      //
      if (cell->type.in(ID(FDCE), ID(FDPE), ID(FDRE), ID(FDRE_1), ID(FDSE),
                        ID(LDCE))) { // LATCH !!!
        nb++;
        continue;
      }

      // Lattice 'Mach ox2' DFFs
      //
      if (cell->type.in(ID(TRELLIS_FF))) {
        nb++;
        continue;
      }

      // ice40 'hx' DFFs
      //
      if (cell->type.in(ID(SB_DFF), ID(SB_DFFE), ID(SB_DFFER), ID(SB_DFFESR),
                        ID(SB_DFFESS), ID(SB_DFFN), ID(SB_DFFR), ID(SB_DFFS),
                        ID(SB_DFFSR), ID(SB_DFFES))) {
        nb++;
        continue;
      }

      // Quicklogic 'pp3' DFFs
      //
      if (cell->type.in(ID(dffepc))) {
        nb++;
        continue;
      }

      // Microchip 'polarfire' Luts
      //
      if (cell->type.in(ID(SLE))) {
        nb++;
        continue;
      }

      // Intel 'cycloneiv' DFFs
      //
      if (cell->type.in(ID(dffeas))) {
        nb++;
        continue;
      }

      // rest of the world
      //
      if (cell->type.in(ID(dffer_pxp))) {
        nb++;
        continue;
      }
      if (cell->type.in(ID(dffes_xpp))) {
        nb++;
        continue;
      }

      if ((cell->type).substr(0, 5) == "$_DFF") {
        nb++;
        continue;
      }

      if ((cell->type).substr(0, 6) == "$_SDFF") {
        nb++;
        continue;
      }
    }

    return nb;
  }

  // -------------------------
  // getNumberOfDSPs
  // -------------------------
  int getNumberOfDSPs() {

    int nb = 0;

    for (auto cell : G_design->top_module()->cells()) {

      // Xilinx xc4v
      //
      if (cell->type.in(ID(DSP48))) {
        nb++;
        continue;
      }

      // Xilinx xc5v
      //
      if (cell->type.in(ID(DSP48E))) {
        nb++;
        continue;
      }

      if (cell->type.in(ID(DSP48E1))) {
        nb++;
        continue;
      }

      // Ice40
      //
      if (cell->type.in(ID(SB_MAC16))) {
        nb++;
        continue;
      }

      // Microchip
      //
      if (cell->type.in(ID(MACC_PA))) {
        nb++;
        continue;
      }

      // ZeroAsic
      //
      // Make sure it is in sync. with file under :
      //    wildebeest/architecture/z1010/dsp/zeroasic_dsp_map_mode.v
      //
      // OR for instance
      //
      //    architecture/z1010/models/tech_mae.v
      //
      if (cell->type.in(ID(efpga_adder))) {
        nb++;
        continue;
      }
      if (cell->type.in(ID(efpga_adder_regi))) {
        nb++;
        continue;
      }
      if (cell->type.in(ID(efpga_adder_rego))) {
        nb++;
        continue;
      }
      if (cell->type.in(ID(efpga_adder_regio))) {
        nb++;
        continue;
      }
      if (cell->type.in(ID(efpga_acc_regi))) {
        nb++;
        continue;
      }
      if (cell->type.in(ID(efpga_acc))) {
        nb++;
        continue;
      }
      if (cell->type.in(ID(efpga_mult))) {
        nb++;
        continue;
      }
      if (cell->type.in(ID(efpga_mult_regi))) {
        nb++;
        continue;
      }
      if (cell->type.in(ID(efpga_mult_rego))) {
        nb++;
        continue;
      }
      if (cell->type.in(ID(efpga_mult_regio))) {
        nb++;
        continue;
      }
      if (cell->type.in(ID(efpga_mult_addc_regi))) {
        nb++;
        continue;
      }
      if (cell->type.in(ID(efpga_mult_addc_rego))) {
        nb++;
        continue;
      }
      if (cell->type.in(ID(efpga_mult_addc_regio))) {
        nb++;
        continue;
      }
      if (cell->type.in(ID(efpga_mult_addc))) {
        nb++;
        continue;
      }
      if (cell->type.in(ID(efpga_macc_pipe))) {
        nb++;
        continue;
      }
      if (cell->type.in(ID(efpga_macc_pipe_regi))) {
        nb++;
        continue;
      }
      if (cell->type.in(ID(efpga_macc_regi))) {
        nb++;
        continue;
      }
      if (cell->type.in(ID(efpga_macc))) {
        nb++;
        continue;
      }
    }

    return nb;
  }

  // -------------------------
  // getNumberOfBRAMs
  // -------------------------
  int getNumberOfBRAMs() {

    int nb = 0;

    for (auto cell : G_design->top_module()->cells()) {

      // Xilinx xc4v
      //
      if (cell->type.in(ID(RAMB16))) {
        nb++;
        continue;
      }

      // Xilinx xc7
      //
      if (cell->type.in(ID(RAM32M))) {
        nb++;
        continue;
      }

      // Ice40
      //
      if (cell->type.in(ID(SB_RAM40_4K))) {
        nb++;
        continue;
      }

      // Lattice xo2
      //
      if (cell->type.in(ID(DP8KC))) {
        nb++;
        continue;
      }

      // Intel cycloneiv
      //
      if (cell->type.in(ID(altsyncram))) {
        nb++;
        continue;
      }

      // Microchip polarfire
      //
      if (cell->type.in(ID(RAM1K20))) {
        nb++;
        continue;
      }

      // Zeroasic
      //
      if (cell->type.in(
              // New Peter's RAM coming from :
              //      architecture/z1010/bram/techmap.v
              //
              // sprams
              //
              ID(spram_512x32), ID(spram_1024x16), ID(spram_2048x8),
              ID(spram_4096x4), ID(spram_8192x2), ID(spram_16384x1),

              // sdprams
              //
              ID(sdpram_1024x16), ID(sdpram_2048x8), ID(sdpram_4096x4),
              ID(sdpram_8192x2), ID(sdpram_16384x1), ID(sdpram_16384x1),

              // tdprams
              //
              ID(tdpram_1024x16), ID(tdpram_2048x8), ID(tdpram_4096x4),
              ID(tdpram_8192x2), ID(tdpram_16384x1),

              // Old RAM names
              //
              ID(spram_512x64), ID(spram_1024x32), ID(spram_2048x16),
              ID(spram_4096x8), ID(spram_8192x4), ID(spram_16384x2),
              ID(spram_32768x1),

              ID(sdpram_1024x32), ID(sdpram_2048x16), ID(sdpram_4096x8),
              ID(sdpram_8192x4), ID(sdpram_16384x2), ID(sdpram_32768x1),

              ID(tdpram_1024x32), ID(tdpram_2048x16), ID(tdpram_4096x8),
              ID(tdpram_8192x4), ID(tdpram_16384x2), ID(tdpram_32768x1))) {
        nb++;
        continue;
      }

      // rest of the world
      //
      if (cell->type.in(ID(RAM64x12))) {
        nb++;
        continue;
      }
    }

    return nb;
  }

  // -------------------------
  void help() override {
    //   |---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|
    log("\n");
    log("    report_stat\n");
    log("\n");
    log("This command reports stat on a flattened netlist. Data are dumped.\n");
    log("in file 'stat.csv'.\n");
    log("\n");
    log("    -csv <file>\n");
    log("        write design statistics into a CSV file. Default file name\n");
    log("        is 'stat.csv'.\n");
    log("\n");
  }

  void clear_flags() override {
    csv_stat_file = "stat.csv";
    dot = false;
  }

  void execute(std::vector<std::string> args, RTLIL::Design *design) override {
    string run_from, run_to;
    clear_flags();

    G_design = design;

    size_t argidx;
    for (argidx = 1; argidx < args.size(); argidx++) {
      if (args[argidx] == "-csv" && argidx + 1 < args.size()) {
        csv_stat_file = args[++argidx];
        continue;
      }
      if (args[argidx] == "-dot") {
        dot = true;
        continue;
      }
    }
    extra_args(args, argidx, design);

    if (!design->full_selection()) {
      log_cmd_error("This command only operates on fully selected designs!\n");
    }

    log_header(design, "Executing 'report_stat'.\n");
    log_push();

    run_script(design, run_from, run_to);

    log_pop();
  }

  // ---------------------------------------------------------------------------
  // report_stat
  // ---------------------------------------------------------------------------
  void script() override {
    if (!G_design) {
      log_warning("Design seems empty ! (did you define the -top or use "
                  "'hierarchy -auto-top' before)\n");
      return;
    }

    Module *topModule = G_design->top_module();

    if (!topModule) {
      log_warning("Design seems empty ! (did you define the -top or use "
                  "'hierarchy -auto-top' before)\n");
      return;
    }

    string topName = log_id(topModule->name);

    int nbLuts = getNumberOfLuts();

    int nbDffs = getNumberOfDffs();

    int nbDSPs = getNumberOfDSPs();

    int nbBRAMs = getNumberOfBRAMs();

    int maxlvl = -1;
    int maxheight = -1;

    // call 'max_level' command if not called yet
    //
    run("max_level -clk2clk"); // -> store 'maxlvl' in scratchpad with
                               // 'max_level.max_levels'

    if (dot) {
      run("max_height -dot"); // -> store 'maxheight' in scratchpad with
                              // 'max_height.max_height' NOTE: we can use 'xdot'
                              // to view the dot file
    } else {
      run("max_height"); // -> store 'maxheight' in scratchpad with
                         // 'max_height.max_height'
    }

    maxlvl = G_design->scratchpad_get_int("max_level.max_levels", 0);
    maxheight = G_design->scratchpad_get_int("max_height.max_height", 0);

    if (maxlvl != maxheight) {
      log_warning(
          "Max level and Max height computations give different values !\n");
    }

    string start = G_design->scratchpad_get_string("time_chrono_start");
    string end = G_design->scratchpad_get_string("time_chrono_end");

    double d_start = atof(start.c_str());
    double d_end = atof(end.c_str());

    int duration = (int)(d_end - d_start) + 1;

    log("\n");

#if 0
    log("   Start time = %s\n", start.c_str());
    log("   End time   = %s\n", end.c_str());
#endif

    log("   Duration   = %d sec.\n", duration);

    // -----
    // Open the csv file and dump the stats.
    //
    std::ofstream csv_file(csv_stat_file);

    csv_file << topName + ",";
    csv_file << std::to_string(nbLuts) + ",";
    csv_file << std::to_string(nbDffs) + ",";
    csv_file << std::to_string(nbDSPs) + ",";
    csv_file << std::to_string(nbBRAMs) + ",";
    csv_file << std::to_string(maxheight) + ",";
    csv_file << std::to_string(duration);
    csv_file << std::endl;

    csv_file.close();

    log("\n   Dumped file %s\n", csv_stat_file.c_str());

    run("stat");

  } // end script()

} ReportStatPass;

PRIVATE_NAMESPACE_END
