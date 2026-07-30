#!/usr/bin/env python3
"""
two-way vtr synthesis qor compare: vanilla_vtr (parmys) vs frankenstein.

runs circuits through both front-ends, then the same abc + vpr backend on a
chosen architecture.

usage (from the vtr repo root):
  python3 frankenstein/scripts/compare_flow.py
  python3 frankenstein/scripts/compare_flow.py --arch <path/to/arch.xml>
  python3 frankenstein/scripts/compare_flow.py --designs arm_core bgm --flows frankenstein
  python3 frankenstein/scripts/compare_flow.py --jobs 8 --large-jobs 3

defaults:
  arch      vtr_flow/arch/timing/k6_frac_N10_frac_chain_mem32K_40nm.xml
  circuits  vtr_flow/benchmarks/verilog/<name>.v
  outdir    compare_output_k6

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
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Tuple

# ---------------------------------------------------------------------------
# paths
# ---------------------------------------------------------------------------

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
largeDesigns = frozenset({"LU32PEEng", "LU8PEEng", "mcml", "bgm"})

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

# classic placer + fixed channel width (matches vtr_reg_qor_chain methodology)
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

csvFields = (
    "design",
    "flow",
    "success",
    "vpr_status",
    "wall_time_sec",
    "packed_luts",
    "packed_luts_2plus",
    "num_clb",
    "num_ff",
    "num_memory",
    "num_dsp",
    "num_adder",
    "total_wire_length",
    "crit_path_delay_ns",
    "fmax_mhz",
    "worst_slack_ns",
    "vpr_peak_mem_mb",
    "pack_time_sec",
    "place_time_sec",
    "route_time_sec",
    "vpr_runtime_sec",
    "return_code",
)

# ---------------------------------------------------------------------------
# helpers
# ---------------------------------------------------------------------------


def ensurePlScriptsExecutable() -> None:
    # abc.py calls blackbox_latches.pl; git sometimes strips +x
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


def parsePackedLutCounts(vprText: str) -> Dict[str, str]:
    metrics = {"packed_luts": "", "packed_luts_2plus": ""}
    match = re.search(
        r"Absorbed\s+(\d+)\s+LUT buffers.*?^\s*\.names\s*:?\s*(\d+)\s*$"
        r"((?:\s+\d+-LUT:\s*\d+\s*$)*)",
        vprText,
        re.M | re.S,
    )
    if not match:
        return metrics
    metrics["packed_luts"] = match.group(2)
    lutDist = {
        int(m.group(1)): int(m.group(2))
        for m in re.finditer(r"(\d+)-LUT:\s*(\d+)", match.group(3))
    }
    metrics["packed_luts_2plus"] = str(sum(v for k, v in lutDist.items() if k >= 2))
    return metrics


def parseVprQor(tempDir: Path) -> Dict[str, str]:
    vprOut = tempDir / "vpr.out"
    metrics = {
        "vpr_status": "missing",
        "num_clb": "",
        "num_ff": "",
        "num_memory": "",
        "num_dsp": "",
        "num_adder": "",
        "packed_luts": "",
        "packed_luts_2plus": "",
        "total_wire_length": "",
        "crit_path_delay_ns": "",
        "fmax_mhz": "",
        "worst_slack_ns": "",
        "vpr_peak_mem_mb": "",
        "pack_time_sec": "",
        "place_time_sec": "",
        "route_time_sec": "",
        "vpr_runtime_sec": "",
    }
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
    if "ff" in blockCounts:
        metrics["num_ff"] = str(blockCounts["ff"])

    metrics.update(parsePackedLutCounts(text))

    adderPbMatch = re.search(r"^\s+adder\s+:\s*(\d+)", text, re.MULTILINE)
    if adderPbMatch:
        metrics["num_adder"] = adderPbMatch.group(1)

    wnsMatch = re.search(
        r"worst.negative.slack[^:\n]*:\s*([0-9.-]+)\s*ns", text, re.IGNORECASE
    )
    if wnsMatch:
        metrics["worst_slack_ns"] = wnsMatch.group(1)

    memMatch = re.search(r"Maximum resident set size \(kbytes\):\s*(\d+)", text)
    if memMatch:
        metrics["vpr_peak_mem_mb"] = f"{int(memMatch.group(1)) / 1024:.1f}"

    wireMatch = re.search(r"Total wirelength:\s*(\d+)", text)
    if wireMatch:
        metrics["total_wire_length"] = wireMatch.group(1)

    runtimeMatch = re.search(r"The entire flow of VPR took ([0-9.]+) seconds", text)
    if runtimeMatch:
        metrics["vpr_runtime_sec"] = runtimeMatch.group(1)
    packMatch = re.search(r"# Packing took ([0-9.]+) seconds", text)
    if packMatch:
        metrics["pack_time_sec"] = packMatch.group(1)
    placeMatch = re.search(r"# Placement took ([0-9.]+) seconds", text)
    if placeMatch:
        metrics["place_time_sec"] = placeMatch.group(1)
    routeMatch = re.search(r"# Routing took ([0-9.]+) seconds", text)
    if routeMatch:
        metrics["route_time_sec"] = routeMatch.group(1)

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
    return (
        f"{runLabel}: {status} wall={row.get('wall_time_sec', '')}s "
        f"packed_luts={row.get('packed_luts', '')} "
        f"packed_luts_2plus={row.get('packed_luts_2plus', '')} "
        f"clb={row.get('num_clb', '')} ff={row.get('num_ff', '')} "
        f"mem={row.get('num_memory', '')} dsp={row.get('num_dsp', '')} "
        f"adder={row.get('num_adder', '')} "
        f"wl={row.get('total_wire_length', '')} "
        f"cpd={row.get('crit_path_delay_ns', '')}ns "
        f"fmax={row.get('fmax_mhz', '')}MHz "
        f"wns={row.get('worst_slack_ns', '')}ns "
        f"peak={row.get('vpr_peak_mem_mb', '')}MB "
        f"rc={row.get('return_code', '')}"
    )


# ---------------------------------------------------------------------------
# one run
# ---------------------------------------------------------------------------


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

    startStage = flows[flowName]["start"]
    cmd = [
        sys.executable,
        str(runVtrFlow.resolve()),
        str(circuitPath.resolve()),
        str(archFile.resolve()),
        "-start",
        startStage,
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


# ---------------------------------------------------------------------------
# csv / orchestration
# ---------------------------------------------------------------------------


def writeCsv(csvPath: Path, rows: List[Dict]) -> None:
    merged: Dict[Tuple[str, str], Dict] = {}
    if csvPath.is_file():
        with open(csvPath, newline="", encoding="utf-8") as existing:
            for oldRow in csv.DictReader(existing):
                key = (oldRow.get("design", ""), oldRow.get("flow", ""))
                merged[key] = dict(oldRow)
    for row in rows:
        key = (str(row.get("design", "")), str(row.get("flow", "")))
        merged[key] = {field: row.get(field, "") for field in csvFields}

    csvPath.parent.mkdir(parents=True, exist_ok=True)
    with open(csvPath, "w", newline="", encoding="utf-8") as outFile:
        writer = csv.DictWriter(outFile, fieldnames=csvFields)
        writer.writeheader()
        for key in sorted(merged):
            writer.writerow(merged[key])


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
    parser.add_argument(
        "--arch",
        default=defaultArchRel,
        help=f"architecture xml (default {defaultArchRel})",
    )
    parser.add_argument(
        "--benchmark-dir",
        default=defaultBenchRel,
        help=f"directory holding <design>.v files (default {defaultBenchRel})",
    )
    parser.add_argument(
        "--designs",
        nargs="+",
        default=None,
        help=f"circuit stems (default: {' '.join(defaultDesigns)})",
    )
    parser.add_argument(
        "--flows",
        nargs="+",
        default=None,
        help="vanilla_vtr and/or frankenstein (aliases: vtr, frank)",
    )
    parser.add_argument("--jobs", type=int, default=4, help="parallel jobs for small circuits")
    parser.add_argument(
        "--large-jobs",
        type=int,
        default=None,
        help="parallel jobs for large circuits (default --jobs/2)",
    )
    parser.add_argument("--serial", action="store_true", help="force --jobs 1 --large-jobs 1")
    parser.add_argument(
        "--outdir",
        default=None,
        help="output directory (default compare_output_<arch_stem>)",
    )
    parser.add_argument(
        "--csv",
        default=None,
        help="results csv path (default <outdir>/compare_results.csv)",
    )
    parser.add_argument("--no-clean", action="store_true", help="keep existing run dirs")
    parser.add_argument(
        "--no-rerun", action="store_true", help="skip runs with a .success marker"
    )
    args = parser.parse_args(argv)

    archFile = resolvePath(args.arch, vtrRoot)
    benchDir = resolvePath(args.benchmark_dir, vtrRoot)
    designs = normalizeDesigns(args.designs)
    selectedFlows = normalizeFlows(args.flows)

    jobs = max(1, args.jobs)
    largeJobs = args.large_jobs if args.large_jobs else max(1, jobs // 2)
    if args.serial:
        jobs = 1
        largeJobs = 1

    defaultOutDirName = f"compare_output_{archFile.stem}"
    outDir = resolvePath(args.outdir or defaultOutDirName, vtrRoot)
    csvPath = resolvePath(args.csv, vtrRoot) if args.csv else (outDir / "compare_results.csv")

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
    print(f"jobs:    small={jobs} large={largeJobs}")
    print(f"outdir:  {outDir}")
    print(f"csv:     {csvPath}")
    print(f"watch:   python3 frankenstein/scripts/watch_compare.py --dir {outDir}")
    print()

    largeTasks = [t for t in tasks if t[0] in largeDesigns]
    smallTasks = [t for t in tasks if t[0] not in largeDesigns]

    rows: List[Dict] = []
    if largeTasks:
        nDesigns = len({t[0] for t in largeTasks})
        nFlows = len({t[1] for t in largeTasks})
        print(f"large circuits ({nDesigns} designs x {nFlows} flows = {len(largeTasks)} runs) @ {largeJobs} jobs")
        rows.extend(runPool(largeTasks, largeJobs))
    if smallTasks:
        nDesigns = len({t[0] for t in smallTasks})
        nFlows = len({t[1] for t in smallTasks})
        print(f"small circuits ({nDesigns} designs x {nFlows} flows = {len(smallTasks)} runs) @ {jobs} jobs")
        rows.extend(runPool(smallTasks, jobs))

    writeCsv(csvPath, rows)

    ok = sum(1 for row in rows if row.get("success"))
    fail = len(rows) - ok
    print()
    print(f"done: {ok} ok, {fail} failed  ->  {csvPath}")
    return 0 if fail == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
