#!/usr/bin/env python3
"""
live status table for frankenstein/scripts/compare_flow.py.

scans <outdir>/status/*.txt and live phase hints from run dirs. run in a
second terminal while the compare is going.

usage:
  python3 frankenstein/scripts/watch_compare.py
  python3 frankenstein/scripts/watch_compare.py --dir compare_output_<arch_stem>
  python3 frankenstein/scripts/watch_compare.py --interval 2
  python3 frankenstein/scripts/watch_compare.py --once

flags:
  --dir <outdir>     compare output directory (default: newest compare_output* by mtime)
  --interval <sec>   refresh period (default 1.0)
  --once             print once and exit
"""

from __future__ import annotations

import argparse
import math
import re
import sys
import time
from pathlib import Path

try:
    from prettytable import PrettyTable
except ImportError:
    print("prettytable required: pip install prettytable", file=sys.stderr)
    raise SystemExit(1)

scriptDir = Path(__file__).resolve().parent
vtrRoot = scriptDir.parents[1]

# status-line keys written by compare_flow.formatSummary
statusFields = (
    "wall",
    "s_s",
    "a_s",
    "v_s",
    "synthesis",
    "s_luts",
    "a_luts",
    "p_luts",
    "s_ff",
    "ff",
    "mem",
    "dsp",
    "adder",
    "io_in",
    "io_out",
    "clb",
    "wl",
    "cpd",
    "fmax",
    "wns",
)

# live-run phase hints (newest matching log line wins). labels are what the
# status column shows.
#
# vpr prints these via vtr::ScopedStartFinishTimer:
#   start:  "Packing" / "SA Placement" / "Routing"
#   finish: "Packing took …" / "SA Placement took …" / "Routing took …"
# packing also logs: Begin packing '…'.
phasePatterns = [
    (re.compile(r"(?:^|#\s*)Routing took\b", re.IGNORECASE), "route done"),
    (re.compile(r"(?:^|#\s*)SA Placement took\b|(?:^|#\s*)Placement took\b", re.IGNORECASE), "place done"),
    (re.compile(r"(?:^|#\s*)Packing took\b", re.IGNORECASE), "pack done"),
    (re.compile(r"(?:^|#\s*)Routing\s*$", re.IGNORECASE), "routing"),
    (re.compile(r"(?:^|#\s*)SA Placement\s*$|(?:^|#\s*)Placement\s*$", re.IGNORECASE), "placing"),
    (re.compile(r"(?:^|#\s*)Packing\s*$|Begin packing\b", re.IGNORECASE), "packing"),
    (re.compile(r"Executing ABC|abc -luts", re.IGNORECASE), "abc"),
    (re.compile(r"frankenstein|vtr_arch_rules|parmys|Executing PARMYS|Yosys [0-9]|Executing.*yosys", re.IGNORECASE), "synth"),
]

# display text -> ansi color for live phases
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
# frontend = raw front-end stage log only (parmys.out / frankenstein.out)
# synthesis = fair compare: frankenstein synth; vanilla synth+abc
# column order matches the per-run table (IO in/out, then CLBs, then FFs)
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
    ("CPD", "cpd", False),
    ("Fmax", "fmax", True),
)


def inferPhase(lines):
    # return None when nothing matches so the caller can keep the last known phase
    for line in reversed(lines[-200:]):
        for pattern, label in phasePatterns:
            if pattern.search(line):
                return label
    return None


def livePhaseFromRunDir(runDir: Path):
    if not runDir.is_dir():
        return None
    candidates = [
        runDir / name
        for name in (
            "frankenstein.out",
            "parmys.out",
            "vpr.out",
            "abc.out",
            "abc0.out",
            "nohup.out",
        )
        if (runDir / name).is_file()
    ]
    if not candidates:
        return "started"
    newest = max(candidates, key=lambda path: path.stat().st_mtime)
    try:
        lines = newest.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        return None
    return inferPhase(lines)


def parseStatusLine(label: str, line: str):
    result = {"label": label, "raw": line}
    statusMatch = re.match(r"\S+:\s+(\S+)", line)
    if statusMatch:
        result["status"] = statusMatch.group(1)
        if result["status"].startswith("FAIL"):
            failMatch = re.match(r"\S+:\s+(FAIL(?:\s*\([^)]*\))?)", line)
            if failMatch:
                result["status"] = failMatch.group(1)
    for key in statusFields:
        match = re.search(rf"\b{re.escape(key)}=([0-9.eE+-]+)", line)
        if match:
            result[key] = match.group(1)
    return result


def scanStatus(targetDir: Path, lastPhases=None):
    # lastPhases remembers the last known live phase per label so an unknown
    # log scrape does not snap the status back to a generic fallback
    if lastPhases is None:
        lastPhases = {}
    statusDir = targetDir / "status"
    runsDir = targetDir / "runs"
    manifest = statusDir / "manifest.txt"

    if manifest.is_file():
        labels = [
            line.strip()
            for line in manifest.read_text(encoding="utf-8").splitlines()
            if line.strip()
        ]
    elif runsDir.is_dir():
        labels = sorted(path.name for path in runsDir.iterdir() if path.is_dir())
    else:
        return []

    rows = []
    for label in labels:
        statusFile = statusDir / f"{label}.txt"
        if statusFile.is_file():
            line = statusFile.read_text(encoding="utf-8", errors="replace").strip()
            rows.append(enrichSynthesis(parseStatusLine(label, line)))
            lastPhases.pop(label, None)
            continue
        runDir = runsDir / label
        if runDir.is_dir():
            phase = livePhaseFromRunDir(runDir)
            if phase is None:
                phase = lastPhases.get(label, "started")
            else:
                lastPhases[label] = phase
            rows.append({"label": label, "status": f"running:{phase}"})
        else:
            rows.append({"label": label, "status": "pending"})
            lastPhases.pop(label, None)
    return rows


def sap(row, *keys):
    return "/".join(str(row.get(k) or "-") for k in keys)


def enrichSynthesis(row):
    # fair synthesis time if missing from older status lines:
    # frankenstein = frontend only (includes in-yosys abc);
    # vanilla_vtr = frontend + abc stage
    if row.get("synthesis"):
        return row
    flow = flowOf(row.get("label", ""))
    s = numeric(row.get("s_s"))
    a = numeric(row.get("a_s"))
    if flow == "frankenstein" and s is not None:
        row["synthesis"] = f"{s:.2f}"
    elif flow == "vanilla_vtr" and (s is not None or a is not None):
        row["synthesis"] = f"{(s or 0.0) + (a or 0.0):.2f}"
    return row


def coloredStatus(row):
    # finished / queued
    #   ok green, fail red, pending grey, cached cyan
    # live phases (no "running (…)" wrapper — the phase is the status)
    #   synth blue, abc cyan, pack/place/route yellow
    status = row.get("status", "")
    if status == "ok":
        text, color = "ok", "\033[92m"
    elif status.startswith("FAIL") or status == "fail":
        text, color = status, "\033[91m"
    elif status == "pending":
        text, color = "pending", "\033[90m"
    elif status == "cached":
        text, color = "cached", "\033[96m"
    elif status.startswith("running:"):
        phase = status.split(":", 1)[1].strip() or "started"
        if phase in ("running", "in progress", "starting", ""):
            phase = "started"
        text = phase
        color = phaseColors.get(phase, "\033[93m")
    elif status == "running":
        text, color = "started", "\033[93m"
    else:
        # unknown token still gets a color so nothing is plain white
        text, color = status or "started", "\033[93m"
    return f"{color}{text}\033[0m"


def renderTable(rows, title):
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
        "CPD",
        "Fmax",
        "WNS",
    ]
    table.align = "l"
    for row in rows:
        table.add_row(
            [
                row.get("label", ""),
                coloredStatus(row),
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


def numeric(value):
    try:
        number = float(value)
    except (TypeError, ValueError):
        return None
    return number if number > 0 else None


def flowOf(label):
    for flow in ("frankenstein", "vanilla_vtr"):
        if label.endswith("_" + flow):
            return flow
    return None


def computeGeomeans(rows):
    byDesign = {}
    flowsSeen = []
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


def renderGeomeanTable(rows):
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
        table.add_row([flow] + [fmt(geo[flow].get(key)) for _, key, _ in geomeanColumns])
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


def resolveCsvPath(targetDir: Path) -> Path | None:
    # prefer the path written by the active compare_flow run
    marker = targetDir / "status" / "csv_path.txt"
    if marker.is_file():
        text = marker.read_text(encoding="utf-8", errors="replace").strip()
        if text:
            path = Path(text)
            if path.is_file() or path.parent.is_dir():
                return path
    candidates = sorted(targetDir.glob("compare_results*.csv"))
    return candidates[-1] if candidates else None


def watchDir(targetDir: Path, interval: float, once: bool):
    logsDir = targetDir / "logs"
    lastPhases = {}
    print(f"watching {targetDir} (interval={interval}s) — Ctrl-C to stop")

    try:
        while True:
            rows = scanStatus(targetDir, lastPhases)
            total = len(rows)
            done = sum(
                1
                for row in rows
                if row.get("status")
                and not row["status"].startswith("running")
                and row["status"] != "pending"
            )
            running = sum(
                1 for row in rows if row.get("status", "").startswith("running")
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


def findOutputDirs():
    """compare_output* dirs, newest modification time last."""
    dirs = [
        path
        for path in vtrRoot.iterdir()
        if path.is_dir() and path.name.startswith("compare_output")
    ]
    dirs.sort(key=lambda path: path.stat().st_mtime)
    return dirs


def main(argv=None):
    parser = argparse.ArgumentParser(description="live status table for compare_flow")
    parser.add_argument(
        "--dir",
        default=None,
        help="compare output directory (default: newest compare_output* by mtime)",
    )
    parser.add_argument("--interval", type=float, default=1.0, help="refresh seconds")
    parser.add_argument("--once", action="store_true", help="print once and exit")
    args = parser.parse_args(argv)

    if args.dir:
        targetDir = Path(args.dir)
        if not targetDir.is_absolute():
            targetDir = vtrRoot / targetDir
    else:
        candidates = findOutputDirs()
        if not candidates:
            print("no compare_output* directories found", file=sys.stderr)
            sys.exit(1)
        targetDir = candidates[-1]
        print(f"auto-selected: {targetDir.name}")

    watchDir(targetDir, args.interval, args.once)


if __name__ == "__main__":
    main()
