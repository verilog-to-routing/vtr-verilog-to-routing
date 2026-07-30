#!/usr/bin/env python3
"""
live status table for frankenstein/scripts/compare_flow.py.

scans <outdir>/status/*.txt and live phase hints from run dirs. run in a
second terminal while the compare is going.

usage:
  python3 frankenstein/scripts/watch_compare.py
  python3 frankenstein/scripts/watch_compare.py --dir compare_output_k6_frac_N10_frac_chain_mem32K_40nm
  python3 frankenstein/scripts/watch_compare.py --interval 2
  python3 frankenstein/scripts/watch_compare.py --once
"""

from __future__ import annotations

import argparse
import math
import re
import sys
import time
from pathlib import Path

scriptDir = Path(__file__).resolve().parent
vtrRoot = scriptDir.parents[1]
defaultOutDirName = "compare_output_k6_frac_N10_frac_chain_mem32K_40nm"

fmaxRe = re.compile(r"fmax=([0-9.]+)MHz")
clbRe = re.compile(r"clb=(\d+)")
wallRe = re.compile(r"wall=([0-9.]+)s")
wnsRe = re.compile(r"wns=([0-9.-]+)ns")
synthWallRe = re.compile(r"s_s=([0-9.]+)")
abcWallRe = re.compile(r"a_s=([0-9.]+)")
vprWallRe = re.compile(r"v_s=([0-9.]+)")
synthLutsRe = re.compile(r"s_luts=(\d+)")
abcLutsRe = re.compile(r"a_luts=(\d+)")
packedLutsRe = re.compile(r"(?:p_luts|packed_luts)=(\d+)")
ffRe = re.compile(r"\bff=(\d+)")
memRe = re.compile(r"(?:packed_brams|mem)=(\d+)")
dspRe = re.compile(r"(?:packed_dsps|dsp)=(\d+)")
adderRe = re.compile(r"(?:packed_adders|adder)=(\d+)")
wlRe = re.compile(r"wl=(\d+)")
cpdRe = re.compile(r"cpd=([0-9.]+)ns")

phasePatterns = [
    (re.compile(r"# Routing took|routing took", re.IGNORECASE), "route:done"),
    (re.compile(r"# Placement took|placement took", re.IGNORECASE), "place:done"),
    (re.compile(r"# Packing took|packing took", re.IGNORECASE), "pack:done"),
    (re.compile(r"Routing|Begin routing|Route:", re.IGNORECASE), "routing"),
    (re.compile(r"Placement|Begin placement|Place:", re.IGNORECASE), "placing"),
    (re.compile(r"Packing|Begin packing|Pack:", re.IGNORECASE), "packing"),
    (re.compile(r"Executing ABC|abc -luts", re.IGNORECASE), "abc"),
    (re.compile(r"frankenstein|vtr_arch_rules", re.IGNORECASE), "synth:frankenstein"),
    (re.compile(r"parmys|Executing PARMYS", re.IGNORECASE), "synth:parmys"),
    (re.compile(r"Yosys [0-9]|Executing.*yosys", re.IGNORECASE), "synth:yosys"),
]


def inferPhase(lines):
    for line in reversed(lines[-200:]):
        for pattern, label in phasePatterns:
            if pattern.search(line):
                return label
    return "running"


def livePhaseFromRunDir(runDir: Path):
    if not runDir.is_dir():
        return None
    candidates = []
    for name in (
        "frankenstein.out",
        "parmys.out",
        "vpr.out",
        "abc.out",
        "abc0.out",
        "nohup.out",
    ):
        path = runDir / name
        if path.is_file():
            candidates.append(path)
    # also pick up the outer log if the compare wrote one under logs/
    if not candidates:
        return "starting"
    newest = max(candidates, key=lambda path: path.stat().st_mtime)
    try:
        lines = newest.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        return "running"
    return inferPhase(lines)


def parseStatusLine(label: str, line: str):
    result = {"label": label, "raw": line}
    statusMatch = re.match(r"\S+:\s+(\S+)", line)
    if statusMatch:
        result["status"] = statusMatch.group(1)
        # FAIL (reason) spans tokens; capture the rest until wall=
        if result["status"].startswith("FAIL"):
            failMatch = re.match(r"\S+:\s+(FAIL(?:\s*\([^)]*\))?)", line)
            if failMatch:
                result["status"] = failMatch.group(1)
    for key, pattern in (
        ("wall", wallRe),
        ("synth_wall", synthWallRe),
        ("abc_wall", abcWallRe),
        ("vpr_wall", vprWallRe),
        ("clb", clbRe),
        ("fmax", fmaxRe),
        ("wns", wnsRe),
        ("synth_luts", synthLutsRe),
        ("abc_luts", abcLutsRe),
        ("packed_luts", packedLutsRe),
        ("ff", ffRe),
        ("mem", memRe),
        ("dsp", dspRe),
        ("adder", adderRe),
        ("wl", wlRe),
        ("cpd", cpdRe),
    ):
        match = pattern.search(line)
        if match:
            result[key] = match.group(1)
    return result


def scanStatus(targetDir: Path):
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
            rows.append(parseStatusLine(label, line))
            continue
        runDir = runsDir / label
        if runDir.is_dir():
            phase = livePhaseFromRunDir(runDir)
            rows.append({"label": label, "status": f"running:{phase}"})
        else:
            rows.append({"label": label, "status": "pending"})
    return rows


def renderTable(rows, title):
    # each metric is one s/a/p column: synth, abc, packed (vpr for counts,
    # vpr wall for time). "" means that stage has no value for the metric.
    timeKeys = ("synth_wall", "abc_wall", "vpr_wall")
    sapGroups = (
        ("time", timeKeys, 18),
        ("lut", ("synth_luts", "abc_luts", "packed_luts"), 18),
        ("clb", ("", "", "clb"), 18),
        ("ff", ("", "", "ff"), 18),
        ("bram", ("", "", "mem"), 18),
        ("dsp", ("", "", "dsp"), 18),
        ("adder", ("", "", "adder"), 18),
    )
    tailKeys = ("wl", "cpd", "fmax", "wns")
    tailHeaders = ("wirelen", "cpd ns", "fmax MHz", "wns ns")
    labelWidth, statusWidth, wallWidth = 36, 22, 8

    def sapCell(row, keys):
        return "/".join(row.get(k, "-") or "-" if k else "-" for k in keys)

    def fmtSap(row, keys, width):
        text = sapCell(row, keys)
        return text if len(text) <= width else text[: width - 1] + "~"

    widths = (
        [labelWidth, statusWidth, wallWidth]
        + [g[2] for g in sapGroups]
        + [10, 8, 9, 8]
    )
    headers = (
        ["run label", "status", "wall(s)"]
        + [g[0] + " s/a/p" for g in sapGroups]
        + list(tailHeaders)
    )

    divider = "+" + "+".join("-" * (width + 2) for width in widths) + "+"
    headerRow = "|" + "|".join(
        f" {header:<{width}} " for header, width in zip(headers, widths)
    ) + "|"

    lines = [title, divider, headerRow, divider]
    for row in rows:
        status = row.get("status", "")
        if status == "ok":
            visibleText, color = "ok", "\033[92m"
        elif status.startswith("FAIL") or status == "fail":
            visibleText, color = status, "\033[91m"
        elif status.startswith("running"):
            phase = status.split(":", 1)[1] if ":" in status else ""
            visibleText = f"running ({phase})" if phase else "running"
            color = "\033[93m"
        elif status == "pending":
            visibleText, color = "pending", "\033[90m"
        elif status == "cached":
            visibleText, color = "cached", "\033[96m"
        else:
            visibleText, color = status, ""

        visibleText = visibleText[:statusWidth]
        statusCell = f"{color}{visibleText}\033[0m" if color else visibleText
        pad = max(0, statusWidth - len(visibleText))
        statusPadded = f" {statusCell}{' ' * pad} "

        cells = [
            f" {row.get('label', '')[:labelWidth]:<{labelWidth}} ",
            statusPadded,
            f" {row.get('wall', '-'):<{wallWidth}} ",
        ]
        for _, keys, width in sapGroups:
            cells.append(f" {fmtSap(row, keys, width):<{width}} ")
        for key, header, width in zip(tailKeys, tailHeaders, (10, 8, 9, 8)):
            cells.append(f" {str(row.get(key, '-')):<{width}} ")
        lines.append("|" + "|".join(cells) + "|")

    lines.append(divider)
    return "\n".join(lines)


# geomean compare across flows, over runs that finished ok on both.
# (column header, row key, higher_is_better)
geomeanColumns = (
    ("LUTs", "packed_luts", False),
    ("BRAMs", "mem", False),
    ("DSPs", "dsp", False),
    ("Adders", "adder", False),
    ("CLBs", "clb", False),
    ("Wirelen", "wl", False),
    ("CPD ns", "cpd", False),
    ("Fmax MHz", "fmax", True),
)


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
    # design -> {flow -> row}; keep only designs where every flow is ok
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
        and all(flowRows[f].get("status") == "ok" for f in flowsSeen)
    }
    if not paired:
        return None

    result = {}
    for flow in flowsSeen:
        result[flow] = {}
        for header, key, _ in geomeanColumns:
            values = [
                numeric(flowRows[flow].get(key))
                for flowRows in paired.values()
            ]
            values = [v for v in values if v is not None]
            if values:
                result[flow][key] = math.exp(sum(math.log(v) for v in values) / len(values))
            else:
                result[flow][key] = None
    return {"flows": flowsSeen, "geo": result, "n": len(paired)}


def renderGeomeanTable(rows):
    data = computeGeomeans(rows)
    if data is None:
        return "geomean: waiting for paired ok runs on both flows"
    flows = data["flows"]
    base = "vanilla_vtr" if "vanilla_vtr" in flows else flows[0]
    others = [f for f in flows if f != base]
    geo = data["geo"]
    n = data["n"]

    def fmt(value):
        return f"{value:,.2f}" if value is not None else "-"

    headers = ["flow"] + [header for header, _, _ in geomeanColumns]
    tableRows = []
    tableRows.append([f"{base} (n={n})"] + [fmt(geo[base].get(key)) for _, key, _ in geomeanColumns])
    for flow in others:
        tableRows.append([flow] + [fmt(geo[flow].get(key)) for _, key, _ in geomeanColumns])
    for flow in others:
        diffRow = [f"% diff {flow}/{base}"]
        for _, key, higherBetter in geomeanColumns:
            fVal, bVal = geo[flow].get(key), geo[base].get(key)
            if fVal is None or bVal is None:
                diffRow.append("-")
            else:
                diffRow.append(f"{(fVal / bVal - 1.0) * 100.0:+.2f}%")
        tableRows.append(diffRow)
        ratioRow = [f"x diff {flow}/{base}"]
        for _, key, higherBetter in geomeanColumns:
            fVal, bVal = geo[flow].get(key), geo[base].get(key)
            ratioRow.append("-" if fVal is None or bVal is None else f"{fVal / bVal:.2f}x")
        tableRows.append(ratioRow)

    colWidths = [
        max(len(str(r[i])) for r in [headers] + tableRows)
        for i in range(len(headers))
    ]
    divider = "+" + "+".join("-" * (w + 2) for w in colWidths) + "+"
    lines = ["geomean over paired ok runs:", divider]
    lines.append("|" + "|".join(f" {h:<{w}} " for h, w in zip(headers, colWidths)) + "|")
    lines.append(divider)
    for row in tableRows:
        lines.append("|" + "|".join(f" {str(c):<{w}} " for c, w in zip(row, colWidths)) + "|")
    lines.append(divider)
    return "\n".join(lines)


def watchDir(targetDir: Path, interval: float, once: bool):
    logsDir = targetDir / "logs"
    print(f"watching {targetDir} (interval={interval}s) — Ctrl-C to stop")

    try:
        while True:
            rows = scanStatus(targetDir)
            total = len(rows)
            done = sum(
                1
                for row in rows
                if row.get("status")
                and not row["status"].startswith("running")
                and row["status"] != "pending"
            )
            running = sum(1 for row in rows if row.get("status", "").startswith("running"))
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
            csvCandidates = sorted(targetDir.glob("*results*.csv"))
            if csvCandidates:
                print(f"csv:  {csvCandidates[-1]}")

            if once:
                break
            time.sleep(interval)
    except KeyboardInterrupt:
        print("\nstopped.")


def findOutputDirs():
    return sorted(
        path
        for path in vtrRoot.iterdir()
        if path.is_dir() and path.name.startswith("compare_output")
    )


def main(argv=None):
    parser = argparse.ArgumentParser(description="live status table for compare_flow")
    parser.add_argument("--dir", default=None, help="compare output directory")
    parser.add_argument("--interval", type=float, default=1.0, help="refresh seconds")
    parser.add_argument("--once", action="store_true", help="print once and exit")
    args = parser.parse_args(argv)

    if args.dir:
        targetDir = Path(args.dir)
        if not targetDir.is_absolute():
            targetDir = vtrRoot / targetDir
    else:
        preferred = vtrRoot / defaultOutDirName
        if preferred.is_dir():
            targetDir = preferred
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
