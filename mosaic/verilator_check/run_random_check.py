#!/usr/bin/env python3
# mosaic verilator random-check. rtl vs post-synth blif (after synth+abc).
#
# takes a vtr run directory (or explicit paths), converts the post-synth blif
# (pre-vpr / after abc) to verilog via yosys, builds a 2-dut testbench, verilates,
# and drives random vectors. exit 0 only if rtl and post-synth outputs match.
#
# optional directed coverage:
#   --check-mem-init   fail if rtl has memory init that hard rams cannot carry
#   --directed-ram     prepend same-addr read/write (and write/write if feasible)
#   --ram-zero-init    force sim ram contents to 0 (hides init bugs. default is x)
#
# usage:
#   python3 mosaic/verilator_check/run_random_check.py \
#     --run-dir compare_output_k6_qor/runs/adder_4bit_mosaic \
#     --vectors 200000 --seed 1
#
#   python3 mosaic/verilator_check/run_random_check.py \
#     --rtl path/to/top.v --post-synth-blif design.pre-vpr.blif

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
    findTopModuleName,
    isClockPort,
    isResetPort,
    parseVerilogTopPorts,
)
from tb_generator import detectDirectedRamPlan, generateDualTestbench  # noqa: E402

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
    # PATH first, then a common user-local install
    found = shutil.which("verilator")
    if found:
        return found
    local = Path.home() / ".local" / "bin" / "verilator"
    if local.is_file():
        return str(local)
    return None


# USE: return human-readable evidence that rtl initializes memory.
def findRtlMemInitEvidence(rtlPath: Path) -> List[str]:
    text = rtlPath.read_text(encoding="utf-8", errors="replace")
    findings: List[str] = []
    if _READMEM_RE.search(text):
        findings.append("$readmemh/$readmemb present")
    memNames = {m.group(1) for m in _MEM_ARRAY_RE.finditer(text)}
    for block in _INITIAL_ASSIGN_RE.finditer(text):
        body = block.group(0)
        for memName in memNames:
            if re.search(rf"\b{re.escape(memName)}\s*\[", body):
                findings.append(f"initial assign into array '{memName}'")
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


# USE: true when post-synth netlist still has hard ram blackboxes.
def synthUsesHardRam(synthBlif: Path, synthVerilog: Optional[Path] = None) -> bool:
    for path in (synthBlif, synthVerilog):
        if path is None or not path.is_file():
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        if _HARD_RAM_RE.search(text):
            return True
    return False


# USE: exit-style status. 0 ok, 1 init likely dropped, 2 scan error.
def checkMemInitMismatch(
    rtlPath: Path, synthBlif: Path, synthVerilog: Optional[Path] = None
) -> int:
    evidence = findRtlMemInitEvidence(rtlPath)
    if not evidence:
        print("mem-init check: no rtl memory init patterns found")
        return 0
    print("mem-init check: rtl appears to initialize memory:")
    for item in evidence:
        print(f"  - {item}")
    if synthUsesHardRam(synthBlif, synthVerilog):
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


# USE: rename one module in src to newName. return the old name used.
# oldName selects which module when the file has more than one.
def renameModule(
    src: Path, dest: Path, newName: str, oldName: Optional[str] = None
) -> str:
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


# USE: return (rtl, postSynthBlif) from a vtr temp dir or compare harness run.
# post-synth means after frontend + abc (*.pre-vpr.blif / mosaic.blif).
def discoverArtifacts(runDir: Path) -> Tuple[Path, Path]:
    if not runDir.is_dir():
        raise FileNotFoundError(f"run dir missing: {runDir}")

    rtlCandidates = sorted(
        p
        for p in runDir.glob("*.v")
        if "post_synthesis" not in p.name
        and not p.name.endswith(".sv")
        and "dut_" not in p.name
        and p.name not in ("synth_raw.v", "tb.sv")
    )
    if not rtlCandidates:
        raise FileNotFoundError(f"no rtl .v in {runDir}")
    rtlPath = rtlCandidates[0]

    postSynthBlif = None
    searchDirs = [runDir]
    vprDir = runDir / "vpr"
    if vprDir.is_dir():
        searchDirs.append(vprDir)

    # post-synth netlist after frontend and abc when present
    # (*.pre-vpr.blif, *.abc.blif, or *.mosaic.blif for mosaic)
    for searchDir in searchDirs:
        for pattern in ("*.pre-vpr.blif", "*.abc.blif", "*.mosaic.blif"):
            hits = sorted(
                h
                for h in searchDir.glob(pattern)
                if not h.name.startswith(("0_", "1_", "2_")) and "raw" not in h.name
            )
            if hits:
                postSynthBlif = hits[0]
                break
        if postSynthBlif is not None:
            break

    if postSynthBlif is None:
        for pattern in ("*.parmys.blif", "*.odin.blif"):
            hits = sorted(p for p in runDir.glob(pattern) if p.parent == runDir)
            if hits:
                # frontend blif when no pre-vpr/abc artifact exists
                postSynthBlif = hits[0]
                break

    if postSynthBlif is None:
        raise FileNotFoundError(
            f"no post-synth blif under {runDir} "
            "(need *.pre-vpr.blif, *.abc.blif, or *.mosaic.blif)"
        )
    return rtlPath, postSynthBlif


# USE: run the random check given explicit paths. return process exit code.
def runRandomCheck(
    rtlPath: Path,
    postSynthBlif: Path,
    workDir: Path,
    *,
    vectors: int = 200_000,
    seed: int = 1,
    maxErrors: int = 20,
    yosysPath: Optional[Path] = None,
    verilatorJobs: int = 4,
    checkMemInit: bool = False,
    directedRam: bool = False,
    ramZeroInit: bool = False,
) -> int:
    for path, label in ((rtlPath, "rtl"), (postSynthBlif, "post-synth-blif")):
        if not path.is_file():
            print(f"error: missing {label}: {path}", file=sys.stderr)
            return 2

    print(f"rtl:             {rtlPath}")
    print(f"post-synth blif: {postSynthBlif}")
    print(f"work:            {workDir}")
    print(f"vectors:         {vectors}  seed={seed}")
    if directedRam:
        print("directed-ram: enabled")
    if ramZeroInit:
        print("ram-zero-init: enabled (sim rams start at 0)")
    else:
        print("ram init: uninitialized/x (use --ram-zero-init to force zeros)")

    resolveYosys(yosysPath)

    _, sources, synthDut = buildWorkDir(
        workDir,
        rtlPath,
        postSynthBlif,
        yosysPath=yosysPath,
        numVectors=vectors,
        seed=seed,
        maxErrors=maxErrors,
        directedRam=directedRam,
    )

    if checkMemInit:
        memStatus = checkMemInitMismatch(rtlPath, postSynthBlif, synthDut)
        if memStatus != 0:
            return memStatus

    return runVerilator(
        workDir,
        sources,
        seed=seed,
        verilatorJobs=max(1, verilatorJobs),
        ramZeroInit=ramZeroInit,
    )


# USE: prepare renamed duts and tb. return (tbPath, verilatorSourceList, synthDutPath).
def buildWorkDir(
    workDir: Path,
    rtlPath: Path,
    postSynthBlif: Path,
    *,
    yosysPath: Optional[Path],
    numVectors: int,
    seed: int,
    maxErrors: int,
    directedRam: bool = False,
) -> Tuple[Path, List[str], Path]:
    if workDir.exists():
        shutil.rmtree(workDir)
    workDir.mkdir(parents=True)

    # flatten post-synth blif to one verilog module for port discovery and the dut
    synthVRaw = workDir / "synth_raw.v"
    blifToVerilog(
        postSynthBlif,
        synthVRaw,
        yosysPath=yosysPath,
        logPath=workDir / "yosys_synth.log",
    )

    # ports come from the flattened post-synth netlist (real design top).
    # rtl may be multi-module; prefer the synth top name when present in rtl.
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
    renameModule(rtlPath, rtlDut, "dut_rtl", oldName=rtlTop)
    renameModule(synthVRaw, synthDut, "dut_synth", oldName=synthTop)

    tbText = generateDualTestbench(
        ports,
        rtlModule="dut_rtl",
        synthModule="dut_synth",
        numVectors=numVectors,
        seed=seed,
        maxErrors=maxErrors,
        directedRam=directedRam,
    )
    tbPath = workDir / "tb.sv"
    tbPath.write_text(tbText, encoding="utf-8")

    # sim_hardblocks.v supplies adder/ram/etc. do not also link vtr primitives.v
    sources = [str(tbPath), str(rtlDut), str(synthDut), str(modelsV)]
    return tbPath, sources, synthDut


# USE: count %Warning lines that cite dut_rtl.v (benchmark rtl, not synth).
def countRtlWarnings(verilatorOut: Path) -> int:
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


# USE: compile and run the 2-dut testbench under verilator.
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
        "-Wno-fatal",
        "--x-initial",
        "0",
    ]
    if ramZeroInit:
        cmd.append("-DMOSAIC_SIM_RAM_ZERO_INIT")
    with open(verilatorOut, "w", encoding="utf-8") as log:
        ret = subprocess.call(cmd, stdout=log, stderr=log)
    rtlWarns = countRtlWarnings(verilatorOut)
    print(f"rtl_warns={rtlWarns}")
    if ret != 0:
        print(f"ERROR: verilator compile (see {verilatorOut})", file=sys.stderr)
        try:
            tail = verilatorOut.read_text(encoding="utf-8", errors="replace").splitlines()[
                -40:
            ]
            print("\n".join(tail), file=sys.stderr)
        except OSError:
            pass
        return ret

    simBin = simBuild / "Vtb"
    if not simBin.is_file():
        alts = list(simBuild.glob("Vtb*"))
        alts = [p for p in alts if p.is_file() and p.stat().st_mode & 0o111]
        if not alts:
            print(f"ERROR: sim binary missing under {simBuild}", file=sys.stderr)
            return 1
        simBin = alts[0]

    with open(simOut, "w", encoding="utf-8") as log:
        ret = subprocess.call(
            [str(simBin), f"+verilator+seed+{seed}", "+verilator+rand+reset+0"],
            stdout=log,
            stderr=log,
        )
    simText = simOut.read_text(encoding="utf-8", errors="replace") if simOut.is_file() else ""
    print(simText.strip())
    if ret != 0:
        if "mismatches in" not in simText:
            print(f"ERROR: simulation (see {simOut})", file=sys.stderr)
    return ret


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(
        description="rtl vs post-synth verilator random-check"
    )
    parser.add_argument("--run-dir", type=Path, default=None, help="vtr/compare run directory")
    parser.add_argument("--rtl", type=Path, default=None)
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
            rtlPath, postSynthBlif = discoverArtifacts(args.run_dir)
            workDir = args.work_dir or (args.run_dir / "verilator_check")
        else:
            postSynthBlif = args.post_synth_blif
            if not (args.rtl and postSynthBlif):
                print(
                    "error: pass --run-dir or both --rtl and --post-synth-blif",
                    file=sys.stderr,
                )
                return 2
            rtlPath = args.rtl
            workDir = args.work_dir or Path("verilator_check_work")

        return runRandomCheck(
            rtlPath,
            postSynthBlif,
            workDir,
            vectors=args.vectors,
            seed=args.seed,
            maxErrors=args.max_errors,
            yosysPath=args.yosys,
            verilatorJobs=max(1, args.verilator_j),
            checkMemInit=args.check_mem_init,
            directedRam=args.directed_ram,
            ramZeroInit=args.ram_zero_init,
        )
    except (FileNotFoundError, ValueError, RuntimeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
