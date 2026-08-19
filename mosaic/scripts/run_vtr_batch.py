#!/usr/bin/env python3
# batch runner built around the vtr_flow/scripts/run_vtr_flow.py call.
#
# each job is python3 vtr_flow/scripts/run_vtr_flow.py <circuit.v> <arch.xml> -start <stage>.
#
# every run is pinned to 1 core (--num_workers 1 + VPR_NUM_WORKERS=1 +
# OMP_NUM_THREADS=1). --jobs N means N of those single-core runs in parallel.
#
# writes a live results csv under mosaic/scripts/compare_output_<arch_stem>/.
# watch_compare.py reads from this file. with --watch, the watcher is also ran
#
# timing
#   wall  process lifetime
#   synth until frontend blif
#   vpr   VPR reported runtime
# --no-rerun reloads cached rows (including those timestamps) from the newest
# prior compare_results_*.csv in the output dir.
#
# usage:
#     python3 mosaic/scripts/run_vtr_batch.py \
#         --arch vtr_flow/arch/COFFE_22nm/k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml \
#         --benchmark-dir vtr_flow/benchmarks/verilog/koios \
#         --designs eltwise_layer conv_layer gemm_layer lenet \
#         --include hard_block_include.v \
#         --jobs 4 --watch --verilator-check mosaic
#
# flags:
#     --arch <xml>           architecture file (required)
#     --benchmark-dir <dir>  directory holding <design>.v
#     --designs <names...>   circuit stems. default: every *.v in bench dir
#                             except *_include.v
#     --include <files...>   -include paths (relative to --benchmark-dir or abs)
#     --flows <names...>     mosaic and/or vanilla_vtr (default: both)
#                             aliases: frank, parmys, vtr
#     --jobs <n>             concurrent single-core runs (default 4).
#                             each run is pinned to 1 core via --num_workers 1
#     --outdir <dir>         output directory (default
#                             mosaic/scripts/compare_output_<arch_stem>)
#     --csv <path>           results csv (default timestamped under outdir)
#     --route-chan-width N   passed to vpr (default 300)
#     --no-clean             keep existing run dirs
#     --no-rerun             skip runs that already have a .success marker
#     --verilator-check <flows...>  after synth(+abc), rtl vs post-synth
#                             random-check on the named flows (required).
#                             same names/aliases as --flows
#     --verilator-vectors N  vector count for --verilator-check (default 50000)
#     --verilator-seed N     seed for --verilator-check (default 1)
#     --watch                spawn watch_compare.py in this terminal
#     --watch-interval <s>   watch refresh period (default 1.0)

from __future__ import annotations

import argparse
import csv
import os
import re
import shutil
import subprocess
import sys
import time
from concurrent.futures import ProcessPoolExecutor, as_completed
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Tuple

try:
    import fcntl
except ImportError:  # windows
    fcntl = None

scriptDir = Path(__file__).resolve().parent
vtrRoot = scriptDir.parents[1]
# default batch output root keeps compare_output_* under mosaic/scripts.
compareOutputRoot = scriptDir
vtrFlow = vtrRoot / "vtr_flow"
runVtrFlow = vtrFlow / "scripts" / "run_vtr_flow.py"
yosysBin = vtrRoot / "build" / "bin" / "yosys"
pluginPath = vtrRoot / "build" / "share" / "yosys" / "plugins" / "mosaic.so"

flows = {
    "vanilla_vtr": {"start": "parmys"},
    "mosaic": {"start": "mosaic"},
}
flowAliases = {
    "vtr": "vanilla_vtr",
    "vanilla": "vanilla_vtr",
    "parmys": "vanilla_vtr",
    "frank": "mosaic",
}

vprBramBlockTypes = ("memory", "bram_multimode")
# koios uses dsp_top, classic k6 uses mult_36, and some arches use mae or dsp.
vprDspBlockTypes = ("mult_36", "mae", "dsp", "dsp_top")

csvFields = (
    "design",
    "flow",
    "status",
    "success",
    "vpr_status",
    "verilator_status",
    "verilator_matched",
    "verilator_mismatched",
    "verilator_vectors",
    "verilator_port_errors",
    "start_unix",
    "synth_finish_unix",
    "vpr_finish_unix",
    "wall_time_sec",
    "synth_wall_sec",
    "abc_wall_sec",
    "vpr_wall_sec",
    "synthesis_sec",
    "synth_luts",
    "abc_luts",
    "packed_luts",
    "synth_ff",
    "num_ff",
    "num_clb",
    "num_memory",
    "num_dsp",
    "num_adder",
    "num_io_in",
    "num_io_out",
    "total_wire_length",
    "crit_path_delay_ns",
    "fmax_mhz",
    "worst_slack_ns",
    "return_code",
)

# live status is monotonic so log-tail flicker cannot move a run backwards.
statusRank = {
    "pending": 0,
    "started": 1,
    "synth": 2,
    "abc": 3,
    "packing": 4,
    "pack done": 5,
    "placing": 6,
    "place done": 7,
    "routing": 8,
    "route done": 9,
    "ok": 100,
    "cached": 100,
    "fail": 100,
}

phasePatterns = [
    (re.compile(r"(?:^|#\s*)Routing took\b", re.I), "route done"),
    (re.compile(r"(?:^|#\s*)SA Placement took\b|(?:^|#\s*)Placement took\b", re.I), "place done"),
    (re.compile(r"(?:^|#\s*)Packing took\b", re.I), "pack done"),
    (re.compile(r"(?:^|#\s*)Routing\s*$", re.I), "routing"),
    (re.compile(r"(?:^|#\s*)SA Placement\s*$|(?:^|#\s*)Placement\s*$", re.I), "placing"),
    (re.compile(r"(?:^|#\s*)Packing\s*$|Begin packing\b", re.I), "packing"),
    (re.compile(r"Executing ABC|abc -luts", re.I), "abc"),
    (
        re.compile(
            r"mosaic|vtr_arch_rules|parmys|Executing PARMYS|Yosys [0-9]",
            re.I,
        ),
        "synth",
    ),
]

statusKeys = (
    ("wall", "wall_time_sec"),
    ("synth", "synth_wall_sec"),
    ("vpr", "vpr_wall_sec"),
    ("vcheck", "verilator_status"),
    ("match", "verilator_matched"),
    ("mismatch", "verilator_mismatched"),
    ("vectors", "verilator_vectors"),
    ("perrors", "verilator_port_errors"),
    ("s_luts", "synth_luts"),
    ("a_luts", "abc_luts"),
    ("p_luts", "packed_luts"),
    ("s_ff", "synth_ff"),
    ("ff", "num_ff"),
    ("mem", "num_memory"),
    ("dsp", "num_dsp"),
    ("adder", "num_adder"),
    ("io_in", "num_io_in"),
    ("io_out", "num_io_out"),
    ("clb", "num_clb"),
    ("wl", "total_wire_length"),
    ("cpd", "crit_path_delay_ns"),
    ("fmax", "fmax_mhz"),
    ("wns", "worst_slack_ns"),
)


def resolvePath(value: str, base: Path) -> Path:
    path = Path(value).expanduser()
    return path if path.is_absolute() else (base / path)


def discoverDesigns(benchDir: Path) -> List[str]:
    designs = []
    for path in sorted(benchDir.glob("*.v")):
        if path.name.endswith("_include.v"):
            continue
        designs.append(path.stem)
    return designs


def resolveDesigns(names: Optional[Sequence[str]], benchDir: Path) -> List[str]:
    designs = list(names) if names else discoverDesigns(benchDir)
    if not designs:
        raise SystemExit(f"no designs found under {benchDir}")
    missing = [name for name in designs if not (benchDir / f"{name}.v").is_file()]
    if missing:
        available = discoverDesigns(benchDir)
        hint = ", ".join(available[:12]) if available else "(none)"
        if len(available) > 12:
            hint += ", ..."
        raise SystemExit(
            "missing verilog for design(s): "
            + ", ".join(missing)
            + f"\n  looked in: {benchDir}"
            + f"\n  available: {hint}"
        )
    return designs


def normalizeFlows(names: Optional[Sequence[str]]) -> List[str]:
    if not names:
        return list(flows)
    selected = []
    for name in names:
        resolved = flowAliases.get(name, name)
        if resolved not in flows:
            raise SystemExit(f"unknown flow: {name} (want: {', '.join(flows)})")
        if resolved not in selected:
            selected.append(resolved)
    return selected


def resolveIncludePaths(includeArgs: Optional[Sequence[str]], benchDir: Path) -> List[Path]:
    resolved: List[Path] = []
    if not includeArgs:
        return resolved
    for includeArg in includeArgs:
        includePath = Path(includeArg)
        if not includePath.is_absolute():
            includePath = benchDir / includePath
        includePath = includePath.resolve()
        if not includePath.is_file():
            raise FileNotFoundError(f"include file not found: {includePath}")
        resolved.append(includePath)
    return resolved


def checkPrerequisites(needMosaic: bool, archFile: Path, benchDir: Path) -> None:
    missing = []
    for path in (runVtrFlow, archFile, yosysBin):
        if not path.is_file():
            missing.append(str(path))
    if not benchDir.is_dir():
        missing.append(str(benchDir))
    if needMosaic and not pluginPath.is_file():
        missing.append(str(pluginPath))
    if missing:
        print("missing prerequisites:", file=sys.stderr)
        for path in missing:
            print(f"  {path}", file=sys.stderr)
        raise SystemExit(1)


def statusSortKey(status: str) -> int:
    if status in statusRank:
        return statusRank[status]
    return statusRank.get("started", 1)


# USE: keep the furthest progress status (never regress from place done to synth).
def preferStatus(current: str, proposed: str) -> str:
    if not proposed:
        return current or "pending"
    if not current:
        return proposed
    if statusSortKey(proposed) >= statusSortKey(current):
        return proposed
    return current


# USE: infer current phase from the newest log tails in a run dir.
def detectPhase(runDir: Path) -> str:
    for name in (
        "vpr.out",
        "vpr_stdout.log",
        "mosaic.out",
        "parmys.out",
        "output.txt",
    ):
        path = runDir / name
        if not path.is_file() or path.stat().st_size == 0:
            continue
        try:
            with open(path, "rb") as handle:
                handle.seek(0, os.SEEK_END)
                size = handle.tell()
                handle.seek(max(0, size - 8192), os.SEEK_SET)
                text = handle.read().decode("utf-8", errors="replace")
        except OSError:
            continue
        for pattern, label in phasePatterns:
            if pattern.search(text):
                return label
    return "started"


def withCsvLock(csvPath: Path, callback):
    csvPath.parent.mkdir(parents=True, exist_ok=True)
    lockPath = csvPath.with_suffix(csvPath.suffix + ".lock")
    with open(lockPath, "a+", encoding="utf-8") as lockFile:
        if fcntl is not None:
            fcntl.flock(lockFile.fileno(), fcntl.LOCK_EX)
        try:
            return callback()
        finally:
            if fcntl is not None:
                fcntl.flock(lockFile.fileno(), fcntl.LOCK_UN)


def loadCsvRows(csvPath: Path) -> List[Dict]:
    if not csvPath.is_file():
        return []
    try:
        with open(csvPath, newline="", encoding="utf-8") as handle:
            return [dict(row) for row in csv.DictReader(handle)]
    except OSError:
        return []


def writeCsv(
    csvPath: Path,
    rows: List[Dict],
    order: Optional[Dict[Tuple[str, str], int]] = None,
) -> None:
    csvPath.parent.mkdir(parents=True, exist_ok=True)

    def sortKey(row: Dict):
        key = (row.get("design", ""), row.get("flow", ""))
        if order is not None:
            return (order.get(key, 10**9), key)
        return key

    with open(csvPath, "w", newline="", encoding="utf-8") as outFile:
        writer = csv.DictWriter(outFile, fieldnames=csvFields, extrasaction="ignore")
        writer.writeheader()
        for row in sorted(rows, key=sortKey):
            writer.writerow({field: row.get(field, "") for field in csvFields})


# USE: merge one row into the live results csv under an exclusive lock.
def upsertCsvRow(
    csvPath: Path,
    row: Dict,
    order: Optional[Dict[Tuple[str, str], int]] = None,
) -> None:
    design = row.get("design", "")
    flow = row.get("flow", "")
    if not design or not flow:
        return

    def merge() -> None:
        index: Dict[Tuple[str, str], Dict] = {}
        for existing in loadCsvRows(csvPath):
            key = (existing.get("design", ""), existing.get("flow", ""))
            if key[0] and key[1]:
                index[key] = existing
        key = (design, flow)
        previous = index.get(key, {})
        merged = dict(previous)
        for field, value in row.items():
            if value == "" or value is None:
                continue
            merged[field] = value
        if "status" in row and row["status"]:
            merged["status"] = preferStatus(
                str(previous.get("status", "")), str(row["status"])
            )
        elif "status" not in merged:
            merged["status"] = "pending"
        merged["design"] = design
        merged["flow"] = flow
        index[key] = merged
        writeCsv(csvPath, list(index.values()), order=order)

    withCsvLock(csvPath, merge)


def formatSummary(runLabel: str, row: Dict, status: Optional[str] = None) -> str:
    if status is None:
        status = row.get("status") or (
            "route done" if row.get("success") else "fail"
        )
    parts = [f"{runLabel}: {status}"]
    for shortKey, field in statusKeys:
        value = row.get(field, "")
        if value == "" or value is None:
            continue
        parts.append(f"{shortKey}={value}")
    return " ".join(parts)


def countBlifNames(blifPath: Path) -> str:
    if not blifPath.is_file():
        return ""
    try:
        count = 0
        with open(blifPath, encoding="utf-8", errors="replace") as blifFile:
            for line in blifFile:
                if line.startswith(".names"):
                    count += 1
        return str(count)
    except OSError:
        return ""


def countBlifLatches(blifPath: Path) -> str:
    if not blifPath.is_file():
        return ""
    try:
        count = 0
        with open(blifPath, encoding="utf-8", errors="replace") as blifFile:
            for line in blifFile:
                if line.startswith(".latch"):
                    count += 1
        return str(count)
    except OSError:
        return ""


# USE: sum $_DFF_*/$_DFFE_* counts from a yosys stat dump.
def parseYosysFfCount(text: str) -> str:
    match = re.search(r"^\s+Number of cells:\s+\d+\s*$", text, re.MULTILINE)
    total = 0
    found = False
    for line in text.splitlines():
        cellMatch = re.match(r"^\s+(\$_DFF\S*|\$_DFFE\S*)\s+(\d+)\s*$", line)
        if cellMatch:
            total += int(cellMatch.group(2))
            found = True
    return str(total) if found else ""


def stageLutCounts(tempDir: Path, design: str) -> Dict[str, str]:
    counts = {"synth_luts": "", "abc_luts": "", "synth_ff": ""}
    for key, names in (
        ("synth_luts", (f"{design}.mosaic.blif", f"{design}.parmys.blif")),
        ("abc_luts", (f"{design}.pre-vpr.blif",)),
    ):
        for name in names:
            path = tempDir / name
            value = countBlifNames(path)
            if value:
                counts[key] = value
                break
    # mosaic abc is in-yosys, so pre-vpr is the post-abc netlist. when only
    # mosaic.blif exists, treat that as synth and leave abc blank.
    if counts["synth_luts"] and not counts["abc_luts"]:
        preVpr = tempDir / f"{design}.pre-vpr.blif"
        frank = tempDir / f"{design}.mosaic.blif"
        if preVpr.is_file() and frank.is_file():
            counts["abc_luts"] = countBlifNames(preVpr)
    for logName in ("mosaic.out", "parmys.out"):
        logPath = tempDir / logName
        if not logPath.is_file():
            continue
        try:
            ffCount = parseYosysFfCount(
                logPath.read_text(encoding="utf-8", errors="replace")
            )
        except OSError:
            ffCount = ""
        if ffCount:
            counts["synth_ff"] = ffCount
            break
    if not counts["synth_ff"]:
        synthBlif = tempDir / f"{design}.mosaic.blif"
        if not synthBlif.is_file():
            synthBlif = tempDir / f"{design}.parmys.blif"
        counts["synth_ff"] = countBlifLatches(synthBlif)
    return counts


# USE: true when the frontend has written a blif that vpr can consume.
def frontendReady(tempDir: Path, design: str, flowName: str) -> bool:
    if flowName == "mosaic":
        return (tempDir / f"{design}.mosaic.blif").is_file() or (
            tempDir / f"{design}.pre-vpr.blif"
        ).is_file()
    return (tempDir / f"{design}.pre-vpr.blif").is_file()


# USE: optional abc gap from parmys.blif mtime to pre-vpr.blif mtime.
def abcWallFromMtimes(tempDir: Path, design: str) -> str:
    parmysBlif = tempDir / f"{design}.parmys.blif"
    preVpr = tempDir / f"{design}.pre-vpr.blif"
    if not parmysBlif.is_file() or not preVpr.is_file():
        return ""
    try:
        abcSec = max(0.0, preVpr.stat().st_mtime - parmysBlif.stat().st_mtime)
    except OSError:
        return ""
    return f"{abcSec:.2f}"


# derive wall/synth from timestamps. vpr uses VPR's own runtime when present
# wall  vpr_finish - start
# synth synth_finish - start
# vpr   "The entire flow of VPR took" or vpr_finish - synth_finish
def timesFromTimestamps(
    startUnix: float,
    synthFinishUnix: Optional[float],
    vprFinishUnix: float,
    tempDir: Path,
    design: str,
    vprRuntimeSec: str = "",
) -> Dict[str, str]:
    if synthFinishUnix is None:
        synthFinishUnix = vprFinishUnix
    synthFinishUnix = min(max(synthFinishUnix, startUnix), vprFinishUnix)
    wallSec = max(0.0, vprFinishUnix - startUnix)
    synthSec = max(0.0, synthFinishUnix - startUnix)
    if vprRuntimeSec:
        try:
            vprSec = max(0.0, float(vprRuntimeSec))
        except ValueError:
            vprSec = max(0.0, vprFinishUnix - synthFinishUnix)
    else:
        vprSec = max(0.0, vprFinishUnix - synthFinishUnix)
    return {
        "start_unix": f"{startUnix:.3f}",
        "synth_finish_unix": f"{synthFinishUnix:.3f}",
        "vpr_finish_unix": f"{vprFinishUnix:.3f}",
        "wall_time_sec": f"{wallSec:.2f}",
        "synth_wall_sec": f"{synthSec:.2f}",
        "vpr_wall_sec": f"{vprSec:.2f}",
        "synthesis_sec": f"{synthSec:.2f}",
        "abc_wall_sec": abcWallFromMtimes(tempDir, design),
    }


# USE: newest prior compare_results_*.csv in outdir (for --no-rerun reuse).
def latestResultsCsv(outDir: Path, exclude: Optional[Path] = None) -> Optional[Path]:
    excludeResolved = exclude.resolve() if exclude is not None else None
    candidates = sorted(outDir.glob("compare_results*.csv"))
    for path in reversed(candidates):
        if excludeResolved is not None and path.resolve() == excludeResolved:
            continue
        try:
            if path.stat().st_size <= 0:
                continue
            with open(path, newline="", encoding="utf-8") as handle:
                reader = csv.DictReader(handle)
                if reader.fieldnames and any(reader):
                    return path
        except OSError:
            continue
    return None


# USE: index prior csv rows by (design, flow) for --no-rerun cache hits.
def loadCsvIndex(csvPath: Path) -> Dict[Tuple[str, str], Dict]:
    index: Dict[Tuple[str, str], Dict] = {}
    if not csvPath.is_file():
        return index
    try:
        with open(csvPath, newline="", encoding="utf-8") as handle:
            for row in csv.DictReader(handle):
                design = (row.get("design") or "").strip()
                flow = (row.get("flow") or "").strip()
                if design and flow:
                    index[(design, flow)] = dict(row)
    except OSError:
        return {}
    return index


# USE: normalize a csv row reloaded for a cached --no-rerun hit.
def rowFromPriorCsv(priorRow: Dict) -> Dict:
    row = {field: priorRow.get(field, "") for field in csvFields}
    success = row.get("success", "")
    if isinstance(success, str):
        row["success"] = success.strip().lower() in ("1", "true", "yes")
    else:
        row["success"] = bool(success)
    try:
        row["return_code"] = int(row.get("return_code") or 0)
    except (TypeError, ValueError):
        row["return_code"] = 0
    row["vpr_status"] = "cached"
    row["status"] = "cached"
    return row


def parsePackedLuts(vprText: str) -> str:
    match = re.search(r"^\s+\.names\s*:\s*(\d+)\s*$", vprText, re.MULTILINE)
    return match.group(1) if match else ""


def parseVprFfCount(vprText: str) -> str:
    ffPbMatch = re.search(r"^\s+ff\s+:\s*(\d+)\s*$", vprText, re.MULTILINE)
    if ffPbMatch:
        return ffPbMatch.group(1)
    latchMatch = re.search(r"^\s+\.latch\s*:\s*(\d+)\s*$", vprText, re.MULTILINE)
    return latchMatch.group(1) if latchMatch else ""


def parseVprQor(tempDir: Path) -> Dict[str, str]:
    vprOut = tempDir / "vpr.out"
    metrics = {
        field: ""
        for field in csvFields
        if field
        not in (
            "design",
            "flow",
            "status",
            "success",
            "verilator_status",
            "verilator_matched",
            "verilator_mismatched",
            "verilator_vectors",
            "verilator_port_errors",
            "wall_time_sec",
            "return_code",
            "start_unix",
            "synth_finish_unix",
            "vpr_finish_unix",
            "synth_wall_sec",
            "abc_wall_sec",
            "vpr_wall_sec",
            "synth_luts",
            "abc_luts",
            "synth_ff",
            "synthesis_sec",
        )
    }
    metrics["vpr_status"] = "missing"
    if not vprOut.is_file() or vprOut.stat().st_size == 0:
        return metrics

    text = vprOut.read_text(encoding="utf-8", errors="replace")
    if re.search(r"Final critical path", text) or (
        "The entire flow of VPR took" in text and "Netlist clb blocks:" in text
    ):
        metrics["vpr_status"] = "ok"
    elif re.search(r"\bfailed\b", text, re.IGNORECASE):
        metrics["vpr_status"] = "fail"

    blockCounts: Dict[str, int] = {}
    for blockMatch in re.finditer(r"Netlist (\S+) blocks:\s*(\d+)", text):
        blockCounts[blockMatch.group(1)] = int(blockMatch.group(2))
    if "clb" in blockCounts:
        metrics["num_clb"] = str(blockCounts["clb"])
    metrics["num_memory"] = str(sum(blockCounts.get(b, 0) for b in vprBramBlockTypes))
    metrics["num_dsp"] = str(sum(blockCounts.get(b, 0) for b in vprDspBlockTypes))

    ioIn = re.search(r"Netlist inputs pins:\s*(\d+)", text)
    if ioIn:
        metrics["num_io_in"] = ioIn.group(1)
    ioOut = re.search(r"Netlist output pins:\s*(\d+)", text)
    if ioOut:
        metrics["num_io_out"] = ioOut.group(1)

    metrics["packed_luts"] = parsePackedLuts(text)
    metrics["num_ff"] = parseVprFfCount(text)

    adderPbMatch = re.search(r"^\s+adder\s+:\s*(\d+)", text, re.MULTILINE)
    if adderPbMatch:
        metrics["num_adder"] = adderPbMatch.group(1)

    wnsMatch = re.search(
        r"worst.negative.slack[^:\n]*:\s*([0-9.-]+)\s*ns", text, re.IGNORECASE
    )
    if wnsMatch:
        metrics["worst_slack_ns"] = wnsMatch.group(1)

    wireMatch = re.search(r"Total wirelength:\s*(\d+)", text)
    if wireMatch:
        metrics["total_wire_length"] = wireMatch.group(1)

    runtimeMatch = re.search(r"The entire flow of VPR took ([0-9.]+) seconds", text)
    vprRuntime = runtimeMatch.group(1) if runtimeMatch else ""

    critFile = tempDir / "vpr.crit_path.out"
    critText = (
        critFile.read_text(encoding="utf-8", errors="replace")
        if critFile.is_file()
        else text
    )
    critMatch = re.search(
        r"Final critical path[^:]*:\s*([0-9.]+)\s*ns(?:,\s*Fmax:\s*([0-9.]+)\s*MHz)?",
        critText,
    )
    if critMatch:
        cpd = float(critMatch.group(1))
        metrics["crit_path_delay_ns"] = critMatch.group(1)
        if critMatch.group(2):
            metrics["fmax_mhz"] = critMatch.group(2)
        elif cpd > 0:
            metrics["fmax_mhz"] = f"{1000.0 / cpd:.2f}"

    metrics["_vpr_runtime_sec"] = vprRuntime
    return metrics


def extractFailReason(tempDir: Path, flowName: str, returnCode: int) -> str:
    stageName = "mosaic" if flowName == "mosaic" else "parmys"
    candidates = (
        tempDir / "output.txt",
        tempDir / f"{stageName}.out",
        tempDir / "vpr.out",
    )
    errorRe = re.compile(
        r"(ERROR[:\s].+|error:\s.+|can't open command file.+|No such file.+|"
        r"unknown command:.+|TCL interpreter returned an error|"
        r"failed to execute.+|Assert.+|EXCEPTION.+|"
        r"vpr_status=.+|exited with return code \d+)",
        re.IGNORECASE,
    )
    for path in candidates:
        if not path.is_file() or path.stat().st_size == 0:
            continue
        try:
            text = path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        matches = errorRe.findall(text)
        if not matches:
            continue
        snippet = re.sub(r"\s+", " ", matches[-1]).strip()
        if len(snippet) > 120:
            snippet = snippet[:117] + "..."
        return f"{path.name}: {snippet}"
    if returnCode == 139:
        return "rc=139 (segfault)"
    if returnCode != 0:
        return f"rc={returnCode}"
    return "missing"


# USE: force each child process onto one worker/thread.
def singleCoreEnv() -> Dict[str, str]:
    env = os.environ.copy()
    env["VPR_NUM_WORKERS"] = "1"
    env["OMP_NUM_THREADS"] = "1"
    env["MKL_NUM_THREADS"] = "1"
    env["OPENBLAS_NUM_THREADS"] = "1"
    return env


def runOne(task: Tuple) -> Dict:
    (
        design,
        flowName,
        outDir,
        archFile,
        benchDir,
        noClean,
        noRerun,
        includeFiles,
        routeChanWidth,
        priorRow,
        csvPath,
        order,
        verilatorCheck,
        verilatorVectors,
        verilatorSeed,
    ) = task
    runLabel = f"{design}_{flowName}"
    tempDir = (outDir / "runs" / runLabel).resolve()
    logPath = (outDir / "logs" / f"{runLabel}.log").resolve()
    successMarker = tempDir / ".success"
    circuitPath = benchDir / f"{design}.v"

    def publish(row: Dict) -> None:
        upsertCsvRow(csvPath, row, order=order)

    if noRerun and successMarker.is_file():
        if priorRow:
            row = rowFromPriorCsv(priorRow)
        else:
            row = {
                "design": design,
                "flow": flowName,
                "status": "cached",
                "success": True,
                "vpr_status": "cached",
                "return_code": 0,
            }
        publish(row)
        return row

    if not circuitPath.is_file():
        row = {
            "design": design,
            "flow": flowName,
            "status": "fail",
            "success": False,
            "vpr_status": "missing_verilog",
            "wall_time_sec": "",
            "return_code": 1,
        }
        publish(row)
        return row

    if tempDir.exists() and not noClean:
        shutil.rmtree(tempDir)
    tempDir.mkdir(parents=True, exist_ok=True)
    logPath.parent.mkdir(parents=True, exist_ok=True)

    # default vtr call uses -j/--num_workers 1 so vpr stays on one core per run.
    cmd = [
        sys.executable,
        str(runVtrFlow.resolve()),
        str(circuitPath.resolve()),
        str(archFile.resolve()),
        "-start",
        flows[flowName]["start"],
        "-temp_dir",
        str(tempDir),
        "-name",
        runLabel,
        "-track_memory_usage",
        "--pack",
        "--place",
        "--route",
        "--analysis",
        "--route_chan_width",
        str(routeChanWidth),
        "-crit_path_router_iterations",
        "100",
        "--num_workers",
        "1",
    ]
    if includeFiles:
        cmd += ["-include", *[str(path) for path in includeFiles]]
    if verilatorCheck:
        cmd += [
            "-verilator_check",
            "-verilator_check_vectors",
            str(verilatorVectors),
            "-verilator_check_seed",
            str(verilatorSeed),
        ]

    # wall is process lifetime. synth is until frontend blif. vpr is VPR's own runtime
    startUnix = time.time()
    synthFinishUnix = None
    liveStatus = "started"
    publish({"design": design, "flow": flowName, "status": liveStatus})
    with open(logPath, "w", encoding="utf-8", errors="replace") as logFile:
        logFile.write("CMD: " + " ".join(cmd) + "\n\n")
        logFile.flush()
        proc = subprocess.Popen(
            cmd,
            cwd=str(vtrRoot),
            stdout=logFile,
            stderr=subprocess.STDOUT,
            text=True,
            env=singleCoreEnv(),
        )
        while True:
            returnCode = proc.poll()
            if synthFinishUnix is None and frontendReady(tempDir, design, flowName):
                synthFinishUnix = time.time()
            phase = detectPhase(tempDir)
            nextStatus = preferStatus(liveStatus, phase)
            if nextStatus != liveStatus:
                liveStatus = nextStatus
                publish({"design": design, "flow": flowName, "status": liveStatus})
            if returnCode is not None:
                break
            time.sleep(0.25)
    vprFinishUnix = time.time()
    if synthFinishUnix is None and frontendReady(tempDir, design, flowName):
        synthFinishUnix = vprFinishUnix

    qor = parseVprQor(tempDir)
    vprRuntime = qor.pop("_vpr_runtime_sec", "")
    vprOut = tempDir / "vpr.out"
    if not vprRuntime and (not vprOut.is_file() or vprOut.stat().st_size == 0):
        vprRuntime = "0"
    stageTimes = timesFromTimestamps(
        startUnix,
        synthFinishUnix,
        vprFinishUnix,
        tempDir,
        design,
        vprRuntimeSec=vprRuntime,
    )
    qor.update(stageLutCounts(tempDir, design))
    qor.update(stageTimes)
    if qor.get("vpr_status") == "missing":
        qor["vpr_status"] = extractFailReason(tempDir, flowName, returnCode)
    success = returnCode == 0 and qor["vpr_status"] == "ok"
    vcheck = readVerilatorMetrics(tempDir, verilatorCheck)
    # final identity/status fields override any same-named keys from qor
    row = {
        **qor,
        "design": design,
        "flow": flowName,
        "status": "route done" if success else "fail",
        "success": success,
        "return_code": returnCode,
        **vcheck,
    }
    if success:
        try:
            successMarker.touch()
        except OSError:
            pass

    publish(row)
    return row


# snapshot of verilator random-check from verilator_random_check.out
# pass  rtl and post-synth matched
# fail  sim ran and found mismatches
# error could not compile or run the check
def readVerilatorMetrics(tempDir: Path, enabled: bool) -> Dict[str, str]:
    empty = {
        "verilator_status": "",
        "verilator_matched": "",
        "verilator_mismatched": "",
        "verilator_vectors": "",
        "verilator_port_errors": "",
    }
    if not enabled:
        return empty
    logPath = tempDir / "verilator_random_check.out"
    if not logPath.is_file():
        return {**empty, "verilator_status": "error"}
    try:
        text = logPath.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return {**empty, "verilator_status": "error"}

    metrics = dict(empty)
    summary = re.search(
        r"VCHECK_SUMMARY\s+matched=(\d+)\s+mismatched=(\d+)\s+"
        r"vectors=(\d+)\s+port_errors=(\d+)",
        text,
    )
    if summary:
        metrics["verilator_matched"] = summary.group(1)
        metrics["verilator_mismatched"] = summary.group(2)
        metrics["verilator_vectors"] = summary.group(3)
        metrics["verilator_port_errors"] = summary.group(4)

    toolError = bool(
        re.search(
            r"verilator not on PATH|ERROR:|yosys blif|missing post-synth|"
            r"script missing",
            text,
        )
    )
    if "vectors matched across rtl" in text:
        metrics["verilator_status"] = "pass"
    elif summary and int(summary.group(2)) > 0:
        metrics["verilator_status"] = "fail"
    elif re.search(r"FAIL:\s+\d+\s+mismatches", text, re.IGNORECASE):
        metrics["verilator_status"] = "fail"
    elif toolError:
        metrics["verilator_status"] = "error"
    else:
        metrics["verilator_status"] = "error"
    return metrics


def runPool(
    tasks: List[Tuple],
    jobs: int,
    csvPath: Path,
    order: Dict[Tuple[str, str], int],
    quiet: bool,
) -> List[Dict]:
    liveResults: List[Dict] = []

    if not tasks:
        return []

    if jobs <= 1 or len(tasks) <= 1:
        for task in tasks:
            liveResults.append(runOne(task))
            if not quiet:
                print(formatSummary(f"{task[0]}_{task[1]}", liveResults[-1]))
        return liveResults

    with ProcessPoolExecutor(max_workers=jobs) as pool:
        futures = {pool.submit(runOne, task): task for task in tasks}
        for future in as_completed(futures):
            row = future.result()
            liveResults.append(row)
            if not quiet:
                print(formatSummary(f"{row['design']}_{row['flow']}", row))
    liveResults.sort(key=lambda row: order.get((row["design"], row["flow"]), 0))
    return liveResults


def startWatch(outDir: Path, interval: float) -> subprocess.Popen:
    watchScript = scriptDir / "watch_compare.py"
    return subprocess.Popen(
        [
            sys.executable,
            str(watchScript),
            "--dir",
            str(outDir),
            "--interval",
            str(interval),
        ],
        cwd=str(vtrRoot),
    )


def stopWatch(proc: Optional[subprocess.Popen]) -> None:
    if proc is None:
        return
    if proc.poll() is not None:
        return
    proc.terminate()
    try:
        proc.wait(timeout=2.0)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait(timeout=2.0)


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(
        description="batch run_vtr_flow.py (1 core per run) with live csv"
    )
    parser.add_argument("--arch", required=True, help="architecture xml")
    parser.add_argument(
        "--benchmark-dir",
        required=True,
        help="directory holding <design>.v files",
    )
    parser.add_argument(
        "--designs",
        nargs="+",
        default=None,
        help="circuit stems (default: every *.v in --benchmark-dir except *_include.v)",
    )
    parser.add_argument(
        "--include",
        nargs="+",
        default=None,
        help="include files for -include (relative to --benchmark-dir or absolute)",
    )
    parser.add_argument("--flows", nargs="+", default=None)
    parser.add_argument(
        "--jobs",
        type=int,
        default=4,
        help="number of concurrent single-core runs (default 4)",
    )
    parser.add_argument("--outdir", default=None)
    parser.add_argument("--csv", default=None)
    parser.add_argument("--route-chan-width", type=int, default=300)
    parser.add_argument("--no-clean", action="store_true")
    parser.add_argument("--no-rerun", action="store_true")
    parser.add_argument(
        "--verilator-check",
        nargs="+",
        metavar="FLOW",
        default=None,
        help="after synth(+abc), rtl vs post-synth random-check on these flows "
        "(mosaic and/or vanilla_vtr; aliases: frank, parmys, vtr)",
    )
    parser.add_argument(
        "--verilator-vectors",
        type=int,
        default=50000,
        help="vector count for --verilator-check (default 50000)",
    )
    parser.add_argument(
        "--verilator-seed",
        type=int,
        default=1,
        help="seed for --verilator-check (default 1)",
    )
    parser.add_argument(
        "--watch",
        action="store_true",
        help="spawn watch_compare.py in this terminal",
    )
    parser.add_argument(
        "--watch-interval",
        type=float,
        default=1.0,
        help="watch refresh seconds (default 1.0)",
    )
    args = parser.parse_args(argv)

    archFile = resolvePath(args.arch, vtrRoot)
    benchDir = resolvePath(args.benchmark_dir, vtrRoot)
    designs = resolveDesigns(args.designs, benchDir)
    selectedFlows = normalizeFlows(args.flows)
    vcheckFlows = normalizeFlows(args.verilator_check) if args.verilator_check else []
    missingVcheck = [flow for flow in vcheckFlows if flow not in selectedFlows]
    if missingVcheck:
        parser.error(
            "--verilator-check flow not in --flows: "
            + ", ".join(missingVcheck)
            + " (selected: "
            + ", ".join(selectedFlows)
            + ")"
        )
    jobs = max(1, args.jobs)
    includeFiles = resolveIncludePaths(args.include, benchDir)
    outDir = resolvePath(
        args.outdir or f"compare_output_{archFile.stem}", compareOutputRoot
    )
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    csvPath = (
        resolvePath(args.csv, vtrRoot)
        if args.csv
        else (outDir / f"compare_results_{stamp}.csv")
    )

    checkPrerequisites("mosaic" in selectedFlows, archFile, benchDir)

    outDir.mkdir(parents=True, exist_ok=True)
    (outDir / "runs").mkdir(exist_ok=True)
    (outDir / "logs").mkdir(exist_ok=True)
    statusDir = outDir / "status"
    if statusDir.exists() and not args.no_rerun:
        shutil.rmtree(statusDir, ignore_errors=True)
    statusDir.mkdir(exist_ok=True)

    labels = [f"{design}_{flow}" for design in designs for flow in selectedFlows]
    (statusDir / "manifest.txt").write_text("\n".join(labels) + "\n", encoding="utf-8")
    (statusDir / "csv_path.txt").write_text(str(csvPath.resolve()) + "\n", encoding="utf-8")
    if vcheckFlows:
        (statusDir / "verilator_check").write_text(
            " ".join(vcheckFlows) + "\n", encoding="utf-8"
        )
    else:
        marker = statusDir / "verilator_check"
        if marker.is_file():
            marker.unlink()

    # --no-rerun reloads timing/qor for cached runs from the newest prior csv.
    priorIndex: Dict[Tuple[str, str], Dict] = {}
    if args.no_rerun:
        priorCsv = latestResultsCsv(outDir, exclude=csvPath)
        if priorCsv is not None:
            priorIndex = loadCsvIndex(priorCsv)
            print(f"prior:   {priorCsv} ({len(priorIndex)} rows)")

    order = {
        (design, flowName): i
        for i, (design, flowName) in enumerate(
            (design, flowName)
            for design in designs
            for flowName in selectedFlows
        )
    }
    # seed the csv with every run so watch_compare can read status-only rows.
    seedRows = [
        {
            "design": design,
            "flow": flowName,
            "status": "pending",
            "success": "",
        }
        for design in designs
        for flowName in selectedFlows
    ]
    writeCsv(csvPath, seedRows, order=order)

    tasks = [
        (
            design,
            flowName,
            outDir,
            archFile,
            benchDir,
            args.no_clean,
            args.no_rerun,
            includeFiles,
            args.route_chan_width,
            priorIndex.get((design, flowName)),
            csvPath,
            order,
            flowName in vcheckFlows,
            args.verilator_vectors,
            args.verilator_seed,
        )
        for design in designs
        for flowName in selectedFlows
    ]

    print(f"arch:    {archFile}")
    print(f"bench:   {benchDir}")
    print(f"designs: {', '.join(designs)}")
    print(f"flows:   {', '.join(selectedFlows)}")
    if vcheckFlows:
        print(f"vcheck:  {', '.join(vcheckFlows)}")
    if includeFiles:
        print(f"include: {', '.join(path.name for path in includeFiles)}")
    print(f"jobs:    {jobs} concurrent runs x 1 core each")
    print(f"outdir:  {outDir}")
    print(f"csv:     {csvPath}")
    if not args.watch:
        print(
            f"watch:   python3 mosaic/scripts/watch_compare.py --dir {outDir}"
        )
    print(
        f"launching {len(designs)} designs x {len(selectedFlows)} flows "
        f"= {len(tasks)} runs"
    )
    print()

    watchProc = None
    if args.watch:
        watchProc = startWatch(outDir, max(0.2, args.watch_interval))

    try:
        rows = runPool(tasks, jobs, csvPath, order, quiet=args.watch)
    except KeyboardInterrupt:
        stopWatch(watchProc)
        print("\ninterrupted.")
        return 130
    finally:
        stopWatch(watchProc)

    # live upserts already wrote each row; rewrite once in stable launch order.
    for row in rows:
        upsertCsvRow(csvPath, row, order=order)
    writeCsv(csvPath, loadCsvRows(csvPath), order=order)
    ok = sum(1 for row in rows if row.get("success"))
    fail = len(rows) - ok
    print()
    print(f"done: {ok} ok, {fail} failed  ->  {csvPath}")
    return 0 if fail == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
