#include "kernel/celltypes.h"
#include "kernel/sigtools.h"
#include "kernel/yosys.h"
#include <cassert>
#include <chrono>

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

static bool clk2clk = false;
static bool summary = false;

struct MaxLvlPass : public ScriptPass {

  RTLIL::Design *G_design = NULL;

  struct MaxLvlWorker {
    RTLIL::Design *design;
    RTLIL::Module *module;
    SigMap sigmap;

    // Bit info attached to a SigBit:
    //    - level
    //    - sigBit
    //    - driven cell from the sigbit
    //    - heigth
    //
    dict<SigBit, tuple<int, SigBit, Cell *, int>> bits;

    // Traversal table : from a sigBit gives the fanout
    // SigBits
    //
    dict<SigBit, dict<SigBit, Cell *>> bit2bits;

    dict<SigBit, tuple<SigBit, Cell *>> bit2ff;

    pool<SigBit> cps;
    pool<SigBit> driven_bits;

    int maxlvl;
    int max_heigth;
    SigBit maxbit;

    pool<SigBit> visited_bits;

    // ---------------------------------------
    // setup_internals_zeroasic_clocked_cells
    // ---------------------------------------
    void setup_internals_zeroasic_clocked_cells(CellTypes &ff_celltypes) {
      // Simply list the DFF cells names is enough as cut points
      //
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

      // Clocked DSPs
      //
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

    // ------------------------------------
    // setup_internals_xilinx_ff_xc4v
    // ------------------------------------
    void setup_internals_xilinx_ff_xc4v(CellTypes &ff_celltypes) {
      // Simply list the DFF cells names is enough as cut points
      //
      ff_celltypes.setup_type(ID(FDCE), {}, {});
      ff_celltypes.setup_type(ID(FDPE), {}, {});
      ff_celltypes.setup_type(ID(FDRE), {}, {});
      ff_celltypes.setup_type(ID(FDRE_1), {}, {});
      ff_celltypes.setup_type(ID(FDSE), {}, {});
      ff_celltypes.setup_type(ID(LDCE), {}, {});
    }
    void setup_internals_xilinx_io_xc4v(CellTypes &ff_celltypes) {
      ff_celltypes.setup_type(ID(IBUF), {}, {});
      ff_celltypes.setup_type(ID(OBUF), {}, {});
    }
    void setup_internals_xilinx_bram(CellTypes &ff_celltypes) {
      ff_celltypes.setup_type(ID(RAMB16), {}, {});
      ff_celltypes.setup_type(ID(RAM32M), {}, {});
      ff_celltypes.setup_type(ID(RAM64x12), {}, {});
      ff_celltypes.setup_type(ID(OBUF), {}, {});
    }

    // ------------------------------------
    // setup_internals_lattice_ff_xo2
    // ------------------------------------
    void setup_internals_lattice_ff_xo2(CellTypes &ff_celltypes) {
      // Simply list the DFF cells names is enough as cut points
      //
      ff_celltypes.setup_type(ID(TRELLIS_FF), {}, {});
    }
    void setup_internals_lattice_bram_xo2(CellTypes &ff_celltypes) {
      // Simply list the BRAM cells names is enough as cut points
      //
      ff_celltypes.setup_type(ID(DP8KC), {}, {});
    }

    // ------------------------------------
    // setup_internals_ice40_ff_hx
    // ------------------------------------
    void setup_internals_ice40_ff_hx(CellTypes &ff_celltypes) {
      // Simply list the DFF cells names is enough as cut points
      //
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

    // ------------------------------------
    // setup_internals_quicklogic_ff_pp3
    // ------------------------------------
    void setup_internals_quicklogic_ff_pp3(CellTypes &ff_celltypes) {
      // Simply list the DFF cells names is enough as cut points
      //
      ff_celltypes.setup_type(ID(dffepc), {}, {});
    }

    // ---------------------------------------
    // setup_internals_microchip_ff_polarfire
    // ---------------------------------------
    void setup_internals_microchip_ff_polarfire(CellTypes &ff_celltypes) {
      // Simply list the DFF cells names is enough as cut points
      //
      ff_celltypes.setup_type(ID(SLE), {}, {});
    }
    void setup_internals_microchip_bram_polarfire(CellTypes &ff_celltypes) {
      // Simply list the BRAM cells names is enough as cut points
      //
      ff_celltypes.setup_type(ID(RAM1K20), {}, {});
    }
    void setup_internals_microchip_cells_polarfire(CellTypes &ff_celltypes) {
      // Simply list the cells names is enough as cut points
      //
      ff_celltypes.setup_type(ID(INBUF), {}, {});
      ff_celltypes.setup_type(ID(OUTBUF), {}, {});
      ff_celltypes.setup_type(ID(SLE), {}, {});
      ff_celltypes.setup_type(ID(CLKINT), {}, {});
    }

    // ------------------------------------
    // setup_internals_intel_ff_cycloneiv
    // ------------------------------------
    void setup_internals_intel_ff_cycloneiv(CellTypes &ff_celltypes) {
      // Simply list the DFF cells names is enough as cut points
      //
      ff_celltypes.setup_type(ID(dffeas), {}, {});
    }

    // ---------------------
    // MaxLvlWorker
    // ---------------------
    MaxLvlWorker(RTLIL::Module *module)
        : design(module->design), module(module), sigmap(module) {
      CellTypes ff_celltypes;

      if (clk2clk) { // define cells that are cutpoints during the traversal.

        ff_celltypes.setup_internals_mem();
        ff_celltypes.setup_stdcells_mem();

        // Specify technology related DFF cutpoints for -clk2clk option
        //
        setup_internals_zeroasic_clocked_cells(ff_celltypes);

        // Xilinx
        //
        setup_internals_xilinx_ff_xc4v(ff_celltypes);
        setup_internals_xilinx_bram(ff_celltypes);
        setup_internals_xilinx_io_xc4v(ff_celltypes);

        // Lattice
        //
        setup_internals_lattice_ff_xo2(ff_celltypes);
        setup_internals_lattice_bram_xo2(ff_celltypes);

        // ICE40
        //
        setup_internals_ice40_ff_hx(ff_celltypes);

        // QuickLogic
        //
        setup_internals_quicklogic_ff_pp3(ff_celltypes);

        // Microchip
        //
        setup_internals_microchip_ff_polarfire(ff_celltypes);
        setup_internals_microchip_bram_polarfire(ff_celltypes);
        setup_internals_microchip_cells_polarfire(ff_celltypes);

        // Intel (Altera)
        //
        setup_internals_intel_ff_cycloneiv(ff_celltypes);
      }

      // For all SigBits create their associated 'bitInfo'
      // to store <level, from bit, Cell>
      //
      for (auto wire : module->selected_wires()) {

        for (auto bit : sigmap(wire)) {

          bits[bit] =
              tuple<int, SigBit, Cell *, int>(-1, State::Sx, nullptr, -1);
        }
      }

      // For all the traversable cells (not in 'ff_celltypes')
      // add the 'src' bits to 'dest_bits( relationship into the
      // traversable table 'bit2bits'.
      //
      for (auto cell : module->selected_cells()) {

        pool<SigBit> src_bits, dst_bits;

        // For the current Cell 'cell' build the 'src_bits'
        // to the 'dest_bits'.
        //
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

        // If It is a DFF that we know then we consider it as a
        // cut point in the traversal and we will not add the
        // src bit to dst bits in the 'bit2bits' forward
        // traversal table.
        //
        if (clk2clk && ff_celltypes.cell_known(cell->type)) {

          for (auto s : src_bits) {

            for (auto d : dst_bits) {
              bit2ff[s] = tuple<SigBit, Cell *>(d, cell);
              break;
            }
          }

          continue;
        }

        // Add the 'src_bits' to the 'dst_bits' relationship
        // into 'bit2bits' table.
        //
        // This table will be used for the traversal in 'runner'
        // to compute all the 'bit' levels..
        //
        for (auto s : src_bits) {

          for (auto d : dst_bits) {
            bit2bits[s][d] = cell;
          }
        }
      }

      maxlvl = -1;
      maxbit = State::Sx;
    }

    // ---------------------
    // get_heigth
    // ---------------------
    int get_heigth(SigBit bit) {
      auto &bitinfo = bits.at(bit);

      int heigth = get<3>(bitinfo);

      if (heigth >= 0) {
        return heigth;
      }

      if (!get<2>(bitinfo)) { // if 'bit' has no cell driving it it is a PI
        get<3>(bitinfo) = 0;
        return 0;
      }

      if (visited_bits.count(bit) > 0) {
        get<3>(bitinfo) = 0;
        log_warning("Detected loop at %s in %s\n", log_signal(bit),
                    log_id(module));
        return heigth;
      }

      visited_bits.insert(bit);

      heigth = get_heigth(get<1>(bitinfo));

      heigth += 1;

      get<3>(bitinfo) = heigth;

      return heigth;
    }

    // ---------------------
    // runner
    // ---------------------
    void runner(SigBit bit, int level, SigBit from, Cell *via) {
      auto &bitinfo = bits.at(bit);

      if (get<0>(bitinfo) >= level) {
        return;
      }

      // If bit already visited ...
      //
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

    // ---------------------
    // printpath
    // ---------------------
    // From input to output.
    //
    void printpath(SigBit bit) {
      auto &bitinfo = bits.at(bit);

      // If the SigBit 'bit' has a cell driving it
      //
      if (get<2>(bitinfo)) {

        // Print recursively the 'from' bit, e.g the SigBit
        // on the critical path driving the cell
        //
        // Since we first print the driving sigBit, we print
        // the path from Input to Output.
        //
        printpath(get<1>(bitinfo));

        Cell *cell = get<2>(bitinfo);

        log("%5d: %s (via %s)\n", get<0>(bitinfo), log_signal(bit),
            log_id(cell->type));

      } else {

        log("%5d: %s\n", get<0>(bitinfo), log_signal(bit));
      }
    }

    // ---------------------
    // get_cps_rec
    // ---------------------
    void get_cps_rec(SigBit bit, int heigth) {
      auto &bitinfo = bits.at(bit);

      // Bit already traversed and TFI added in 'cps'
      //
      if (cps.count(bit)) {
        return;
      }

      // return if a PI
      //
      if (get<3>(bitinfo) == 0) {
        return;
      }

      // If on the CP then add it
      //
      if (get<3>(bitinfo) == heigth) {
        assert(get<2>(bitinfo)); // make sure it is driven
        cps.insert(bit);
      }

      // return if not on the CP
      //
      if (get<3>(bitinfo) < heigth) {
        return;
      }

      visited_bits.insert(bit);

      SigBit from = get<1>(bitinfo);

      // Collect 'from' bit with heigth 'heigth-1'
      //
      get_cps_rec(from, heigth - 1);
    }

    // ---------------------
    // get_cps
    // ---------------------
    void get_cps() {
      for (auto &it : bits) {

        if (get<3>(it.second) == max_heigth) {
          get_cps_rec(it.first, max_heigth);
        }
      }
    }

    // ---------------------
    // get_driven_bits
    // ---------------------
    void get_driven_bits() {
      for (auto &it : bits) {

        if (get<2>(it.second)) {
          driven_bits.insert(it.first);
        }
      }
    }

    // ---------------------
    // run
    // ---------------------
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

      max_heigth = -1;

      for (auto &it : bits) {

        visited_bits.clear();

        if (get<3>(it.second) < 0) {

          int heigth = get_heigth(it.first);

          if (heigth > max_heigth) {
            max_heigth = heigth;
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
        log("   Max heigth      = %d\n", max_heigth);
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
        log("   Max heigth      = %d\n", max_heigth);
        log("   CP size         = %ld\n", cps.size());
        log("   Total bits      = %ld\n", driven_bits.size());
      }
    }
  };

  MaxLvlPass() : ScriptPass("max_level", "print max logic level") {}

  // -------------------------
  // load_LUT_models
  // -------------------------
  void load_LUT_models() {
    run("read_verilog +/plugins/wildebeest/lut_models/LUTs.v");

    run("hierarchy -auto-top");
  }

  // ---------------------
  // help
  // ---------------------
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

  // ---------------------
  // clear_flags
  // ---------------------
  void clear_flags() override {
    clk2clk = false;
    summary = false;
  }

  // ---------------------
  // execute
  // ---------------------
  void execute(std::vector<std::string> args, RTLIL::Design *design) override {
    string run_from, run_to;

    log_header(design,
               "Executing 'max_level' command (find max logic level).\n");
    clear_flags();

    size_t argidx;

    G_design = design;

    for (argidx = 1; argidx < args.size(); argidx++) {

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

  // ---------------------
  // script
  // ---------------------
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
