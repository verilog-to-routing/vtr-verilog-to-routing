#!/usr/bin/env python3
"""live status table for mosaic/scripts/run_vtr_batch.py.

scans <outdir>/status/*.txt and live phase hints from run dirs. run in a
second terminal while the batch is going.

usage:
    python3 mosaic/scripts/watch_compare.py
    python3 mosaic/scripts/watch_compare.py \
        --dir mosaic/scripts/compare_output_<arch_stem>
    python3 mosaic/scripts/watch_compare.py --interval 2
    python3 mosaic/scripts/watch_compare.py --once

flags:
    --dir <outdir>     compare output directory (default: newest compare_output*
                            under mosaic/scripts then repo root, by mtime)
    --interval <sec>   refresh period (default 1.0)
    --once             print once and exit
"""

from __future__ import annotations

import argparse
import math
import os
import re
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

phaseColors = {
    "started": "\033[93m",
    "synth": "\033[94m",
    "abc": "\033[96m",
    "packing": "\033[93m",
    "pack done": "\033[93m",
    "placing": "\033[93m",
    "place done": "\033[93m",
    "routing": "\033[93m",
    "route done": "\033[93m",
}

# (header, status key, higher_is_better)
# frontend is raw front-end stage only (parmys.out / mosaic.out).
# synthesis is the fair compare column: mosaic synth alone, vanilla synth+abc.
geomeanColumns = (
    ("frontend", "s_s", False),
    ("synthesis", "synthesis", False),
    ("vpr", "v_s", False),
    ("wall", "wall", False),
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


# USE: parse one status line into label, status, and key=value fields.
def parseStatusLine(text: str) -> Dict[str, str]:
    # "label: status key=val ...". status may be "FAIL (reason with spaces)"
    row: Dict[str, str] = {}
    line = text.strip()
    if not line:
        return row
    match = re.match(r"^([^:]+):\s*(.*)$", line)
    if not match:
        row["label"] = line.split()[0]
        return row
    row["label"] = match.group(1).strip()
    rest = match.group(2).strip()
    if not rest:
        return row
    tokens = rest.split()
    kvStart = len(tokens)
    for i in range(len(tokens) - 1, -1, -1):
        if "=" in tokens[i]:
            kvStart = i
        else:
            break
    row["status"] = " ".join(tokens[:kvStart]).strip() or "ok"
    for tok in tokens[kvStart:]:
        if "=" not in tok:
            continue
        key, value = tok.split("=", 1)
        row[key] = stripUnit(value)
    return row


# HELPER: accept bare numbers or legacy status values like 12.3ns / 100MHz / 4.5s.
def stripUnit(value: str) -> str:
    text = str(value).strip()
    match = re.match(
        r"^([+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?)\s*"
        r"(ns|mhz|s|sec|seconds?)?$",
        text,
        re.I,
    )
    if match:
        return match.group(1)
    return text


# USE: infer the current vtr phase from the newest log tails in a run dir.
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


def readLabels(outDir: Path) -> List[str]:
    manifest = outDir / "status" / "manifest.txt"
    if manifest.is_file():
        try:
            text = manifest.read_text(encoding="utf-8", errors="replace")
        except OSError:
            text = ""
        labels = [line.strip() for line in text.splitlines() if line.strip()]
        if labels:
            return labels
    statusDir = outDir / "status"
    if statusDir.is_dir():
        return sorted(path.stem for path in statusDir.glob("*.txt") if path.stem != "csv_path")
    runsDir = outDir / "runs"
    if runsDir.is_dir():
        return sorted(path.name for path in runsDir.iterdir() if path.is_dir())
    return []


def scanStatus(outDir: Path, labels: Sequence[str]) -> List[Dict[str, str]]:
    rows = []
    for label in labels:
        statusPath = outDir / "status" / f"{label}.txt"
        runDir = outDir / "runs" / label
        if statusPath.is_file():
            try:
                text = statusPath.read_text(encoding="utf-8", errors="replace")
            except OSError:
                text = ""
            row = parseStatusLine(text.splitlines()[0] if text else "")
            if not row.get("label"):
                row["label"] = label
            rows.append(enrichSynthesis(row))
        elif runDir.is_dir():
            rows.append({"label": label, "status": f"running:{detectPhase(runDir)}"})
        else:
            rows.append({"label": label, "status": "pending"})
    return rows


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


# USE: fill fair synthesis time when older status lines omit it.
# mosaic uses frontend only, while vanilla_vtr uses frontend plus abc.
def enrichSynthesis(row: Dict[str, str]) -> Dict[str, str]:
    if row.get("synthesis"):
        return row
    flow = flowOf(row.get("label", ""))
    synthSec = numeric(row.get("s_s"))
    abcSec = numeric(row.get("a_s"))
    if flow == "mosaic" and synthSec is not None:
        row["synthesis"] = f"{synthSec:.2f}"
    elif flow == "vanilla_vtr" and (synthSec is not None or abcSec is not None):
        row["synthesis"] = f"{(synthSec or 0.0) + (abcSec or 0.0):.2f}"
    return row


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
        and all(flowRows[f].get("status") in ("ok", "cached") for f in flowsSeen)
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
    if status == "ok":
        return "\033[92mok\033[0m"
    if status.startswith("FAIL") or status == "fail":
        return f"\033[91m{status}\033[0m"
    if status == "pending":
        return "\033[90mpending\033[0m"
    if status == "cached":
        return "\033[96mcached\033[0m"
    if status.startswith("running:"):
        phase = status.split(":", 1)[1] or "started"
        color = phaseColors.get(phase, "\033[93m")
        return f"{color}{phase}\033[0m"
    return f"\033[93m{status or 'started'}\033[0m"


def sap(row: Dict[str, str], *keys: str) -> str:
    return "/".join(str(row.get(key) or "-") for key in keys)


def renderTable(rows: List[Dict[str, str]], title: str) -> str:
    table = PrettyTable()
    table.field_names = [
        "run",
        "status",
        "wall",
        "synthesis",
        "time s/a/v",
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
    table.align = "l"
    for row in rows:
        table.add_row(
            [
                row.get("label", ""),
                coloredStatus(row.get("status", "")),
                row.get("wall", "-"),
                row.get("synthesis", "-"),
                sap(row, "s_s", "a_s", "v_s"),
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
    return f"{title}\n{table}"


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


def findOutputDirs() -> List[Path]:
    roots = [compareOutputRoot]
    if compareOutputRoot != vtrRoot:
        roots.append(vtrRoot)
    dirs = []
    for root in roots:
        dirs.extend(
            path
            for path in root.iterdir()
            if path.is_dir() and path.name.startswith("compare_output")
        )
    dirs.sort(key=lambda path: path.stat().st_mtime)
    return dirs


# USE: refresh the live status table until interrupted or --once.
def watchDir(targetDir: Path, interval: float, once: bool) -> None:
    logsDir = targetDir / "logs"
    print(f"watching {targetDir} (interval={interval}s), Ctrl-C to stop")
    try:
        while True:
            labels = readLabels(targetDir)
            rows = scanStatus(targetDir, labels)
            total = len(rows)
            done = sum(
                1
                for row in rows
                if row.get("status")
                and not str(row["status"]).startswith("running")
                and row["status"] != "pending"
            )
            running = sum(
                1 for row in rows if str(row.get("status", "")).startswith("running")
            )
            pending = sum(1 for row in rows if row.get("status") == "pending")
            title = (
                f"{targetDir.name}: {done}/{total} done, "
                f"{running} running, {pending} pending"
            )
            if not once:
                print("\033[H\033[J", end="")
            print(renderTable(rows, title))
            print()
            print(renderGeomeanTable(rows))
            print(f"\nlogs: {logsDir}")
            csvPath = resolveCsvPath(targetDir)
            if csvPath is not None:
                print(f"csv:  {csvPath}")
            if once:
                break
            time.sleep(interval)
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
