#!/usr/bin/env python3
"""mosaic verilator random-check: rtl vs post-synth BLIF (after synth+abc).

takes a VTR run directory (or explicit paths), converts the post-synth BLIF
(pre-VPR / after ABC) to verilog via yosys, builds a 2-dut testbench, verilates,
and drives random vectors. exit 0 only if rtl and post-synth outputs match.

optional directed coverage:
  --check-mem-init   fail if rtl has memory init that hard rams cannot carry
  --directed-ram     prepend same-addr read/write (and write/write if feasible)
  --ram-zero-init    force sim ram contents to 0 (hides init bugs. default is x)

usage:
  python3 mosaic/verilator_check/run_random_check.py \\
    --run-dir compare_output_k6_qor/runs/adder_4bit_mosaic \\
    --vectors 200000 --seed 1

  python3 mosaic/verilator_check/run_random_check.py \\
    --rtl path/to/top.v --post-synth-blif design.pre-vpr.blif
"""

from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import List, Optional, Tuple

_HERE = Path(__file__).resolve().parent
if str(_HERE) not in sys.path:
    sys.path.insert(0, str(_HERE))

# pylint: disable=wrong-import-position
from blif_to_verilog import blif_to_verilog, resolve_yosys  # noqa: E402
from port_parse import (  # noqa: E402
    find_top_module_name,
    is_clock_port,
    is_reset_port,
    parse_verilog_top_ports,
)
from tb_generator import detect_directed_ram_plan, generate_dual_testbench  # noqa: E402
# pylint: enable=wrong-import-position

REPO_ROOT = _HERE.parents[1]
MODELS_V = _HERE / "models" / "sim_hardblocks.v"

_READMEM_RE = re.compile(r"\$readmem[hb]\b", re.I)
_MEM_ARRAY_RE = re.compile(
    r"\b(?:reg|logic)\s*(?:\[[^\]]+\])?\s*(\w+)\s*(?:\[[^\]]+\])?\s*;",
    re.I,
)
_INITIAL_ASSIGN_RE = re.compile(
    r"initial\s+begin.*?end",
    re.I | re.S,
)
_HARD_RAM_RE = re.compile(r"\b(?:single_port_ram|dual_port_ram)\b")


def resolve_verilator() -> Optional[str]:
    """locate verilator on PATH or in a common user-local install."""
    found = shutil.which("verilator")
    if found:
        return found
    local = Path.home() / ".local" / "bin" / "verilator"
    if local.is_file():
        return str(local)
    return None


def find_rtl_mem_init_evidence(rtl_path: Path) -> List[str]:
    """return human-readable evidence that rtl initializes memory."""
    text = rtl_path.read_text(encoding="utf-8", errors="replace")
    findings: List[str] = []
    if _READMEM_RE.search(text):
        findings.append("$readmemh/$readmemb present")
    mem_names = {m.group(1) for m in _MEM_ARRAY_RE.finditer(text)}
    for block in _INITIAL_ASSIGN_RE.finditer(text):
        body = block.group(0)
        for mem_name in mem_names:
            if re.search(rf"\b{re.escape(mem_name)}\s*\[", body):
                findings.append(f"initial assign into array '{mem_name}'")
                break
        if re.search(r"\w+\s*\[[^\]]+\]\s*=", body) and "for" in body.lower():
            if "mem init loop-style assign in initial" not in findings:
                findings.append("mem init loop-style assign in initial")
    seen = set()
    ordered: List[str] = []
    for item in findings:
        if item not in seen:
            seen.add(item)
            ordered.append(item)
    return ordered


def synth_uses_hard_ram(synth_blif: Path, synth_verilog: Optional[Path] = None) -> bool:
    """return true when post-synth netlist still has hard ram blackboxes."""
    for path in (synth_blif, synth_verilog):
        if path is None or not path.is_file():
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        if _HARD_RAM_RE.search(text):
            return True
    return False


def check_mem_init_mismatch(
    rtl_path: Path, synth_blif: Path, synth_verilog: Optional[Path] = None
) -> int:
    """exit-style status: 0 ok, 1 init likely dropped, 2 scan error."""
    evidence = find_rtl_mem_init_evidence(rtl_path)
    if not evidence:
        print("mem-init check: no rtl memory init patterns found")
        return 0
    print("mem-init check: rtl appears to initialize memory:")
    for item in evidence:
        print(f"  - {item}")
    if synth_uses_hard_ram(synth_blif, synth_verilog):
        print(
            "mem-init check: FAIL, synth uses hard single_port_ram/dual_port_ram "
            "blackboxes with no INIT ports; rtl init cannot be preserved",
            file=sys.stderr,
        )
        return 1
    print(
        "mem-init check: WARN, rtl has init but no hard ram cells found in synth; "
        "soft-mapped init may or may not have survived (manual review)",
        file=sys.stderr,
    )
    return 0


def rename_module(
    src: Path, dest: Path, new_name: str, old_name: Optional[str] = None
) -> str:
    """rename one module in src to new_name, return the old name used."""
    text = src.read_text(encoding="utf-8", errors="replace")
    if old_name is None:
        match = re.search(r"^(\s*)module\s+(\w+)\b", text, re.M)
        if not match:
            raise ValueError(f"no module in {src}")
        old_name = match.group(2)
    elif not re.search(rf"^\s*module\s+{re.escape(old_name)}\b", text, re.M):
        raise ValueError(f"module {old_name} not found in {src}")
    text = re.sub(
        rf"^(\s*)module\s+{re.escape(old_name)}\b",
        rf"\1module {new_name}",
        text,
        count=1,
        flags=re.M,
    )
    dest.write_text(text, encoding="utf-8")
    return old_name


def discover_artifacts(run_dir: Path) -> Tuple[Path, Path]:  # pylint: disable=too-many-branches
    """return (rtl, post_synth_blif) from a VTR temp dir or compare harness run."""
    if not run_dir.is_dir():
        raise FileNotFoundError(f"run dir missing: {run_dir}")

    rtl_candidates = sorted(
        p
        for p in run_dir.glob("*.v")
        if "post_synthesis" not in p.name
        and not p.name.endswith(".sv")
        and "dut_" not in p.name
        and p.name not in ("synth_raw.v", "tb.sv")
    )
    if not rtl_candidates:
        raise FileNotFoundError(f"no rtl .v in {run_dir}")
    rtl_path = rtl_candidates[0]

    post_synth_blif = None
    search_dirs = [run_dir]
    vpr_dir = run_dir / "vpr"
    if vpr_dir.is_dir():
        search_dirs.append(vpr_dir)

    for search_dir in search_dirs:
        for pattern in ("*.pre-vpr.blif", "*.abc.blif", "*.mosaic.blif"):
            hits = sorted(
                h
                for h in search_dir.glob(pattern)
                if not h.name.startswith(("0_", "1_", "2_")) and "raw" not in h.name
            )
            if hits:
                post_synth_blif = hits[0]
                break
        if post_synth_blif is not None:
            break

    if post_synth_blif is None:
        for pattern in ("*.parmys.blif", "*.odin.blif"):
            hits = sorted(p for p in run_dir.glob(pattern) if p.parent == run_dir)
            if hits:
                post_synth_blif = hits[0]
                break

    if post_synth_blif is None:
        raise FileNotFoundError(
            f"no post-synth blif under {run_dir} "
            "(need *.pre-vpr.blif, *.abc.blif, or *.mosaic.blif)"
        )
    return rtl_path, post_synth_blif


def run_random_check(  # pylint: disable=too-many-arguments,too-many-locals
    rtl_path: Path,
    post_synth_blif: Path,
    work_dir: Path,
    *,
    vectors: int = 200_000,
    seed: int = 1,
    max_errors: int = 20,
    yosys_path: Optional[Path] = None,
    verilator_jobs: int = 4,
    check_mem_init: bool = False,
    directed_ram: bool = False,
    ram_zero_init: bool = False,
    include_files: Optional[List[Path]] = None,
) -> int:
    """run the random check given explicit paths, return process exit code."""
    for path, label in ((rtl_path, "rtl"), (post_synth_blif, "post-synth-blif")):
        if not path.is_file():
            print(f"error: missing {label}: {path}", file=sys.stderr)
            return 2

    print(f"rtl:             {rtl_path}")
    print(f"post-synth blif: {post_synth_blif}")
    print(f"work:            {work_dir}")
    print(f"vectors:         {vectors}  seed={seed}")
    if include_files:
        print(f"includes:        {', '.join(str(path) for path in include_files)}")
    if directed_ram:
        print("directed-ram: enabled")
    if ram_zero_init:
        print("ram-zero-init: enabled (sim rams start at 0)")
    else:
        print("ram init: uninitialized/x (use --ram-zero-init to force zeros)")

    resolve_yosys(yosys_path)

    _, sources, synth_dut = build_work_dir(
        work_dir,
        rtl_path,
        post_synth_blif,
        yosys_path=yosys_path,
        num_vectors=vectors,
        seed=seed,
        max_errors=max_errors,
        directed_ram=directed_ram,
        include_files=include_files,
    )

    if check_mem_init:
        mem_status = check_mem_init_mismatch(rtl_path, post_synth_blif, synth_dut)
        if mem_status != 0:
            return mem_status

    return run_verilator(
        work_dir,
        sources,
        seed=seed,
        verilator_jobs=max(1, verilator_jobs),
        ram_zero_init=ram_zero_init,
    )


def combine_rtl_with_includes(
    rtl_path: Path, include_files: Optional[List[Path]], dest: Path
) -> Path:
    """prepend run_vtr_flow -include files so rtl ifdef matches synthesis."""
    include_paths = [Path(path) for path in (include_files or []) if Path(path).is_file()]
    if not include_paths:
        return rtl_path
    chunks: List[str] = []
    for inc_path in include_paths:
        chunks.append(f"// vtr -include {inc_path.name}\n")
        body = inc_path.read_text(encoding="utf-8", errors="replace")
        chunks.append(body)
        if body and not body.endswith("\n"):
            chunks.append("\n")
    chunks.append(rtl_path.read_text(encoding="utf-8", errors="replace"))
    dest.write_text("".join(chunks), encoding="utf-8")
    return dest


_OUTPUT_REG_RE = re.compile(
    r"^(\s*output\s+)reg(\s+(?:\[[^\]]+\]\s*)?)(\w+)\s*;",
    re.M,
)
_CELL_OUT_PIN_RE = re.compile(r"\.(?:out|out1|out2|sumout)\s*\(\s*(\w+)\s*\)")


def relax_output_regs_driven_by_cells(verilog_text: str) -> str:
    """drop 'reg' on output ports that a child cell drives (koios hard_mem wrappers)."""
    driven_nets = set(_CELL_OUT_PIN_RE.findall(verilog_text))
    if not driven_nets:
        return verilog_text

    def drop_reg_if_cell_driven(match: re.Match) -> str:
        name = match.group(3)
        if name in driven_nets:
            return f"{match.group(1)}{match.group(2)}{name};"
        return match.group(0)

    return _OUTPUT_REG_RE.sub(drop_reg_if_cell_driven, verilog_text)


def build_work_dir(  # pylint: disable=too-many-arguments,too-many-locals
    work_dir: Path,
    rtl_path: Path,
    post_synth_blif: Path,
    *,
    yosys_path: Optional[Path],
    num_vectors: int,
    seed: int,
    max_errors: int,
    directed_ram: bool = False,
    include_files: Optional[List[Path]] = None,
) -> Tuple[Path, List[str], Path]:
    """prepare renamed duts and tb, return (tb_path, source_list, synth_dut_path)."""
    if work_dir.exists():
        shutil.rmtree(work_dir)
    work_dir.mkdir(parents=True)

    synth_v_raw = work_dir / "synth_raw.v"
    blif_to_verilog(
        post_synth_blif,
        synth_v_raw,
        yosys_path=yosys_path,
        log_path=work_dir / "yosys_synth.log",
    )

    synth_top = find_top_module_name(synth_v_raw)
    ports = parse_verilog_top_ports(synth_v_raw, top_name=synth_top)
    rtl_top = find_top_module_name(
        rtl_path,
        preferred_name=synth_top,
        preferred_ports=[p.name for p in ports],
    )
    clocks = [p.name for p in ports if p.direction == "input" and is_clock_port(p.name)]
    resets = [p.name for p in ports if p.direction == "input" and is_reset_port(p.name)]
    print(f"tops: synth={synth_top} rtl={rtl_top}")
    print(f"clocks: {clocks or ['(none)']}")
    print(f"resets: {resets or ['(none)']}")

    if directed_ram:
        ram_plan = detect_directed_ram_plan(ports)
        if ram_plan is None:
            print("directed-ram: no matching ram port shape on this top")
        else:
            print(f"directed-ram: {ram_plan.note}")

    rtl_dut = work_dir / "dut_rtl.v"
    synth_dut = work_dir / "dut_synth.v"
    rtl_source = combine_rtl_with_includes(
        rtl_path, include_files, work_dir / "rtl_with_includes.v"
    )
    rename_module(rtl_source, rtl_dut, "dut_rtl", old_name=rtl_top)
    rtl_dut.write_text(
        relax_output_regs_driven_by_cells(rtl_dut.read_text(encoding="utf-8", errors="replace")),
        encoding="utf-8",
    )
    rename_module(synth_v_raw, synth_dut, "dut_synth", old_name=synth_top)

    tb_text = generate_dual_testbench(
        ports,
        rtl_module="dut_rtl",
        synth_module="dut_synth",
        num_vectors=num_vectors,
        seed=seed,
        max_errors=max_errors,
        directed_ram=directed_ram,
    )
    tb_path = work_dir / "tb.sv"
    tb_path.write_text(tb_text, encoding="utf-8")

    sources = [str(tb_path), str(rtl_dut), str(synth_dut), str(MODELS_V)]
    return tb_path, sources, synth_dut


def count_rtl_warnings(verilator_out: Path) -> int:
    """count %Warning lines that cite dut_rtl.v (benchmark rtl, not synth)."""
    if not verilator_out.is_file():
        return 0
    text = verilator_out.read_text(encoding="utf-8", errors="replace")
    count = 0
    for line in text.splitlines():
        if "%Warning" not in line:
            continue
        if "dut_rtl.v" in line or "/dut_rtl.v:" in line:
            count += 1
    return count


def run_verilator(  # pylint: disable=too-many-locals
    work_dir: Path,
    sources: List[str],
    *,
    seed: int,
    verilator_jobs: int,
    ram_zero_init: bool = False,
) -> int:
    """compile and run the 2-dut testbench under verilator."""
    verilator = resolve_verilator()
    if verilator is None:
        print("error: verilator not on PATH", file=sys.stderr)
        return 2

    sim_build = work_dir / "sim_build"
    verilator_out = work_dir / "verilator.out"
    sim_out = work_dir / "sim.out"

    cmd = [
        verilator,
        "--binary",
        "-sv",
        *sources,
        "--top-module",
        "tb",
        "--Mdir",
        str(sim_build),
        "-j",
        str(verilator_jobs),
        "-Wno-WIDTHTRUNC",
        "-Wno-WIDTHEXPAND",
        "-Wno-PINMISSING",
        "-Wno-INITIALDLY",
        "-Wno-TIMESCALEMOD",
        "-Wno-DECLFILENAME",
        "-Wno-UNOPTFLAT",
        "-Wno-CASEOVERLAP",
        "-Wno-CMPCONST",
        "-Wno-UNSIGNED",
        "-Wno-BLKANDNBLK",
        "-Wno-UNOPTTHREADS",
        "-Wno-fatal",
        "--x-initial",
        "0",
    ]
    if ram_zero_init:
        cmd.append("-DMOSAIC_SIM_RAM_ZERO_INIT")
    with open(verilator_out, "w", encoding="utf-8") as log:
        ret = subprocess.call(cmd, stdout=log, stderr=log)
    rtl_warns = count_rtl_warnings(verilator_out)
    print(f"rtl_warns={rtl_warns}")
    if ret != 0:
        print(f"ERROR: verilator compile (see {verilator_out})", file=sys.stderr)
        try:
            tail = verilator_out.read_text(encoding="utf-8", errors="replace").splitlines()[
                -40:
            ]
            print("\n".join(tail), file=sys.stderr)
        except OSError:
            pass
        return ret

    sim_bin = sim_build / "Vtb"
    if not sim_bin.is_file():
        alts = list(sim_build.glob("Vtb*"))
        alts = [p for p in alts if p.is_file() and p.stat().st_mode & 0o111]
        if not alts:
            print(f"ERROR: sim binary missing under {sim_build}", file=sys.stderr)
            return 1
        sim_bin = alts[0]

    with open(sim_out, "w", encoding="utf-8") as log:
        ret = subprocess.call(
            [str(sim_bin), f"+verilator+seed+{seed}", "+verilator+rand+reset+0"],
            stdout=log,
            stderr=log,
        )
    sim_text = sim_out.read_text(encoding="utf-8", errors="replace") if sim_out.is_file() else ""
    print(sim_text.strip())
    if ret != 0:
        if "mismatches in" not in sim_text:
            print(f"ERROR: simulation (see {sim_out})", file=sys.stderr)
    return ret


def main(argv=None) -> int:
    """CLI entry point for the rtl vs post-synth random-check."""
    parser = argparse.ArgumentParser(
        description="rtl vs post-synth verilator random-check"
    )
    parser.add_argument("--run-dir", type=Path, default=None, help="vtr/compare run directory")
    parser.add_argument("--rtl", type=Path, default=None)
    parser.add_argument(
        "--include",
        type=Path,
        action="append",
        default=None,
        help="extra rtl include files (defines / modules), same as run_vtr_flow -include",
    )
    parser.add_argument(
        "--post-synth-blif",
        type=Path,
        default=None,
        help="blif after synth+abc (*.pre-vpr.blif / *.mosaic.blif)",
    )
    parser.add_argument(
        "--work-dir",
        type=Path,
        default=None,
        help="scratch dir (default: <run-dir>/verilator_check)",
    )
    parser.add_argument("--vectors", type=int, default=200_000)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--max-errors", type=int, default=20)
    parser.add_argument("--yosys", type=Path, default=None)
    parser.add_argument("--verilator-j", type=int, default=4)
    parser.add_argument(
        "--check-mem-init",
        action="store_true",
        help="fail if rtl has memory init that hard ram blackboxes cannot carry",
    )
    parser.add_argument(
        "--directed-ram",
        action="store_true",
        help="prepend directed same-addr read/write (and write/write if dual-we ports)",
    )
    parser.add_argument(
        "--ram-zero-init",
        action="store_true",
        help="force sim_hardblocks ram contents to 0 (default leaves mem uninitialized/x)",
    )
    args = parser.parse_args(argv)

    try:
        if args.run_dir:
            rtl_path, post_synth_blif = discover_artifacts(args.run_dir)
            work_dir = args.work_dir or (args.run_dir / "verilator_check")
        else:
            post_synth_blif = args.post_synth_blif
            if not (args.rtl and post_synth_blif):
                print(
                    "error: pass --run-dir or both --rtl and --post-synth-blif",
                    file=sys.stderr,
                )
                return 2
            rtl_path = args.rtl
            work_dir = args.work_dir or Path("verilator_check_work")

        return run_random_check(
            rtl_path,
            post_synth_blif,
            work_dir,
            vectors=args.vectors,
            seed=args.seed,
            max_errors=args.max_errors,
            yosys_path=args.yosys,
            verilator_jobs=max(1, args.verilator_j),
            check_mem_init=args.check_mem_init,
            directed_ram=args.directed_ram,
            ram_zero_init=args.ram_zero_init,
            include_files=args.include,
        )
    except (FileNotFoundError, ValueError, RuntimeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
