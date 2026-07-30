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
#include <ctime>

#define SYNTH_FPGA_VERSION "1.0"

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct TimeChronoPass : public ScriptPass {
  // Global data
  //
  RTLIL::Design *G_design = NULL;
  bool start, end;

  // Methods
  //
  TimeChronoPass() : ScriptPass("time_chrono", "stores current time") {}

  void help() override {
    //   |---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|
    log("\n");
    log("    time_chrono\n");
    log("\n");
    log("This command stores time in order to use it afterward in "
        "'report_stat'\n");
    log("command.\n");
    log("\n");
  }

  void clear_flags() override {
    start = false;
    end = false;
  }

  void execute(std::vector<std::string> args, RTLIL::Design *design) override {
    string run_from, run_to;
    clear_flags();

    G_design = design;

    size_t argidx;
    for (argidx = 1; argidx < args.size(); argidx++) {
      if (args[argidx] == "-start") {
        start = true;
        continue;
      }
      if (args[argidx] == "-end") {
        end = true;
        continue;
      }
    }
    extra_args(args, argidx, design);

    if (!design->full_selection()) {
      log_cmd_error("This command only operates on fully selected designs!\n");
    }

    log_header(design, "Executing 'time_chrono'.\n");
    log_push();

    run_script(design, run_from, run_to);

    log_pop();
  }

  // ---------------------------------------------------------------------------
  // time_chrono
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

    time_t timestamp;
    time(&timestamp);

    char output[100];
    struct tm *datetime;

    string ftime = ctime(&timestamp);

    datetime = localtime(&timestamp);

    // Store whole number is seconds with format "%s" !
    //
    strftime(output, 100, "%s", datetime);

    if (start) {

      G_design->scratchpad_set_string("time_chrono_start", string(output));
#if 0
      log("\n");
      log("   Start Time = %s\n", output);
#endif
      return;

    } else if (end) {

      G_design->scratchpad_set_string("time_chrono_end", string(output));
#if 0
      log("\n");
      log("   End Time = %s\n", output);
#endif
      return;
    }

    log_warning("time_chrono command has no effect without an option !\n");

  } // end script()

} TimeChronoPass;

PRIVATE_NAMESPACE_END
