// max_level pass from wildebeest, kept under mosaic so VTR can cut timing paths
// at arch clocked hardblocks. the mosaic-specific piece is -vtr_arch, which
// registers is_clock models from the arch xml without linking libarchfpga.

#include "kernel/celltypes.h"
#include "kernel/sigtools.h"
#include "kernel/yosys.h"
#include <cassert>
#include <chrono>

#ifdef YOSYS_ENABLE_TCL
#include <tcl.h>
#endif

#include "vtr_arch_clocks.h"

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

static bool clk2clk = false;
static bool summary = false;
static std::string vtr_arch_file = "";

struct MaxLvlPass : public ScriptPass {

  RTLIL::Design *G_design = NULL;

  struct MaxLvlWorker {
    RTLIL::Design *design;
    RTLIL::Module *module;
    SigMap sigmap;

    // longest path search needs level, predecessor, driving cell, and height
    // for each bit so critical path reconstruction can walk backward later.
    dict<SigBit, tuple<int, SigBit, Cell *, int>> bits;

    // fanout edges let the level walk expand only to bits that are actually driven.
    dict<SigBit, dict<SigBit, Cell *>> bit2bits;

    // clocked cell crossings become cut points when clk2clk is enabled so path
    // length does not continue across sequential boundaries.
    dict<SigBit, tuple<SigBit, Cell *>> bit2ff;

    pool<SigBit> cps;
    pool<SigBit> driven_bits;

    int maxlvl;
    int max_height;
    SigBit maxbit;

    pool<SigBit> visited_bits;

    // HELPER: register clk2clk cut points from VTR arch xml clocked models.
    // MOSAIC uses the same is_clock model list as PARMYS without linking VTR libs.
    void setup_internals_vtr_arch(CellTypes &ff_celltypes,
                                  const std::string &xml_file) {
      log("Deriving clk2clk cut points from vtr arch xml: %s\n",
          xml_file.c_str());

      std::string parse_error;
      std::vector<std::string> clocked_models =
          mosaic::readClockedModelNames(xml_file, &parse_error);

      if (!parse_error.empty()) {
        log_error("max_level -vtr_arch: %s\n", parse_error.c_str());
      }

      if (clocked_models.empty()) {
        log_warning("max_level -vtr_arch: no clocked models found in %s\n",
                    xml_file.c_str());
      }

      for (const std::string &model_name : clocked_models) {
        IdString cell_type = RTLIL::escape_id(model_name);
        ff_celltypes.setup_type(cell_type, {}, {});
        log("  -> registered vtr clocked primitive cut point: %s\n",
            model_name.c_str());
      }
    }

    // HELPER: register ZeroASIC clocked cell types as clk2clk cut points.
    void setup_internals_zeroasic_clocked_cells(CellTypes &ff_celltypes) {
      ff_celltypes.setup_type(ID(dffehl), {}, {});
      ff_celltypes.setup_type(ID(dffeh), {}, {});
      ff_celltypes.setup_type(ID(dffel), {}, {});
      ff_celltypes.setup_type(ID(dffer), {}, {});
      ff_celltypes.setup_type(ID(dffes), {}, {});
      ff_celltypes.setup_type(ID(dffe), {}, {});
      ff_celltypes.setup_type(ID(dffhl), {}, {});
      ff_celltypes.setup_type(ID(dffh), {}, {});
      ff_celltypes.setup_type(ID(dffl), {}, {});
      ff_celltypes.setup_type(ID(dffr), {}, {});
      ff_celltypes.setup_type(ID(dffs), {}, {});
      ff_celltypes.setup_type(ID(dff), {}, {});

      ff_celltypes.setup_type(ID(efpga_adder_regi), {}, {});
      ff_celltypes.setup_type(ID(efpga_adder_rego), {}, {});
      ff_celltypes.setup_type(ID(efpga_adder_regio), {}, {});

      ff_celltypes.setup_type(ID(efpga_acc_regi), {}, {});

      ff_celltypes.setup_type(ID(efpga_mult_regi), {}, {});
      ff_celltypes.setup_type(ID(efpga_mult_rego), {}, {});
      ff_celltypes.setup_type(ID(efpga_mult_regio), {}, {});

      ff_celltypes.setup_type(ID(efpga_macc_regi), {}, {});
      ff_celltypes.setup_type(ID(efpga_macc_pipe_regi), {}, {});

      ff_celltypes.setup_type(ID(efpga_mult_addc_regio), {}, {});
      ff_celltypes.setup_type(ID(efpga_mult_addc_regi), {}, {});
      ff_celltypes.setup_type(ID(efpga_mult_addc_rego), {}, {});
    }

    // HELPER: register Xilinx XC4V flip-flop types as clk2clk cut points.
    void setup_internals_xilinx_ff_xc4v(CellTypes &ff_celltypes) {
      ff_celltypes.setup_type(ID(FDCE), {}, {});
      ff_celltypes.setup_type(ID(FDPE), {}, {});
      ff_celltypes.setup_type(ID(FDRE), {}, {});
      ff_celltypes.setup_type(ID(FDRE_1), {}, {});
      ff_celltypes.setup_type(ID(FDSE), {}, {});
      ff_celltypes.setup_type(ID(LDCE), {}, {});
    }
    // HELPER: register Xilinx XC4V IO cells as clk2clk cut points.
    void setup_internals_xilinx_io_xc4v(CellTypes &ff_celltypes) {
      ff_celltypes.setup_type(ID(IBUF), {}, {});
      ff_celltypes.setup_type(ID(OBUF), {}, {});
    }
    // HELPER: register Xilinx BRAM types as clk2clk cut points.
    void setup_internals_xilinx_bram(CellTypes &ff_celltypes) {
      ff_celltypes.setup_type(ID(RAMB16), {}, {});
      ff_celltypes.setup_type(ID(RAM32M), {}, {});
      ff_celltypes.setup_type(ID(RAM64x12), {}, {});
      ff_celltypes.setup_type(ID(OBUF), {}, {});
    }

    // HELPER: register Lattice XO2 flip-flop types as clk2clk cut points.
    void setup_internals_lattice_ff_xo2(CellTypes &ff_celltypes) {
      ff_celltypes.setup_type(ID(TRELLIS_FF), {}, {});
    }
    // HELPER: register Lattice XO2 BRAM types as clk2clk cut points.
    void setup_internals_lattice_bram_xo2(CellTypes &ff_celltypes) {
      ff_celltypes.setup_type(ID(DP8KC), {}, {});
    }

    // HELPER: register iCE40 HX flip-flop types as clk2clk cut points.
    void setup_internals_ice40_ff_hx(CellTypes &ff_celltypes) {
      ff_celltypes.setup_type(ID(SB_DFF), {}, {});
      ff_celltypes.setup_type(ID(SB_DFFE), {}, {});
      ff_celltypes.setup_type(ID(SB_DFFER), {}, {});
      ff_celltypes.setup_type(ID(SB_DFFESR), {}, {});
      ff_celltypes.setup_type(ID(SB_DFFESS), {}, {});
      ff_celltypes.setup_type(ID(SB_DFFN), {}, {});
      ff_celltypes.setup_type(ID(SB_DFFR), {}, {});
      ff_celltypes.setup_type(ID(SB_DFFRS), {}, {});
      ff_celltypes.setup_type(ID(SB_DFFRR), {}, {});
      ff_celltypes.setup_type(ID(SB_DFFS), {}, {});
      ff_celltypes.setup_type(ID(SB_DFFSS), {}, {});
      ff_celltypes.setup_type(ID(SB_DFFSR), {}, {});
      ff_celltypes.setup_type(ID(SB_DFFES), {}, {});
    }

    // HELPER: register QuickLogic PP3 flip-flop types as clk2clk cut points.
    void setup_internals_quicklogic_ff_pp3(CellTypes &ff_celltypes) {
      ff_celltypes.setup_type(ID(dffepc), {}, {});
    }

    // HELPER: register Microchip PolarFire flip-flop types as clk2clk cut points.
    void setup_internals_microchip_ff_polarfire(CellTypes &ff_celltypes) {
      ff_celltypes.setup_type(ID(SLE), {}, {});
    }
    // HELPER: register Microchip PolarFire BRAM types as clk2clk cut points.
    void setup_internals_microchip_bram_polarfire(CellTypes &ff_celltypes) {
      ff_celltypes.setup_type(ID(RAM1K20), {}, {});
    }
    // HELPER: register other Microchip PolarFire clocked cells as cut points.
    void setup_internals_microchip_cells_polarfire(CellTypes &ff_celltypes) {
      ff_celltypes.setup_type(ID(INBUF), {}, {});
      ff_celltypes.setup_type(ID(OUTBUF), {}, {});
      ff_celltypes.setup_type(ID(SLE), {}, {});
      ff_celltypes.setup_type(ID(CLKINT), {}, {});
    }

    // HELPER: register Intel Cyclone IV flip-flop types as clk2clk cut points.
    void setup_internals_intel_ff_cycloneiv(CellTypes &ff_celltypes) {
      ff_celltypes.setup_type(ID(dffeas), {}, {});
    }

    MaxLvlWorker(RTLIL::Module *module)
        : design(module->design), module(module), sigmap(module) {
      CellTypes ff_celltypes;

      if (clk2clk) {
        // register cut points used during level traversal when measuring clk2clk paths.
        ff_celltypes.setup_internals_mem();
        ff_celltypes.setup_stdcells_mem();

        if (!vtr_arch_file.empty()) {
          // MOSAIC path: cut points come from the VTR arch xml instead of vendor lists.
          setup_internals_vtr_arch(ff_celltypes, vtr_arch_file);
        } else {
          setup_internals_zeroasic_clocked_cells(ff_celltypes);

          setup_internals_xilinx_ff_xc4v(ff_celltypes);
          setup_internals_xilinx_bram(ff_celltypes);
          setup_internals_xilinx_io_xc4v(ff_celltypes);

          setup_internals_lattice_ff_xo2(ff_celltypes);
          setup_internals_lattice_bram_xo2(ff_celltypes);

          setup_internals_ice40_ff_hx(ff_celltypes);

          setup_internals_quicklogic_ff_pp3(ff_celltypes);

          setup_internals_microchip_ff_polarfire(ff_celltypes);
          setup_internals_microchip_bram_polarfire(ff_celltypes);
          setup_internals_microchip_cells_polarfire(ff_celltypes);

          setup_internals_intel_ff_cycloneiv(ff_celltypes);
        }
      }

      // initialize per-bit traversal state for every selected wire bit.
      for (auto wire : module->selected_wires()) {
        for (auto bit : sigmap(wire)) {
          bits[bit] =
              tuple<int, SigBit, Cell *, int>(-1, State::Sx, nullptr, -1);
        }
      }

      // build forward fanout table for traversable cells, treating registered cut
      // points as barriers that do not propagate src to dst in bit2bits.
      for (auto cell : module->selected_cells()) {
        pool<SigBit> src_bits, dst_bits;

        for (auto &conn : cell->connections()) {
          for (auto bit : sigmap(conn.second)) {
            if (cell->input(conn.first)) {
              src_bits.insert(bit);
            }
            if (cell->output(conn.first)) {
              dst_bits.insert(bit);
            }
          }
        }

        if (clk2clk && ff_celltypes.cell_known(cell->type)) {
          // record clk2clk ff mapping but do not add cut-point cells to bit2bits.
          for (auto s : src_bits) {
            for (auto d : dst_bits) {
              bit2ff[s] = tuple<SigBit, Cell *>(d, cell);
              break;
            }
          }
          continue;
        }

        for (auto s : src_bits) {
          for (auto d : dst_bits) {
            bit2bits[s][d] = cell;
          }
        }
      }

      maxlvl = -1;
      maxbit = State::Sx;
    }

    // HELPER: compute combinational height of a bit for critical-path analysis.
    int get_height(SigBit bit) {
      auto &bitinfo = bits.at(bit);

      int height = get<3>(bitinfo);

      if (height >= 0) {
        return height;
      }

      if (!get<2>(bitinfo)) {
        // undriven bits are primary inputs and therefore have height zero.
        get<3>(bitinfo) = 0;
        return 0;
      }

      if (visited_bits.count(bit) > 0) {
        get<3>(bitinfo) = 0;
        log_warning("Detected loop at %s in %s\n", log_signal(bit),
                    log_id(module));
        return height;
      }

      visited_bits.insert(bit);

      height = get_height(get<1>(bitinfo));

      height += 1;

      get<3>(bitinfo) = height;

      return height;
    }

    // HELPER: depth-first traversal assigning logic levels along fanout edges.
    void runner(SigBit bit, int level, SigBit from, Cell *via) {
      auto &bitinfo = bits.at(bit);

      if (get<0>(bitinfo) >= level) {
        return;
      }

      if (visited_bits.count(bit) > 0) {
        log_warning("Detected loop at %s in %s\n", log_signal(bit),
                    log_id(module));
        return;
      }

      visited_bits.insert(bit);

      get<0>(bitinfo) = level;
      get<1>(bitinfo) = from;
      get<2>(bitinfo) = via;

      if (level > maxlvl) {
        maxlvl = level;
        maxbit = bit;
      }

      if (bit2bits.count(bit)) {
        for (auto &it : bit2bits.at(bit)) {
          runner(it.first, level + 1, bit, it.second);
        }
      }

      visited_bits.erase(bit);
    }

    // HELPER: print the critical path from inputs to outputs in level order.
    void printpath(SigBit bit) {
      auto &bitinfo = bits.at(bit);

      if (get<2>(bitinfo)) {
        printpath(get<1>(bitinfo));

        Cell *cell = get<2>(bitinfo);

        log("%5d: %s (via %s)\n", get<0>(bitinfo), log_signal(bit),
            log_id(cell->type));

      } else {
        log("%5d: %s\n", get<0>(bitinfo), log_signal(bit));
      }
    }

    // HELPER: recursively collect bits on the critical path at a given height.
    void get_cps_rec(SigBit bit, int height) {
      auto &bitinfo = bits.at(bit);

      if (cps.count(bit)) {
        return;
      }

      if (get<3>(bitinfo) == 0) {
        return;
      }

      if (get<3>(bitinfo) == height) {
        assert(get<2>(bitinfo));
        cps.insert(bit);
      }

      if (get<3>(bitinfo) < height) {
        return;
      }

      visited_bits.insert(bit);

      SigBit from = get<1>(bitinfo);

      get_cps_rec(from, height - 1);
    }

    // HELPER: find all bits that lie on the maximum-height critical path.
    void get_cps() {
      for (auto &it : bits) {
        if (get<3>(it.second) == max_height) {
          get_cps_rec(it.first, max_height);
        }
      }
    }

    // HELPER: collect every bit that is driven by a cell for CP size reporting.
    void get_driven_bits() {
      for (auto &it : bits) {
        if (get<2>(it.second)) {
          driven_bits.insert(it.first);
        }
      }
    }

    // USE: run level and height analysis and log summary or full critical path.
    void run() {
      visited_bits.clear();

      for (auto &it : bits) {
        if (get<0>(it.second) < 0) {
          runner(it.first, 0, State::Sx, nullptr);
        }
      }

      design->scratchpad_set_int("max_level.max_levels", maxlvl);

      auto startTime = std::chrono::high_resolution_clock::now();

      visited_bits.clear();

      max_height = -1;

      for (auto &it : bits) {
        visited_bits.clear();

        if (get<3>(it.second) < 0) {
          int height = get_height(it.first);

          if (height > max_height) {
            max_height = height;
          }
        }
      }

      get_cps();
      get_driven_bits();

      auto endTime = std::chrono::high_resolution_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
          endTime - startTime);

      float totalTime = 1 + elapsed.count() * 1e-9;

      log("[Run Time = %.1f sec.]\n", totalTime);

      if (summary) {
        log("\n");
        log("   Max logic level = %d\n", maxlvl);
        log("   Max height      = %d\n", max_height);
        log("   CP size         = %ld\n", cps.size());
        log("   Total bits      = %ld\n", driven_bits.size());

      } else {
        log("\n");
        log("Max logic level in %s (length=%d):\n", log_id(module), maxlvl);

        if (maxlvl >= 0) {
          printpath(maxbit);
        }

        if (bit2ff.count(maxbit)) {
          log("%5s: %s (via %s)\n", "xx", log_signal(get<0>(bit2ff.at(maxbit))),
              log_id(get<1>(bit2ff.at(maxbit))));
        }

        log("\n");
        log("   Max logic level = %d\n", maxlvl);
        log("   Max height      = %d\n", max_height);
        log("   CP size         = %ld\n", cps.size());
        log("   Total bits      = %ld\n", driven_bits.size());
      }
    }
  };

  MaxLvlPass() : ScriptPass("max_level", "print max logic level") {}

  // HELPER: load arch-independent LUT blackbox models before hierarchy flattening.
  // resolve LUTs.v from templateDir when the Tcl global is set, otherwise from
  // the plugin share dir.
  void load_LUT_models() {
    std::string lut_models = "+/plugins/wildebeest/lut_models/LUTs.v";
#ifdef YOSYS_ENABLE_TCL
    Tcl_Interp *interp = yosys_get_tcl_interp();
    if (interp != NULL) {
      const char *tdir = Tcl_GetVar(interp, "templateDir", TCL_GLOBAL_ONLY);
      if (tdir != NULL && tdir[0] != '\0')
        lut_models = std::string(tdir) + "/lut_models/LUTs.v";
    }
#endif
    run("read_verilog " + lut_models);

    run("hierarchy -auto-top");
  }

  void help() override {
    //   |---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|
    log("\n");
    log("    max_level [options] [selection]\n");
    log("\n");
    log("This command prints the max logic level in the design. (Only "
        "considers\n");
    log("paths within a single module, so the design must be flattened to get "
        "the)\n");
    log("overall longest path in the design.\n");
    log("\n");
    log("    -clk2clk\n");
    log("        Consider longest paths from clocked cell to clocked cell. "
        "They are \n");
    log("        considered as cut points. This is off by default. All cells "
        "are \n");
    log("        traversable by default even DFFs, RAMs, ....\n");
    log("\n");

    log("    -summary\n");
    log("        just print max level number.\n");
    log("\n");
  }

  void clear_flags() override {
    clk2clk = false;
    summary = false;
    vtr_arch_file = "";
  }

  void execute(std::vector<std::string> args, RTLIL::Design *design) override {
    string run_from, run_to;

    log_header(design,
               "Executing 'max_level' command (find max logic level).\n");
    clear_flags();

    size_t argidx;

    G_design = design;

    for (argidx = 1; argidx < args.size(); argidx++) {
      if (args[argidx] == "-vtr_arch" && argidx + 1 < args.size()) {
        vtr_arch_file = args[++argidx];
        continue;
      }

      if (args[argidx] == "-clk2clk") {
        clk2clk = true;
        continue;
      }
      if (args[argidx] == "-summary") {
        summary = true;
        continue;
      }
      break;
    }

    extra_args(args, argidx, design);

    run_script(design, run_from, run_to);
  }

  void script() override {
    load_LUT_models();

    for (Module *module : G_design->selected_modules()) {
      if (module->has_processes_warn()) {
        continue;
      }

      MaxLvlWorker worker(module);
      worker.run();
    }
  }

} MaxLvlPass;

PRIVATE_NAMESPACE_END
