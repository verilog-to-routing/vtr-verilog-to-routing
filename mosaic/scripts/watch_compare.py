#!/usr/bin/env python3
# live status table for mosaic/scripts/run_vtr_batch.py.
#
# reads the results .csv file written by run_vtr_batch.py.
#
# usage:
#     python3 mosaic/scripts/watch_compare.py
#     python3 mosaic/scripts/watch_compare.py \
#         --dir mosaic/scripts/compare_output_<arch_stem>
#     python3 mosaic/scripts/watch_compare.py --interval 2
#     python3 mosaic/scripts/watch_compare.py --once
#
# flags:
#     --dir <outdir>     compare output directory (default: newest compare_output*
#                             under mosaic/scripts then repo root, by mtime)
#     --interval <sec>   refresh period (default 1.0)
#     --once             print once and exit
#
# when --verilator-check <flows...> is set the table adds vcheck match mismatch vectors perrors
# vcheck is pass, fail (mismatches), or error (could not run)

from __future__ import annotations

import argparse
import csv
import math
import sys
import time
from pathlib import Path
from typing import Dict, List, Optional, Sequence

try:
    from prettytable import PrettyTable
except ImportError:
    print("prettytable required: pip install prettytable", file=sys.stderr)
    raise SystemExit(1)

scriptDir = Path(__file__).resolve().parent
vtrRoot = scriptDir.parents[1]
# new batches default under mosaic/scripts, and the repo root remains a fallback for older runs.
compareOutputRoot = scriptDir

phaseColors = {
    "pending": "\033[90m",
    "started": "\033[93m",
    "synth": "\033[94m",
    "abc": "\033[96m",
    "packing": "\033[93m",
    "pack done": "\033[93m",
    "placing": "\033[93m",
    "place done": "\033[93m",
    "routing": "\033[93m",
    "route done": "\033[92m",
}

spinnerFrames = ("-", "\\", "|", "/")

# csv field -> table/geomean short key
csvDisplayKeys = (
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

# (header, status key, higher_is_better)
geomeanColumns = (
    ("wall", "wall", False),
    ("synth", "synth", False),
    ("vpr", "vpr", False),
    ("LUTs", "p_luts", False),
    ("FFs", "ff", False),
    ("BRAMs", "mem", False),
    ("DSPs", "dsp", False),
    ("Adders", "adder", False),
    ("IO in", "io_in", False),
    ("IO out", "io_out", False),
    ("CLBs", "clb", False),
    ("Wirelen", "wl", False),
    ("CPD (ns)", "cpd", False),
    ("Fmax (MHz)", "fmax", True),
)


# HELPER: accept bare numbers or legacy status values like 12.3ns / 100MHz / 4.5s.
def stripUnit(value: str) -> str:
    text = str(value).strip()
    if not text:
        return ""
    # keep plain numbers; strip common unit suffixes if present
    for suffix in ("ns", "MHz", "mhz", "s", "sec", "seconds", "Seconds"):
        if text.lower().endswith(suffix.lower()) and len(text) > len(suffix):
            trimmed = text[: -len(suffix)].strip()
            try:
                float(trimmed)
                return trimmed
            except ValueError:
                break
    return text


def resolveCsvPath(targetDir: Path) -> Path | None:
    marker = targetDir / "status" / "csv_path.txt"
    if marker.is_file():
        text = marker.read_text(encoding="utf-8", errors="replace").strip()
        if text:
            path = Path(text)
            if path.is_file() or path.parent.is_dir():
                return path
    candidates = sorted(targetDir.glob("compare_results*.csv"))
    return candidates[-1] if candidates else None


# USE: map one results-csv row into the short keys used by the tables.
def rowFromCsv(raw: Dict[str, str]) -> Dict[str, str]:
    design = (raw.get("design") or "").strip()
    flow = (raw.get("flow") or "").strip()
    row: Dict[str, str] = {
        "label": f"{design}_{flow}" if design and flow else design or flow,
        "status": (raw.get("status") or "pending").strip() or "pending",
    }
    for shortKey, field in csvDisplayKeys:
        value = raw.get(field, "")
        if value == "" or value is None:
            continue
        row[shortKey] = stripUnit(value)
    return row


def loadRows(csvPath: Path) -> List[Dict[str, str]]:
    if not csvPath.is_file():
        return []
    try:
        with open(csvPath, newline="", encoding="utf-8") as handle:
            return [rowFromCsv(dict(raw)) for raw in csv.DictReader(handle)]
    except OSError:
        return []


def numeric(value) -> Optional[float]:
    if value is None:
        return None
    try:
        number = float(stripUnit(value))
    except (TypeError, ValueError):
        return None
    return number if number > 0 else None


def flowOf(label: str) -> Optional[str]:
    for flow in ("mosaic", "vanilla_vtr"):
        if label.endswith("_" + flow):
            return flow
    return None


# USE: compute per-flow geomeans over designs that completed on every flow.
def computeGeomeans(rows: Sequence[Dict[str, str]]):
    byDesign: Dict[str, Dict[str, Dict[str, str]]] = {}
    flowsSeen: List[str] = []
    for row in rows:
        flow = flowOf(row.get("label", ""))
        if flow is None:
            continue
        if flow not in flowsSeen:
            flowsSeen.append(flow)
        design = row["label"][: -(len(flow) + 1)]
        byDesign.setdefault(design, {})[flow] = row

    if len(flowsSeen) < 2:
        return None

    paired = {
        design: flowRows
        for design, flowRows in byDesign.items()
        if len(flowRows) == len(flowsSeen)
        and all(isGeomeanReady(flowRows[f]) for f in flowsSeen)
    }
    if not paired:
        return None

    result = {}
    for flow in flowsSeen:
        result[flow] = {}
        for _, key, _ in geomeanColumns:
            values = [
                numeric(flowRows[flow].get(key)) for flowRows in paired.values()
            ]
            values = [v for v in values if v is not None]
            if values:
                result[flow][key] = math.exp(
                    sum(math.log(v) for v in values) / len(values)
                )
            else:
                result[flow][key] = None
    return {"flows": flowsSeen, "geo": result}


# USE: render the geomean table with percent and ratio diffs vs vanilla_vtr.
def renderGeomeanTable(rows: Sequence[Dict[str, str]]) -> str:
    data = computeGeomeans(rows)
    if data is None:
        return "geomean: waiting for paired ok/cached runs on both flows"

    flows = data["flows"]
    base = "vanilla_vtr" if "vanilla_vtr" in flows else flows[0]
    others = [f for f in flows if f != base]
    geo = data["geo"]

    def fmt(value):
        return f"{value:,.2f}" if value is not None else "-"

    table = PrettyTable()
    table.field_names = ["flow"] + [header for header, _, _ in geomeanColumns]
    table.align = "l"
    table.add_row(
        [base] + [fmt(geo[base].get(key)) for _, key, _ in geomeanColumns]
    )
    for flow in others:
        table.add_row(
            [flow] + [fmt(geo[flow].get(key)) for _, key, _ in geomeanColumns]
        )
        diffRow = [f"% diff {flow}/{base}"]
        ratioRow = [f"x diff {flow}/{base}"]
        for _, key, _ in geomeanColumns:
            fVal, bVal = geo[flow].get(key), geo[base].get(key)
            if fVal is None or bVal is None:
                diffRow.append("-")
                ratioRow.append("-")
            else:
                diffRow.append(f"{(fVal / bVal - 1.0) * 100.0:+.2f}%")
                ratioRow.append(f"{fVal / bVal:.2f}x")
        table.add_row(diffRow)
        table.add_row(ratioRow)
    return f"geomean:\n{table}"


def coloredStatus(status: str) -> str:
    if status == "fail":
        return "\033[91mfail\033[0m"
    if status == "pending":
        return "\033[90mpending\033[0m"
    if status == "cached":
        return "\033[96mcached\033[0m"
    color = phaseColors.get(status, "\033[93m")
    return f"{color}{status or 'started'}\033[0m"


def coloredVcheck(value: str) -> str:
    text = (value or "-").strip() or "-"
    if text == "pass":
        return f"\033[92m{text}\033[0m"
    if text == "fail":
        return f"\033[91m{text}\033[0m"
    if text == "error":
        return f"\033[93m{text}\033[0m"
    return text


def sap(row: Dict[str, str], *keys: str) -> str:
    return "/".join(str(row.get(key) or "-") for key in keys)


# USE: show vcheck when batch wrote the flag marker or any row has a value.
def showVerilatorColumn(targetDir: Path, rows: Sequence[Dict[str, str]]) -> bool:
    marker = targetDir / "status" / "verilator_check"
    if marker.is_file():
        return True
    return any(
        (row.get(key) or "").strip()
        for row in rows
        for key in ("vcheck", "match", "mismatch", "vectors", "perrors")
    )


def renderTable(
    rows: List[Dict[str, str]],
    title: str,
    showVcheck: bool = False,
) -> str:
    table = PrettyTable()
    fieldNames = [
        "run",
        "status",
    ]
    if showVcheck:
        fieldNames.extend(["vcheck", "match", "mismatch", "vectors", "perrors"])
    fieldNames.extend(
        [
            "wall",
            "synth",
            "vpr",
            "LUTs s/a/p",
            "FFs s/p",
            "BRAMs",
            "DSPs",
            "Adders",
            "IO in",
            "IO out",
            "CLBs",
            "Wirelen",
            "CPD (ns)",
            "Fmax (MHz)",
            "WNS (ns)",
        ]
    )
    table.field_names = fieldNames
    table.align = "l"
    for row in rows:
        values = [
            row.get("label", ""),
            coloredStatus(row.get("status", "")),
        ]
        if showVcheck:
            values.extend(
                [
                    coloredVcheck(row.get("vcheck", "")),
                    row.get("match", "-") or "-",
                    row.get("mismatch", "-") or "-",
                    row.get("vectors", "-") or "-",
                    row.get("perrors", "-") or "-",
                ]
            )
        values.extend(
            [
                row.get("wall", "-"),
                row.get("synth", "-"),
                row.get("vpr", "-"),
                sap(row, "s_luts", "a_luts", "p_luts"),
                sap(row, "s_ff", "ff"),
                row.get("mem", "-"),
                row.get("dsp", "-"),
                row.get("adder", "-"),
                row.get("io_in", "-"),
                row.get("io_out", "-"),
                row.get("clb", "-"),
                row.get("wl", "-"),
                row.get("cpd", "-"),
                row.get("fmax", "-"),
                row.get("wns", "-"),
            ]
        )
        table.add_row(values)
    return f"{title}\n{table}"


def findOutputDirs() -> List[Path]:
    roots = [compareOutputRoot]
    if compareOutputRoot != vtrRoot:
        roots.append(vtrRoot)
    dirs = []
    for root in roots:
        if not root.is_dir():
            continue
        dirs.extend(
            path
            for path in root.iterdir()
            if path.is_dir() and path.name.startswith("compare_output")
        )
    dirs.sort(key=lambda path: path.stat().st_mtime)
    return dirs


def isDoneStatus(status: str) -> bool:
    return status in ("route done", "ok", "cached", "fail")


# USE: true when a row can enter the live geomean.
def isGeomeanReady(row: Dict[str, str]) -> bool:
    status = row.get("status", "")
    if status == "fail":
        return False
    if status not in ("route done", "ok", "cached"):
        return False
    return status == "cached" or numeric(row.get("wall")) is not None


# USE: refresh the live status table until interrupted or --once.
def watchDir(targetDir: Path, interval: float, once: bool) -> None:
    logsDir = targetDir / "logs"
    spinnerPeriod = 0.25
    print(f"watching {targetDir} (interval={interval}s), Ctrl-C to stop")
    frame = 0
    lastCsvLoad = 0.0
    csvPath: Path | None = None
    rows: List[Dict[str, str]] = []
    try:
        while True:
            now = time.monotonic()
            if once or csvPath is None or (now - lastCsvLoad) >= interval:
                csvPath = resolveCsvPath(targetDir)
                rows = loadRows(csvPath) if csvPath is not None else []
                lastCsvLoad = now
            total = len(rows)
            done = sum(1 for row in rows if isDoneStatus(row.get("status", "")))
            pending = sum(1 for row in rows if row.get("status") == "pending")
            running = total - done - pending
            spinner = spinnerFrames[frame % len(spinnerFrames)] if running else ""
            title = (
                f"{targetDir.name}: {done}/{total} done, "
                f"{running} running, {pending} pending"
            )
            if spinner:
                title = f"{title} {spinner}"
            if not once:
                print("\033[H\033[J", end="")
            if csvPath is None:
                print(f"{title}\n(no results csv yet)")
            else:
                showVcheck = showVerilatorColumn(targetDir, rows)
                print(renderTable(rows, title, showVcheck=showVcheck))
                print()
                print(renderGeomeanTable(rows))
            print(f"\nlogs: {logsDir}")
            if csvPath is not None:
                print(f"csv:  {csvPath}")
            if once:
                break
            frame += 1
            time.sleep(spinnerPeriod)
    except KeyboardInterrupt:
        print("\nstopped.")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description="live status table for run_vtr_batch")
    parser.add_argument(
        "--dir",
        default=None,
        help="compare output directory (default: newest compare_output* under "
        "mosaic/scripts then repo root, by mtime)",
    )
    parser.add_argument("--interval", type=float, default=1.0, help="refresh seconds")
    parser.add_argument("--once", action="store_true", help="print once and exit")
    args = parser.parse_args(argv)

    if args.dir:
        targetDir = Path(args.dir)
        if not targetDir.is_absolute():
            candidate = compareOutputRoot / targetDir
            targetDir = candidate if candidate.is_dir() else (vtrRoot / targetDir)
    else:
        candidates = findOutputDirs()
        if not candidates:
            print("no compare_output* directories found", file=sys.stderr)
            return 1
        targetDir = candidates[-1]
        print(f"auto-selected: {targetDir.name}")

    if not targetDir.is_dir():
        print(f"missing outdir: {targetDir}", file=sys.stderr)
        return 1

    watchDir(targetDir, max(0.2, args.interval), args.once)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
