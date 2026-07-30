#!/usr/bin/env python3
"""
frankenstein verilator random-check: rtl vs post-synth blif vs post-vtr-abc blif.

takes a harness circuit (or explicit paths), converts the two blifs to verilog
via yosys, builds a 3-dut testbench, verilates, and drives hundreds of thousands
of identical random vectors. exit 0 only if all three match.

optional directed coverage:
  --check-mem-init   fail if rtl has memory init that hard rams cannot carry
  --directed-ram     prepend same-addr read/write (and write/write if feasible)
  --ram-zero-init    force sim ram contents to 0 (hides init bugs; default is x)

usage:
  python3 frankenstein/verilator_check/run_random_check.py \\
    --run-dir compare_output_k6_qor/runs/adder_4bit_frankenstein \\
    --vectors 200000 --seed 1

  python3 frankenstein/verilator_check/run_random_check.py \\
    --rtl path/to/top.v --synth-blif a.vtr.blif --abc-blif a.abc.blif
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

from blif_to_verilog import blifToVerilog, resolveYosys  # noqa: E402
from port_parse import (  # noqa: E402
    PortDecl,
    findTopModuleName,
    isClockPort,
    isResetPort,
    parseVerilogTopPorts,
)
from tb_generator import detectDirectedRamPlan, generateTripleTestbench  # noqa: E402

repoRoot = _HERE.parents[1]
modelsV = _HERE / "models" / "sim_hardblocks.v"

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


def resolveVerilator() -> Optional[str]:
    found = shutil.which("verilator")
    if found:
        return found
    local = Path.home() / ".local" / "bin" / "verilator"
    if local.is_file():
        return str(local)
    return None


def findRtlMemInitEvidence(rtlPath: Path) -> List[str]:
    """return human-readable evidence that rtl initializes memory."""
    text = rtlPath.read_text(encoding="utf-8", errors="replace")
    findings: List[str] = []
    if _READMEM_RE.search(text):
        findings.append("$readmemh/$readmemb present")
    # look for initial blocks that assign into array-like identifiers
    memNames = {m.group(1) for m in _MEM_ARRAY_RE.finditer(text)}
    for block in _INITIAL_ASSIGN_RE.finditer(text):
        body = block.group(0)
        for memName in memNames:
            if re.search(rf"\b{re.escape(memName)}\s*\[", body):
                findings.append(f"initial assign into array '{memName}'")
                break
        if re.search(r"\w+\s*\[[^\]]+\]\s*=", body) and "for" in body.lower():
            # loop init of an array without a clean typed decl match
            if "mem init loop-style assign in initial" not in findings:
                findings.append("mem init loop-style assign in initial")
    # dedupe while preserving order
    seen = set()
    ordered: List[str] = []
    for item in findings:
        if item not in seen:
            seen.add(item)
            ordered.append(item)
    return ordered


def synthUsesHardRam(synthBlif: Path, synthVerilog: Optional[Path] = None) -> bool:
    """true when post-synth netlist still has hard ram blackboxes."""
    for path in (synthBlif, synthVerilog):
        if path is None or not path.is_file():
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        if _HARD_RAM_RE.search(text):
            return True
    return False


def checkMemInitMismatch(rtlPath: Path, synthBlif: Path, synthVerilog: Optional[Path] = None) -> int:
    """exit-style status: 0 ok, 1 init likely dropped, 2 scan error."""
    evidence = findRtlMemInitEvidence(rtlPath)
    if not evidence:
        print("mem-init check: no rtl memory init patterns found")
        return 0
    print("mem-init check: rtl appears to initialize memory:")
    for item in evidence:
        print(f"  - {item}")
    if synthUsesHardRam(synthBlif, synthVerilog):
        print(
            "mem-init check: FAIL — synth uses hard single_port_ram/dual_port_ram "
            "blackboxes with no INIT ports; rtl init cannot be preserved",
            file=sys.stderr,
        )
        return 1
    print(
        "mem-init check: WARN — rtl has init but no hard ram cells found in synth; "
        "soft-mapped init may or may not have survived (manual review)",
        file=sys.stderr,
    )
    return 0


def renameModule(src: Path, dest: Path, newName: str, oldName: Optional[str] = None) -> str:
    """rename one module in src to newName; return the old name used.

    if oldName is given, rename that module (needed for multi-module rtl where
    the top is not the first module). otherwise rename the first module.
    """
    text = src.read_text(encoding="utf-8", errors="replace")
    if oldName is None:
        match = re.search(r"^(\s*)module\s+(\w+)\b", text, re.M)
        if not match:
            raise ValueError(f"no module in {src}")
        oldName = match.group(2)
    elif not re.search(rf"^\s*module\s+{re.escape(oldName)}\b", text, re.M):
        raise ValueError(f"module {oldName} not found in {src}")
    text = re.sub(
        rf"^(\s*)module\s+{re.escape(oldName)}\b",
        rf"\1module {newName}",
        text,
        count=1,
        flags=re.M,
    )
    dest.write_text(text, encoding="utf-8")
    return oldName


def discoverArtifacts(runDir: Path) -> Tuple[Path, Path, Path]:
    """return (rtl, synthBlif, abcBlif) from a compare harness run directory."""
    if not runDir.is_dir():
        raise FileNotFoundError(f"run dir missing: {runDir}")

    # rtl: copied .v in run dir (not .blif)
    rtlCandidates = sorted(
        p for p in runDir.glob("*.v")
        if "post_synthesis" not in p.name and not p.name.endswith(".sv")
    )
    if not rtlCandidates:
        raise FileNotFoundError(f"no rtl .v in {runDir}")
    rtlPath = rtlCandidates[0]

    # post-synth blif after fix: prefer *.vtr.blif then *.wb.blif (run dir root only)
    synthBlif = None
    for pattern in ("*.vtr.blif", "*.wb.blif", "*.parmys.blif"):
        hits = sorted(p for p in runDir.glob(pattern) if p.parent == runDir)
        if hits:
            synthBlif = hits[0]
            break
    if synthBlif is None:
        hits = sorted(p for p in runDir.glob("*.blif") if p.parent == runDir)
        if hits:
            synthBlif = hits[0]
    if synthBlif is None:
        raise FileNotFoundError(f"no post-synth blif in {runDir}")

    vprDir = runDir / "vpr"
    abcBlif = None
    if vprDir.is_dir():
        for pattern in (
            "*.pre-vpr.blif",
            "*.abc.blif",
            "*_arm_core.vtr.abc.blif",
        ):
            hits = sorted(vprDir.glob(pattern))
            # prefer final restored abc over raw / numbered intermediates
            preferred = [h for h in hits if not h.name.startswith(("0_", "1_", "2_"))]
            preferred = [h for h in preferred if "raw" not in h.name]
            if preferred:
                abcBlif = preferred[0]
                break
            if hits:
                # last resort: highest-numbered non-raw
                nonRaw = [h for h in hits if "raw" not in h.name]
                if nonRaw:
                    abcBlif = nonRaw[-1]
                    break
        if abcBlif is None:
            # any *.abc.blif
            hits = sorted(vprDir.glob("*.abc.blif"))
            nonRaw = [h for h in hits if "raw" not in h.name]
            if nonRaw:
                abcBlif = nonRaw[-1]
    if abcBlif is None:
        raise FileNotFoundError(
            f"no post-vtr-abc blif under {vprDir} "
            "(need --vtr-abc harness run that completed abc)"
        )
    return rtlPath, synthBlif, abcBlif


def buildWorkDir(
    workDir: Path,
    rtlPath: Path,
    synthBlif: Path,
    abcBlif: Path,
    *,
    yosysPath: Optional[Path],
    numVectors: int,
    seed: int,
    maxErrors: int,
    directedRam: bool = False,
) -> Tuple[Path, List[str], Path]:
    """prepare renamed duts + tb; return (tbPath, verilatorSourceList, synthDutPath)."""
    if workDir.exists():
        shutil.rmtree(workDir)
    workDir.mkdir(parents=True)

    # convert blifs
    synthVRaw = workDir / "synth_raw.v"
    abcVRaw = workDir / "abc_raw.v"
    blifToVerilog(synthBlif, synthVRaw, yosysPath=yosysPath, logPath=workDir / "yosys_synth.log")
    blifToVerilog(abcBlif, abcVRaw, yosysPath=yosysPath, logPath=workDir / "yosys_abc.log")

    # ports come from the flattened synth netlist (always the real design top).
    # rtl may be multi-module; prefer the synth top name if present in rtl
    # (e.g. RLE_BlobMerging in blob_merge.v), else file stem, else best
    # port-name overlap. never fall back to a helper like divider first.
    synthTop = findTopModuleName(synthVRaw)
    ports = parseVerilogTopPorts(synthVRaw, topName=synthTop)
    rtlTop = findTopModuleName(
        rtlPath,
        preferredName=synthTop,
        preferredPorts=[p.name for p in ports],
    )
    clocks = [p.name for p in ports if p.direction == "input" and isClockPort(p.name)]
    resets = [p.name for p in ports if p.direction == "input" and isResetPort(p.name)]
    print(f"tops: synth={synthTop} rtl={rtlTop}")
    print(f"clocks: {clocks or ['(none)']}")
    print(f"resets: {resets or ['(none)']}")

    if directedRam:
        ramPlan = detectDirectedRamPlan(ports)
        if ramPlan is None:
            print("directed-ram: no matching ram port shape on this top")
        else:
            print(f"directed-ram: {ramPlan.note}")

    rtlDut = workDir / "dut_rtl.v"
    synthDut = workDir / "dut_synth.v"
    abcDut = workDir / "dut_abc.v"
    renameModule(rtlPath, rtlDut, "dut_rtl", oldName=rtlTop)
    renameModule(synthVRaw, synthDut, "dut_synth", oldName=synthTop)
    renameModule(abcVRaw, abcDut, "dut_abc")

    tbText = generateTripleTestbench(
        ports,
        rtlModule="dut_rtl",
        synthModule="dut_synth",
        abcModule="dut_abc",
        numVectors=numVectors,
        seed=seed,
        maxErrors=maxErrors,
        directedRam=directedRam,
    )
    tbPath = workDir / "tb.sv"
    tbPath.write_text(tbText, encoding="utf-8")

    # use sim_hardblocks only — do not also link vtr_flow/primitives.v
    # (duplicate adder/dff module definitions).
    sources = [str(tbPath), str(rtlDut), str(synthDut), str(abcDut), str(modelsV)]
    return tbPath, sources, synthDut


def countRtlWarnings(verilatorOut: Path) -> int:
    """count %Warning lines that cite dut_rtl.v (benchmark rtl, not synth/abc)."""
    if not verilatorOut.is_file():
        return 0
    text = verilatorOut.read_text(encoding="utf-8", errors="replace")
    count = 0
    for line in text.splitlines():
        if "%Warning" not in line:
            continue
        if "dut_rtl.v" in line or "/dut_rtl.v:" in line:
            count += 1
    return count


def runVerilator(
    workDir: Path,
    sources: List[str],
    *,
    seed: int,
    verilatorJobs: int,
    ramZeroInit: bool = False,
) -> int:
    verilator = resolveVerilator()
    if verilator is None:
        print("error: verilator not on PATH", file=sys.stderr)
        return 2

    simBuild = workDir / "sim_build"
    verilatorOut = workDir / "verilator.out"
    simOut = workDir / "sim.out"

    cmd = [
        verilator,
        "--binary",
        "-sv",
        *sources,
        "--top-module",
        "tb",
        "--Mdir",
        str(simBuild),
        "-j",
        str(verilatorJobs),
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
        # keep rtl warnings visible for the rtl_warns status column, but do not
        # abort compile on them (benchmark rtl often trips IMPLICIT/LATCH/etc).
        "-Wno-fatal",
    ]
    if ramZeroInit:
        cmd.append("-DFRANKENSTEIN_SIM_RAM_ZERO_INIT")
    with open(verilatorOut, "w", encoding="utf-8") as log:
        ret = subprocess.call(cmd, stdout=log, stderr=log)
    rtlWarns = countRtlWarnings(verilatorOut)
    print(f"rtl_warns={rtlWarns}")
    if ret != 0:
        print(f"FAIL: verilator compile (see {verilatorOut})", file=sys.stderr)
        # print last lines for convenience
        try:
            tail = verilatorOut.read_text(encoding="utf-8", errors="replace").splitlines()[-40:]
            print("\n".join(tail), file=sys.stderr)
        except OSError:
            pass
        return ret

    simBin = simBuild / "Vtb"
    if not simBin.is_file():
        # some verilator versions nest differently
        alts = list(simBuild.glob("Vtb*"))
        alts = [p for p in alts if p.is_file() and p.stat().st_mode & 0o111]
        if not alts:
            print(f"FAIL: sim binary missing under {simBuild}", file=sys.stderr)
            return 1
        simBin = alts[0]

    with open(simOut, "w", encoding="utf-8") as log:
        ret = subprocess.call(
            [str(simBin), f"+verilator+seed+{seed}"],
            stdout=log,
            stderr=log,
        )
    simText = simOut.read_text(encoding="utf-8", errors="replace") if simOut.is_file() else ""
    print(simText.strip())
    if ret != 0:
        print(f"FAIL: simulation (see {simOut})", file=sys.stderr)
    return ret


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run-dir", type=Path, default=None, help="compare harness run directory")
    parser.add_argument("--rtl", type=Path, default=None)
    parser.add_argument("--synth-blif", type=Path, default=None)
    parser.add_argument("--abc-blif", type=Path, default=None)
    parser.add_argument("--work-dir", type=Path, default=None, help="scratch dir (default: <run-dir>/verilator_check)")
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
            rtlPath, synthBlif, abcBlif = discoverArtifacts(args.run_dir)
            workDir = args.work_dir or (args.run_dir / "verilator_check")
        else:
            if not (args.rtl and args.synth_blif and args.abc_blif):
                print("error: pass --run-dir or all of --rtl/--synth-blif/--abc-blif", file=sys.stderr)
                return 2
            rtlPath, synthBlif, abcBlif = args.rtl, args.synth_blif, args.abc_blif
            workDir = args.work_dir or Path("verilator_check_work")
        for path, label in (
            (rtlPath, "rtl"),
            (synthBlif, "synth-blif"),
            (abcBlif, "abc-blif"),
        ):
            if not path.is_file():
                print(f"error: missing {label}: {path}", file=sys.stderr)
                return 2

        print(f"rtl:        {rtlPath}")
        print(f"synth blif: {synthBlif}")
        print(f"abc blif:   {abcBlif}")
        print(f"work:       {workDir}")
        print(f"vectors:    {args.vectors}  seed={args.seed}")
        if args.directed_ram:
            print("directed-ram: enabled")
        if args.ram_zero_init:
            print("ram-zero-init: enabled (sim rams start at 0)")
        else:
            print("ram init: uninitialized/x (use --ram-zero-init to force zeros)")

        # smoke that yosys exists early
        resolveYosys(args.yosys)

        _, sources, synthDut = buildWorkDir(
            workDir,
            rtlPath,
            synthBlif,
            abcBlif,
            yosysPath=args.yosys,
            numVectors=args.vectors,
            seed=args.seed,
            maxErrors=args.max_errors,
            directedRam=args.directed_ram,
        )

        if args.check_mem_init:
            memStatus = checkMemInitMismatch(rtlPath, synthBlif, synthDut)
            if memStatus != 0:
                return memStatus

        return runVerilator(
            workDir,
            sources,
            seed=args.seed,
            verilatorJobs=max(1, args.verilator_j),
            ramZeroInit=args.ram_zero_init,
        )
    except (FileNotFoundError, ValueError, RuntimeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
