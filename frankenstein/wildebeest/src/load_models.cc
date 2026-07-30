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
#include <chrono>

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct LoadModelsPass : public ScriptPass {
  // Global data
  //

  // Methods
  //
  LoadModelsPass() : ScriptPass("load_models", "Load Zero  Asic RTL models") {}

  void help() override {
    //   |---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|
    log("\n");
    log("    load_models\n");
    log("\n");

    help_script();
    log("\n");
  }

  void clear_flags() override {}

  void execute(std::vector<std::string> args, RTLIL::Design *design) override {
    string run_from, run_to;
    clear_flags();

    size_t argidx;
    for (argidx = 1; argidx < args.size(); argidx++) {
    }
    extra_args(args, argidx, design);

    log_header(design, "Executing Zero Asic load models.\n");
    log_push();

    run_script(design, run_from, run_to);

    log_pop();
  }

  // ---------------------------------------------------------------------------
  // load_models
  // ---------------------------------------------------------------------------
  //
  // Obsolete since file names have changed. Needs to be revisited. (Thierry)
  //
  void script() override {
    run("read_verilog +/plugins/wildebeest/ff_models/dff.v");
    run("read_verilog +/plugins/wildebeest/ff_models/dffe.v");
    run("read_verilog +/plugins/wildebeest/ff_models/dffr.v");
    run("read_verilog +/plugins/wildebeest/ff_models/dffs.v");
    run("read_verilog +/plugins/wildebeest/ff_models/dffrs.v");
    run("read_verilog +/plugins/wildebeest/ff_models/dffer.v");
    run("read_verilog +/plugins/wildebeest/ff_models/dffes.v");
    run("read_verilog +/plugins/wildebeest/ff_models/dffers.v");

    run("read_verilog +/plugins/wildebeest/ram_models/RAM64x12.v");

  } // end script()

} LoadModelsPass;

PRIVATE_NAMESPACE_END
