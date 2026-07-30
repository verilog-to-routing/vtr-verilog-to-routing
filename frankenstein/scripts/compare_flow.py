#!/usr/bin/env python3
"""
two-way vtr synthesis qor compare: vanilla_vtr (parmys) vs frankenstein.

runs circuits through both front-ends, then the same vpr backend on a chosen architecture.

usage (from the vtr repo root):
  python3 frankenstein/scripts/compare_flow.py
  python3 frankenstein/scripts/compare_flow.py --arch <path/to/arch.xml>
  python3 frankenstein/scripts/compare_flow.py --designs arm_core bgm --flows frankenstein
  python3 frankenstein/scripts/compare_flow.py --jobs 8
  python3 frankenstein/scripts/compare_flow.py --no-rerun
  python3 frankenstein/scripts/compare_flow.py --no-clean
  python3 frankenstein/scripts/compare_flow.py --outdir <dir> --csv <path.csv>

defaults:
  arch      vtr_flow/arch/timing/k6_frac_N10_frac_chain_mem32K_40nm.xml
  circuits  vtr_flow/benchmarks/verilog/<name>.v
  outdir    compare_output_<arch_stem>
  csv       <outdir>/compare_results_<YYYYMMDD_HHMMSS>.csv

flags:
  --arch <xml>           architecture file
  --benchmark-dir <dir>  directory holding <design>.v files
  --designs <names...>   circuit stems (default: eight readme circuits)
  --flows <names...>     vanilla_vtr and/or frankenstein (aliases: vtr, frank)
  --jobs <n>             parallel jobs (default 4)
  --outdir <dir>         output directory
  --csv <path>           results csv path (default: timestamped under outdir)
  --no-clean             keep existing run dirs
  --no-rerun             skip runs that already have a .success marker

watch progress in a second terminal:
  python3 frankenstein/scripts/watch_compare.py --dir <outdir>
"""

from __future__ import annotations

import argparse
import csv
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
vtrFlow = vtrRoot / "vtr_flow"
runVtrFlow = vtrFlow / "scripts" / "run_vtr_flow.py"
yosysBin = vtrRoot / "build" / "bin" / "yosys"
pluginPath = vtrRoot / "build" / "share" / "yosys" / "plugins" / "wildebeest.so"

defaultArchRel = "vtr_flow/arch/timing/k6_frac_N10_frac_chain_mem32K_40nm.xml"
defaultBenchRel = "vtr_flow/benchmarks/verilog"
defaultDesigns = (
    "LU32PEEng",
    "LU8PEEng",
    "arm_core",
    "bgm",
    "mcml",
    "stereovision0",
    "stereovision1",
    "stereovision2",
)

flows = {
    "vanilla_vtr": {"start": "parmys"},
    "frankenstein": {"start": "frankenstein"},
}
flowAliases = {
    "vtr": "vanilla_vtr",
    "vanilla": "vanilla_vtr",
    "parmys": "vanilla_vtr",
    "frank": "frankenstein",
}

vprArgs = (
    "--pack",
    "--place",
    "--route",
    "--analysis",
    "--route_chan_width",
    "300",
    "-crit_path_router_iterations",
    "100",
)

vprBramBlockTypes = ("memory", "bram_multimode")
vprDspBlockTypes = ("mult_36", "mae", "dsp")

# only metrics that have a reliable source in the stage logs / blifs / vpr.out
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

# status line keys consumed by watch_compare.py
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


def ensurePlScriptsExecutable() -> None:
    for plScript in (vtrFlow / "scripts").glob("*.pl"):
        try:
            plScript.chmod(plScript.stat().st_mode | 0o111)
        except OSError:
            pass


def checkPrerequisites(needFrankenstein: bool, archFile: Path, benchDir: Path) -> None:
    missing = []
    for path in (runVtrFlow, archFile, yosysBin):
        if not path.is_file():
            missing.append(str(path))
    if not benchDir.is_dir():
        missing.append(str(benchDir))
    if needFrankenstein and not pluginPath.is_file():
        missing.append(str(pluginPath))
    if missing:
        print("missing prerequisites:", file=sys.stderr)
        for path in missing:
            print(f"  {path}", file=sys.stderr)
        print(
            "build vtr first (`make -j$(nproc)`), then "
            "`bash frankenstein/build_frankenstein.sh` if using frankenstein",
            file=sys.stderr,
        )
        raise SystemExit(1)


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
    # yosys `stat` lines look like: "    $dff                           1234"
    # after dffunmap / techmap the cell may be $dff, $_DFF_P_, etc.
    total = 0
    found = False
    for line in text.splitlines():
        match = re.match(r"\s*(\S+)\s+(\d+)\s*$", line)
        if not match:
            continue
        cell = match.group(1).lower().lstrip("$")
        if "dff" in cell or cell == "dlatch" or "latch" in cell:
            total += int(match.group(2))
            found = True
    return str(total) if found else ""


def stageLutCounts(tempDir: Path, circuitStem: str) -> Dict[str, str]:
    counts = {"synth_luts": "", "abc_luts": "", "synth_ff": ""}
    synthBlif = None
    for path in (
        tempDir / f"{circuitStem}.frankenstein.blif",
        tempDir / f"{circuitStem}.parmys.blif",
        tempDir / f"{circuitStem}.odin.blif",
    ):
        if path.is_file():
            counts["synth_luts"] = countBlifNames(path)
            synthBlif = path
            break
    for path in (
        tempDir / f"{circuitStem}.abc.blif",
        tempDir / f"0_{circuitStem}.abc.blif",
    ):
        if path.is_file():
            counts["abc_luts"] = countBlifNames(path)
            break

    # synth ff: prefer yosys stat in frankenstein.out / parmys.out; fall back to
    # .latch count in the synth blif
    for logName in ("frankenstein.out", "parmys.out", "odin.out"):
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
    if not counts["synth_ff"] and synthBlif is not None:
        counts["synth_ff"] = countBlifLatches(synthBlif)
    return counts


def parseTimeSeconds(text: str) -> Optional[float]:
    # prefer gnu time -v wall clock; fall back to user time (vtr parse_config style)
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


def stageWallTimes(tempDir: Path, vprRuntimeSec: str = "") -> Dict[str, str]:
    times = {"synth_wall_sec": "", "abc_wall_sec": "", "vpr_wall_sec": ""}
    for key, names in (
        ("synth_wall_sec", ("frankenstein.out", "parmys.out", "odin.out")),
        ("abc_wall_sec", ("abc0.out", "abc.out")),
        ("vpr_wall_sec", ("vpr.out",)),
    ):
        for name in names:
            path = tempDir / name
            if not path.is_file():
                continue
            try:
                value = parseTimeSeconds(path.read_text(encoding="utf-8", errors="replace"))
            except OSError:
                value = None
            if value is not None:
                times[key] = f"{value:.2f}"
                break
    # vpr always prints its own total runtime even without time -v
    if not times["vpr_wall_sec"] and vprRuntimeSec:
        times["vpr_wall_sec"] = vprRuntimeSec
    return times


def fairSynthesisSec(flowName: str, synthWall: str, abcWall: str) -> str:
    # apples-to-apples synthesis time:
    # frankenstein.out already includes in-yosys abc, so use synth alone.
    # vanilla_vtr is parmys + a separate abc stage, so sum both.
    try:
        synth = float(synthWall) if synthWall else None
    except ValueError:
        synth = None
    try:
        abc = float(abcWall) if abcWall else None
    except ValueError:
        abc = None
    if flowName == "frankenstein":
        return f"{synth:.2f}" if synth is not None else ""
    if flowName == "vanilla_vtr":
        if synth is None and abc is None:
            return ""
        return f"{(synth or 0.0) + (abc or 0.0):.2f}"
    return f"{synth:.2f}" if synth is not None else ""


def parsePackedLuts(vprText: str) -> str:
    # circuit statistics block: "    .names:    1234"
    match = re.search(r"^\s+\.names\s*:\s*(\d+)\s*$", vprText, re.MULTILINE)
    if match:
        return match.group(1)
    match = re.search(
        r"Absorbed\s+\d+\s+LUT buffers.*?^\s*\.names\s*:?\s*(\d+)\s*$",
        vprText,
        re.M | re.S,
    )
    return match.group(1) if match else ""


def parseVprFfCount(vprText: str) -> str:
    # prefer packed pb-type usage ("  ff : N"), else circuit stats ".latch: N"
    ffPbMatch = re.search(r"^\s+ff\s+:\s*(\d+)\s*$", vprText, re.MULTILINE)
    if ffPbMatch:
        return ffPbMatch.group(1)
    latchMatch = re.search(r"^\s+\.latch\s*:\s*(\d+)\s*$", vprText, re.MULTILINE)
    if latchMatch:
        return latchMatch.group(1)
    return ""


def parseVprQor(tempDir: Path) -> Dict[str, str]:
    vprOut = tempDir / "vpr.out"
    metrics = {field: "" for field in csvFields if field not in (
        "design", "flow", "success", "wall_time_sec", "return_code",
        "synth_wall_sec", "abc_wall_sec", "vpr_wall_sec",
        "synth_luts", "abc_luts", "synth_ff",
    )}
    metrics["vpr_status"] = "missing"
    if not vprOut.is_file():
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


def writeStatus(outDir: Path, runLabel: str, summaryLine: str) -> None:
    statusDir = outDir / "status"
    statusDir.mkdir(parents=True, exist_ok=True)
    try:
        (statusDir / f"{runLabel}.txt").write_text(summaryLine + "\n", encoding="utf-8")
    except OSError:
        pass


def formatSummary(runLabel: str, row: Dict) -> str:
    status = "ok" if row.get("success") else f"FAIL ({row.get('vpr_status', '')})"
    parts = [f"{runLabel}: {status}"]
    for shortKey, field in statusKeys:
        value = row.get(field, "")
        if value == "" or value is None:
            continue
        if shortKey == "cpd":
            parts.append(f"{shortKey}={value}ns")
        elif shortKey == "fmax":
            parts.append(f"{shortKey}={value}MHz")
        elif shortKey == "wns":
            parts.append(f"{shortKey}={value}ns")
        elif shortKey == "wall":
            parts.append(f"{shortKey}={value}s")
        else:
            parts.append(f"{shortKey}={value}")
    return " ".join(parts)


def runOne(task: Tuple) -> Dict:
    design, flowName, outDir, archFile, benchDir, noClean, noRerun = task
    runLabel = f"{design}_{flowName}"
    tempDir = (outDir / "runs" / runLabel).resolve()
    logPath = (outDir / "logs" / f"{runLabel}.log").resolve()
    successMarker = tempDir / ".success"
    circuitPath = benchDir / f"{design}.v"

    if noRerun and successMarker.is_file():
        summary = f"{runLabel}: cached"
        writeStatus(outDir, runLabel, summary)
        print(summary)
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
        summary = formatSummary(runLabel, row)
        writeStatus(outDir, runLabel, summary)
        print(summary)
        return row

    if tempDir.exists() and not noClean:
        shutil.rmtree(tempDir)
    tempDir.mkdir(parents=True, exist_ok=True)
    logPath.parent.mkdir(parents=True, exist_ok=True)

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
        *vprArgs,
    ]

    print(f"--- {runLabel} ---")
    startTime = time.perf_counter()
    with open(logPath, "w", encoding="utf-8", errors="replace") as logFile:
        result = subprocess.run(
            cmd,
            cwd=str(vtrRoot),
            stdout=logFile,
            stderr=subprocess.STDOUT,
            text=True,
            check=False,
        )
    wallSec = f"{time.perf_counter() - startTime:.2f}"

    qor = parseVprQor(tempDir)
    vprRuntime = qor.pop("_vpr_runtime_sec", "")
    qor.update(stageLutCounts(tempDir, design))
    qor.update(stageWallTimes(tempDir, vprRuntime))
    qor["synthesis_sec"] = fairSynthesisSec(
        flowName, qor.get("synth_wall_sec", ""), qor.get("abc_wall_sec", "")
    )
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
    print(summary)
    return row


def writeCsv(csvPath: Path, rows: List[Dict]) -> None:
    csvPath.parent.mkdir(parents=True, exist_ok=True)
    with open(csvPath, "w", newline="", encoding="utf-8") as outFile:
        writer = csv.DictWriter(outFile, fieldnames=csvFields, extrasaction="ignore")
        writer.writeheader()
        for row in sorted(rows, key=lambda r: (r.get("design", ""), r.get("flow", ""))):
            writer.writerow({field: row.get(field, "") for field in csvFields})


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


def normalizeDesigns(names: Optional[Sequence[str]]) -> List[str]:
    return list(names) if names else list(defaultDesigns)


def resolvePath(value: str, base: Path) -> Path:
    path = Path(value)
    return path if path.is_absolute() else (base / path)


def runPool(tasks: List[Tuple], jobs: int) -> List[Dict]:
    if jobs <= 1 or len(tasks) <= 1:
        return [runOne(task) for task in tasks]

    results: List[Dict] = []
    with ProcessPoolExecutor(max_workers=jobs) as pool:
        futures = {pool.submit(runOne, task): task for task in tasks}
        for future in as_completed(futures):
            results.append(future.result())
    order = {(t[0], t[1]): i for i, t in enumerate(tasks)}
    results.sort(key=lambda row: order.get((row["design"], row["flow"]), 0))
    return results


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(
        description="vanilla_vtr vs frankenstein vtr qor compare"
    )
    parser.add_argument("--arch", default=defaultArchRel)
    parser.add_argument("--benchmark-dir", default=defaultBenchRel)
    parser.add_argument("--designs", nargs="+", default=None)
    parser.add_argument("--flows", nargs="+", default=None)
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--outdir", default=None)
    parser.add_argument(
        "--csv",
        default=None,
        help="results csv path (default <outdir>/compare_results_<timestamp>.csv)",
    )
    parser.add_argument("--no-clean", action="store_true")
    parser.add_argument("--no-rerun", action="store_true")
    args = parser.parse_args(argv)

    archFile = resolvePath(args.arch, vtrRoot)
    benchDir = resolvePath(args.benchmark_dir, vtrRoot)
    designs = normalizeDesigns(args.designs)
    selectedFlows = normalizeFlows(args.flows)
    jobs = max(1, args.jobs)

    outDir = resolvePath(args.outdir or f"compare_output_{archFile.stem}", vtrRoot)
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    csvPath = (
        resolvePath(args.csv, vtrRoot)
        if args.csv
        else (outDir / f"compare_results_{stamp}.csv")
    )

    checkPrerequisites("frankenstein" in selectedFlows, archFile, benchDir)
    ensurePlScriptsExecutable()

    outDir.mkdir(parents=True, exist_ok=True)
    (outDir / "runs").mkdir(exist_ok=True)
    (outDir / "logs").mkdir(exist_ok=True)
    statusDir = outDir / "status"
    if statusDir.exists() and not args.no_rerun:
        shutil.rmtree(statusDir, ignore_errors=True)
    statusDir.mkdir(exist_ok=True)

    tasks = [
        (design, flowName, outDir, archFile, benchDir, args.no_clean, args.no_rerun)
        for design in designs
        for flowName in selectedFlows
    ]
    labels = [f"{d}_{f}" for d, f, *_ in tasks]
    (statusDir / "manifest.txt").write_text("\n".join(labels) + "\n", encoding="utf-8")

    print(f"arch:    {archFile.name}")
    print(f"designs: {', '.join(designs)}")
    print(f"flows:   {', '.join(selectedFlows)}")
    print(f"jobs:    {jobs}")
    print(f"outdir:  {outDir}")
    print(f"csv:     {csvPath}")
    print(f"watch:   python3 frankenstein/scripts/watch_compare.py --dir {outDir}")
    print()

    print(
        f"launching {len(designs)} designs x {len(selectedFlows)} flows "
        f"= {len(tasks)} runs @ {jobs} jobs"
    )
    rows = runPool(tasks, jobs)
    writeCsv(csvPath, rows)

    ok = sum(1 for row in rows if row.get("success"))
    fail = len(rows) - ok
    print()
    print(f"done: {ok} ok, {fail} failed  ->  {csvPath}")
    return 0 if fail == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
