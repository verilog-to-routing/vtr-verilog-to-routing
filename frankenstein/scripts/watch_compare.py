#!/usr/bin/env python3
"""
live status table for frankenstein/scripts/compare_k6.py.

scans <outdir>/status/*.txt and live phase hints from run dirs. run in a
second terminal while the compare is going.

usage:
  python3 frankenstein/scripts/watch_compare.py
  python3 frankenstein/scripts/watch_compare.py --dir compare_output_k6
  python3 frankenstein/scripts/watch_compare.py --interval 2
  python3 frankenstein/scripts/watch_compare.py --once
"""

from __future__ import annotations

import argparse
import re
import sys
import time
from pathlib import Path

scriptDir = Path(__file__).resolve().parent
vtrRoot = scriptDir.parents[1]
defaultOutDir = vtrRoot / "compare_output_k6"

fmaxRe = re.compile(r"fmax=([0-9.]+)MHz")
clbRe = re.compile(r"clb=(\d+)")
wallRe = re.compile(r"wall=([0-9.]+)s")
wnsRe = re.compile(r"wns=([0-9.-]+)ns")
packedLutsRe = re.compile(r"packed_luts=(\d+)")
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
        ("clb", clbRe),
        ("fmax", fmaxRe),
        ("wns", wnsRe),
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
    colWidths = {
        "label": 36,
        "status": 22,
        "wall": 8,
        "clb": 6,
        "luts": 8,
        "ff": 6,
        "mem": 5,
        "dsp": 5,
        "adder": 7,
        "wl": 10,
        "cpd": 8,
        "fmax": 9,
        "wns": 8,
    }
    headers = (
        "run label",
        "status",
        "wall(s)",
        "clbs",
        "luts",
        "ffs",
        "bram",
        "dsp",
        "adder",
        "wirelen",
        "cpd ns",
        "fmax MHz",
        "wns ns",
    )
    keys = (
        "label",
        "status",
        "wall",
        "clb",
        "packed_luts",
        "ff",
        "mem",
        "dsp",
        "adder",
        "wl",
        "cpd",
        "fmax",
        "wns",
    )
    widths = [colWidths[k] for k in (
        "label", "status", "wall", "clb", "luts", "ff", "mem", "dsp", "adder", "wl", "cpd", "fmax", "wns"
    )]

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

        width = widths[1]
        visibleText = visibleText[:width]
        statusCell = f"{color}{visibleText}\033[0m" if color else visibleText
        pad = max(0, width - len(visibleText))
        statusPadded = f" {statusCell}{' ' * pad} "

        cells = [statusPadded]
        dataKeys = [k for k in keys if k != "status"]
        dataWidths = [w for i, w in enumerate(widths) if i != 1]
        for key, width in zip(dataKeys, dataWidths):
            if key == "label":
                cells.insert(0, f" {row.get(key, '')[:width]:<{width}} ")
            else:
                cells.append(f" {str(row.get(key, '-')):<{width}} ")
        lines.append("|" + "|".join(cells) + "|")

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
            print(f"\nlogs: {logsDir}")
            csvPath = targetDir / "compare_k6_results.csv"
            if csvPath.is_file():
                print(f"csv:  {csvPath}")

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
    parser = argparse.ArgumentParser(description="live status table for compare_k6")
    parser.add_argument("--dir", default=None, help="compare output directory")
    parser.add_argument("--interval", type=float, default=1.0, help="refresh seconds")
    parser.add_argument("--once", action="store_true", help="print once and exit")
    args = parser.parse_args(argv)

    if args.dir:
        targetDir = Path(args.dir)
        if not targetDir.is_absolute():
            targetDir = vtrRoot / targetDir
    elif defaultOutDir.is_dir():
        targetDir = defaultOutDir
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
