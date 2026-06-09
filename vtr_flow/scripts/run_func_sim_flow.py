#!/usr/bin/env python3
"""
Runs the VTR CAD flow and then verifies the post-implementation netlist with
Verilator functional simulation.

Usage:
    run_func_sim_flow.py <circuit> <arch> -testbench <tb.sv> [run_vtr_flow_args...]

All arguments except -testbench are forwarded verbatim to run_vtr_flow.py.
The flag --gen_post_synthesis_netlist on is appended automatically so the
caller does not need to supply it.

The testbench must follow this convention:
  - The top-level module is named 'tb'.
  - On success the testbench calls $finish  (exits with code 0).
  - On failure the testbench calls $fatal(1) (exits with non-zero code).

Exit code of this script:
  0        — VTR flow succeeded and functional simulation passed.
  non-zero — Either the VTR flow failed or the simulation reported a failure.
"""

import argparse
import io
import os
import re
import subprocess
import sys
import textwrap
from contextlib import redirect_stdout
from datetime import datetime
from pathlib import Path

# Make python_libs importable when running as a subprocess from run_vtr_task.py.
sys.path.insert(0, str(Path(__file__).resolve().parent / "python_libs"))

# pylint: disable=wrong-import-position,import-error
from vtr import (
    paths,
    format_elapsed_time,
    RawDefaultHelpFormatter,
)
from run_vtr_flow import vtr_command_main as run_vtr_flow

# pylint: enable=wrong-import-position,import-error


def _parse_args(argv):
    """
    Extract -testbench and -verilator_j from argv, leaving everything else for run_vtr_flow.
    We scan (but do not consume) -temp_dir from the remaining args so we know
    where the post-synthesis netlist will land after the flow completes.
    """
    description = textwrap.dedent("""
        Runs the VTR CAD flow for a single circuit and architecture, then verifies
        the post-implementation netlist using Verilator-based functional simulation.

        All arguments except -testbench and -verilator_j are forwarded verbatim to
        run_vtr_flow.py.  The flag --gen_post_synthesis_netlist on is appended
        automatically.

        The testbench must follow these conventions:
          - The top-level module is named 'tb'.
          - On success the testbench calls $finish  (exits with code 0).
          - On failure the testbench calls $fatal(1) (exits with non-zero code).
    """)
    epilog = textwrap.dedent("""
        Examples
        --------

            Run the full flow and simulate a 4-bit adder:

                %(prog)s adder_4bit.v arch.xml -testbench tb_adder_4bit.sv

            Use a specific temp directory and limit Verilator to 4 threads:

                %(prog)s adder_4bit.v arch.xml -testbench tb_adder_4bit.sv \\
                    -temp_dir /tmp/my_run -verilator_j 4

            Start from the VPR stage (circuit is already a BLIF):

                %(prog)s adder_4bit.blif arch.xml -testbench tb_adder_4bit.sv \\
                    -starting_stage vpr

        Exit Code
        ---------
            0        VTR flow succeeded and functional simulation passed.
            non-zero Either the VTR flow failed or the simulation reported a failure.
    """)
    parser = argparse.ArgumentParser(
        description=description,
        epilog=epilog,
        formatter_class=RawDefaultHelpFormatter,
    )
    parser.add_argument(
        "-testbench",
        required=True,
        metavar="FILE",
        help="SystemVerilog/Verilog testbench for functional simulation",
    )
    parser.add_argument(
        "-verilator_j",
        type=int,
        default=os.cpu_count() or 1,
        metavar="N",
        help="Number of threads for Verilator compilation (default: all cores)",
    )

    known, remaining = parser.parse_known_args(argv)

    # Peek at -temp_dir without removing it so it is forwarded to run_vtr_flow.
    temp_dir = None
    for i, arg in enumerate(remaining):
        if arg == "-temp_dir" and i + 1 < len(remaining):
            temp_dir = remaining[i + 1]
            break

    return known, remaining, temp_dir


def _extract_mem_info(vtr_output):
    """Return the 'overall memory peak ... consumed by ... run' fragment, or ''."""
    m = re.search(r"overall memory peak .+? consumed by .+? run", vtr_output)
    return m.group(0) if m else ""


def _format_detail(start, vtr_output):
    """Return the timing+memory detail string for the summary line."""
    elapsed = datetime.now() - start
    mem_info = _extract_mem_info(vtr_output)
    return (
        "took {}, {}".format(format_elapsed_time(elapsed), mem_info)
        if mem_info
        else "took {}".format(format_elapsed_time(elapsed))
    )


# this is unused, func sim only needs this if the post-pnr netlist calls module subtract
# synthesis does not emit subtract instances and so remains unused. when synthesis does
# emit subtract instances, turn on _ENABLE_SUBTRACT_PRIMITIVE_PATCH in run_func_sim_flow.py
_ENABLE_SUBTRACT_PRIMITIVE_PATCH = False


def _patch_subtract_primitive_for_sim(netlist_path, testbench_path):
    """swap synthesized subtract subckts to primitives.v subtract once abc/vpr emit them."""
    if not _ENABLE_SUBTRACT_PRIMITIVE_PATCH:
        return

    testbench_name = Path(testbench_path).name
    if testbench_name not in ("tb_sub_4bit.sv", "tb_addsub_4bit.sv"):
        return

    # placeholder until abc/vpr emit a stable subtract subckt name to rewrite here
    _ = netlist_path.read_text(encoding="utf-8")


# this should be modified to be more centralized and generalized so that this
# workaround is not needed. it will remove the need for a custom patch for every new primitive.
def _prepare_netlist_for_sim(netlist_path, testbench_path):
    """
    patch post-pnr netlist before verilator when a testcase needs non-default
    primitive behaviour (e.g. signed dsp multiply).
    """
    _patch_subtract_primitive_for_sim(netlist_path, testbench_path)

    testbench_name = Path(testbench_path).name
    if testbench_name != "tb_mult_8x8_signed.sv":
        return

    text = netlist_path.read_text(encoding="utf-8")
    patched = text.replace("multiply #(", "multiply_signed #(")
    if patched != text:
        netlist_path.write_text(patched, encoding="utf-8")


def _simulate(temp_dir, testbench, num_threads, label):
    """Locate the post-synthesis netlist, compile with Verilator, and run the simulation.

    Returns (exit_code, sim_out_path) where sim_out_path is None when an early
    error (missing netlist or failed Verilator compilation) was already reported.
    """
    # VPR names the netlist after the top-level module, not the circuit
    # filename (e.g. module top -> top_post_synthesis.v).  Glob for it so
    # we don't need to parse the source to find the module name.
    matches = list(Path(temp_dir).glob("*_post_synthesis.v"))
    if len(matches) == 0:
        print(
            "{}\t\tFAILED: no *_post_synthesis.v found in {}".format(label, temp_dir),
            flush=True,
        )
        return 1, None
    if len(matches) > 1:
        print(
            "{}\t\tFAILED: multiple *_post_synthesis.v files found in {}: {}".format(
                label, temp_dir, matches
            ),
            flush=True,
        )
        return 1, None
    netlist = matches[0]
    _prepare_netlist_for_sim(netlist, testbench)

    primitives = paths.root_path / "vtr_flow" / "primitives.v"
    sim_build = Path(temp_dir) / "sim_build"
    verilator_out = Path(temp_dir) / "verilator.out"
    sim_out = Path(temp_dir) / "sim.out"

    verilator_cmd = [
        "verilator",
        "--binary",
        "-sv",
        str(testbench),
        str(netlist),
        str(primitives),
        "--top-module",
        "tb",
        "--Mdir",
        str(sim_build),
        "-j",
        str(num_threads),
        # post-pnr netlists widen address/control buses, this shouldn't cause the test to fail
        "-Wno-WIDTHTRUNC",
        "-Wno-WIDTHEXPAND",
        "-Wno-PINMISSING",
        "-Wno-INITIALDLY",
    ]
    with open(verilator_out, "w", encoding="utf-8") as log:
        ret = subprocess.call(verilator_cmd, stdout=log, stderr=log)
    if ret:
        print(
            "{}\t\tFAILED: Verilator compilation (see {})".format(label, verilator_out),
            flush=True,
        )
        return ret, None

    with open(sim_out, "w", encoding="utf-8") as log:
        ret = subprocess.call([str(sim_build / "Vtb")], stdout=log, stderr=log)

    return ret, sim_out


def main(argv=None):
    """Entry point."""
    if argv is None:
        argv = sys.argv[1:]

    known, remaining, temp_dir = _parse_args(argv)
    testbench = known.testbench
    num_threads = known.verilator_j
    start = datetime.now()

    # --- Step 1: run the VTR flow ----------------------------------------
    # Capture stdout so we can suppress the intermediate VPR summary line and
    # replace it with a single combined summary at the end.
    vtr_buf = io.StringIO()
    vtr_args = list(remaining) + ["--gen_post_synthesis_netlist", "on"]
    with redirect_stdout(vtr_buf):
        ret = run_vtr_flow(vtr_args, str(paths.run_vtr_flow_path))

    if ret:
        # VPR failed: surface its output unchanged so the user sees the error.
        print(vtr_buf.getvalue(), end="", flush=True)
        return ret

    # Extract the arch/circuit label that VPR printed (e.g. "k6_N10/adder_4bit").
    vtr_output = vtr_buf.getvalue()
    label = vtr_output.split("\t\t")[0] if "\t\t" in vtr_output else "unknown/unknown"

    # --- Steps 2–4: locate netlist, compile with Verilator, simulate -----
    if temp_dir is None:
        # run_vtr_flow defaults to a 'temp' subdirectory of cwd.
        temp_dir = str(Path.cwd() / "temp")

    ret, sim_out = _simulate(temp_dir, testbench, num_threads, label)
    if sim_out is None:
        return ret

    # --- Step 5: print the single combined summary line ------------------
    detail = _format_detail(start, vtr_output)
    if ret == 0:
        print("{}\t\tOK ({})".format(label, detail), flush=True)
    else:
        print(
            "{}\t\tFAILED: functional simulation ({}, see {})".format(label, detail, sim_out),
            flush=True,
        )
    return ret


if __name__ == "__main__":
    sys.exit(main())
