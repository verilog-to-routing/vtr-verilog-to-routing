#!/usr/bin/env python3
"""
batch runner built around the vtr_flow/scripts/run_vtr_flow.py call.

details:
    each job is python3 vtr_flow/scripts/run_vtr_flow.py <circuit.v> <arch.xml> -start <stage>

    every run is pinned to 1 core (--num_workers 1 + VPR_NUM_WORKERS=1 +
    OMP_NUM_THREADS=1). --jobs N means N of those single-core runs in parallel.

    writes live status + csv under mosaic/scripts/compare_output_<arch_stem>/.
    with --watch, spawns watch_compare.py in this terminal (or run it yourself
    in a second one).

usage:
    python3 mosaic/scripts/run_vtr_batch.py \\
        --arch vtr_flow/arch/COFFE_22nm/k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml \\
        --benchmark-dir vtr_flow/benchmarks/verilog/koios \\
        --designs eltwise_layer conv_layer gemm_layer lenet \\
        --include hard_block_include.v \\
        --jobs 4 --watch

flags:
    --arch <xml>           architecture file (required)
    --benchmark-dir <dir>  directory holding <design>.v
    --designs <names...>   circuit stems; default: every *.v in bench dir
                            except *_include.v
    --include <files...>   -include paths (relative to --benchmark-dir or abs)
    --flows <names...>     mosaic and/or vanilla_vtr (default: both)
                            aliases: frank, parmys, vtr
    --jobs <n>             concurrent single-core runs (default 4);
                            each run is pinned to 1 core via --num_workers 1
    --outdir <dir>         output directory (default
                            mosaic/scripts/compare_output_<arch_stem>)
    --csv <path>           results csv (default timestamped under outdir)
    --route-chan-width N   passed to vpr (default 300)
    --no-clean             keep existing run dirs
    --no-rerun             skip runs that already have a .success marker
    --watch                spawn watch_compare.py in this terminal
    --watch-interval <s>   watch refresh period (default 1.0)
"""

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

scriptDir = Path(__file__).resolve().parent
vtrRoot = scriptDir.parents[1]
# default batch output root (keeps compare_output_* out of the repo root)
compareOutputRoot = scriptDir
vtrFlow = vtrRoot / "vtr_flow"
runVtrFlow = vtrFlow / "scripts" / "run_vtr_flow.py"
yosysBin = vtrRoot / "build" / "bin" / "yosys"
pluginPath = vtrRoot / "build" / "share" / "yosys" / "plugins" / "wildebeest.so"

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
# koios uses dsp_top; classic k6 uses mult_36; some arches use mae/dsp
vprDspBlockTypes = ("mult_36", "mae", "dsp", "dsp_top")

csvFields = (
    "design",
    "flow",
    "success",
    "vpr_status",
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

statusKeys = (
    ("wall", "wall_time_sec"),
    ("s_s", "synth_wall_sec"),
    ("a_s", "abc_wall_sec"),
    ("v_s", "vpr_wall_sec"),
    ("synthesis", "synthesis_sec"),
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


def writeStatus(outDir: Path, runLabel: str, summaryLine: str) -> None:
    statusDir = outDir / "status"
    statusDir.mkdir(parents=True, exist_ok=True)
    try:
        (statusDir / f"{runLabel}.txt").write_text(summaryLine + "\n", encoding="utf-8")
    except OSError:
        pass


def writeCsv(csvPath: Path, rows: List[Dict]) -> None:
    csvPath.parent.mkdir(parents=True, exist_ok=True)
    with open(csvPath, "w", newline="", encoding="utf-8") as outFile:
        writer = csv.DictWriter(outFile, fieldnames=csvFields, extrasaction="ignore")
        writer.writeheader()
        for row in sorted(rows, key=lambda r: (r.get("design", ""), r.get("flow", ""))):
            writer.writerow({field: row.get(field, "") for field in csvFields})


def formatSummary(runLabel: str, row: Dict, status: Optional[str] = None) -> str:
    if status is None:
        status = "ok" if row.get("success") else f"FAIL ({row.get('vpr_status', '')})"
    parts = [f"{runLabel}: {status}"]
    for shortKey, field in statusKeys:
        value = row.get(field, "")
        if value == "" or value is None:
            continue
        parts.append(f"{shortKey}={value}")
    return " ".join(parts)


def parseTimeSeconds(text: str) -> Optional[float]:
    wallMatch = None
    userMatch = None
    for line in text.splitlines():
        found = re.search(r"Elapsed \(wall clock\) time.*?:\s*(\S+)", line)
        if found:
            wallMatch = found.group(1)
        found = re.search(r"User time \(seconds\):\s*([0-9.]+)", line)
        if found:
            userMatch = found.group(1)
    if wallMatch is not None:
        parts = wallMatch.split(":")
        try:
            seconds = float(parts[-1])
            for multiplier, part in zip((60, 3600), reversed(parts[:-1])):
                seconds += float(part) * multiplier
            return seconds
        except ValueError:
            pass
    if userMatch is not None:
        try:
            return float(userMatch)
        except ValueError:
            return None
    return None


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


def parseYosysFfCount(text: str) -> str:
    match = re.search(r"^\s+Number of cells:\s+\d+\s*$", text, re.MULTILINE)
    # prefer $_DFF_*/$_DFFE_* counts from yosys stat
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
    # mosaic abc is in-yosys so pre-vpr is the post-abc netlist; when
    # only mosaic.blif exists, treat that as synth and leave abc blank
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


def stageWallTimes(tempDir: Path, vprRuntimeSec: str = "") -> Dict[str, str]:
    times = {"synth_wall_sec": "", "abc_wall_sec": "", "vpr_wall_sec": ""}
    for key, names in (
        ("synth_wall_sec", ("mosaic.out", "parmys.out", "odin.out")),
        ("abc_wall_sec", ("abc0.out", "abc.out")),
        ("vpr_wall_sec", ("vpr.out",)),
    ):
        for name in names:
            path = tempDir / name
            if not path.is_file():
                continue
            try:
                value = parseTimeSeconds(
                    path.read_text(encoding="utf-8", errors="replace")
                )
            except OSError:
                value = None
            if value is not None:
                times[key] = f"{value:.2f}"
                break
    if not times["vpr_wall_sec"] and vprRuntimeSec:
        times["vpr_wall_sec"] = vprRuntimeSec
    return times


def fairSynthesisSec(flowName: str, synthWall: str, abcWall: str) -> str:
    try:
        synth = float(synthWall) if synthWall else None
    except ValueError:
        synth = None
    try:
        abc = float(abcWall) if abcWall else None
    except ValueError:
        abc = None
    if flowName == "mosaic":
        return f"{synth:.2f}" if synth is not None else ""
    if flowName == "vanilla_vtr":
        if synth is None and abc is None:
            return ""
        return f"{(synth or 0.0) + (abc or 0.0):.2f}"
    return f"{synth:.2f}" if synth is not None else ""


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
            "success",
            "wall_time_sec",
            "return_code",
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


def singleCoreEnv() -> Dict[str, str]:
    """force each child process onto one worker/thread."""
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
    ) = task
    runLabel = f"{design}_{flowName}"
    tempDir = (outDir / "runs" / runLabel).resolve()
    logPath = (outDir / "logs" / f"{runLabel}.log").resolve()
    successMarker = tempDir / ".success"
    circuitPath = benchDir / f"{design}.v"

    if noRerun and successMarker.is_file():
        summary = f"{runLabel}: cached"
        writeStatus(outDir, runLabel, summary)
        return {
            "design": design,
            "flow": flowName,
            "success": True,
            "vpr_status": "cached",
            "wall_time_sec": "",
            "return_code": 0,
        }

    if not circuitPath.is_file():
        row = {
            "design": design,
            "flow": flowName,
            "success": False,
            "vpr_status": "missing_verilog",
            "wall_time_sec": "",
            "return_code": 1,
        }
        writeStatus(outDir, runLabel, formatSummary(runLabel, row))
        return row

    if tempDir.exists() and not noClean:
        shutil.rmtree(tempDir)
    tempDir.mkdir(parents=True, exist_ok=True)
    logPath.parent.mkdir(parents=True, exist_ok=True)

    # default vtr call; -j/--num_workers 1 keeps vpr on one core per run
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

    startTime = time.perf_counter()
    with open(logPath, "w", encoding="utf-8", errors="replace") as logFile:
        logFile.write("CMD: " + " ".join(cmd) + "\n\n")
        logFile.flush()
        result = subprocess.run(
            cmd,
            cwd=str(vtrRoot),
            stdout=logFile,
            stderr=subprocess.STDOUT,
            text=True,
            check=False,
            env=singleCoreEnv(),
        )
    wallSec = f"{time.perf_counter() - startTime:.2f}"

    qor = parseVprQor(tempDir)
    vprRuntime = qor.pop("_vpr_runtime_sec", "")
    qor.update(stageLutCounts(tempDir, design))
    qor.update(stageWallTimes(tempDir, vprRuntime))
    qor["synthesis_sec"] = fairSynthesisSec(
        flowName, qor.get("synth_wall_sec", ""), qor.get("abc_wall_sec", "")
    )
    if qor.get("vpr_status") == "missing":
        qor["vpr_status"] = extractFailReason(tempDir, flowName, result.returncode)
    success = result.returncode == 0 and qor["vpr_status"] == "ok"
    row = {
        "design": design,
        "flow": flowName,
        "success": success,
        "wall_time_sec": wallSec,
        "return_code": result.returncode,
        **qor,
    }
    if success:
        try:
            successMarker.touch()
        except OSError:
            pass

    summary = formatSummary(runLabel, row)
    writeStatus(outDir, runLabel, summary)
    return row


def runPool(
    tasks: List[Tuple],
    jobs: int,
    csvPath: Path,
    order: Dict[Tuple[str, str], int],
    quiet: bool,
) -> List[Dict]:
    liveResults: List[Dict] = []

    def flush() -> None:
        merged = list(liveResults)
        merged.sort(key=lambda row: order.get((row["design"], row["flow"]), 0))
        writeCsv(csvPath, merged)

    if not tasks:
        writeCsv(csvPath, [])
        return []

    if jobs <= 1 or len(tasks) <= 1:
        for task in tasks:
            liveResults.append(runOne(task))
            flush()
            if not quiet:
                print(formatSummary(f"{task[0]}_{task[1]}", liveResults[-1]))
        return liveResults

    with ProcessPoolExecutor(max_workers=jobs) as pool:
        futures = {pool.submit(runOne, task): task for task in tasks}
        for future in as_completed(futures):
            row = future.result()
            liveResults.append(row)
            flush()
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
    writeCsv(csvPath, [])
    (statusDir / "csv_path.txt").write_text(str(csvPath.resolve()) + "\n", encoding="utf-8")

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
        )
        for design in designs
        for flowName in selectedFlows
    ]
    order = {(task[0], task[1]): i for i, task in enumerate(tasks)}

    print(f"arch:    {archFile}")
    print(f"bench:   {benchDir}")
    print(f"designs: {', '.join(designs)}")
    print(f"flows:   {', '.join(selectedFlows)}")
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

    writeCsv(csvPath, rows)
    ok = sum(1 for row in rows if row.get("success"))
    fail = len(rows) - ok
    print()
    print(f"done: {ok} ok, {fail} failed  ->  {csvPath}")
    return 0 if fail == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
