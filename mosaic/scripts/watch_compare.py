#!/usr/bin/env python3
"""live status table for mosaic/scripts/run_vtr_batch.py.

reads the results .csv file written by run_vtr_batch.py.

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

when --verilator-check <flows...> is set the table adds vcheck match mismatch vectors perrors
vcheck is pass, fail (mismatches), or error (could not run)
"""

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
except ImportError as exc:
    print("prettytable required: pip install prettytable", file=sys.stderr)
    raise SystemExit(1) from exc

SCRIPT_DIR = Path(__file__).resolve().parent
VTR_ROOT = SCRIPT_DIR.parents[1]
# new batches default under mosaic/scripts, and the repo root remains a fallback for older runs.
COMPARE_OUTPUT_ROOT = SCRIPT_DIR

PHASE_COLORS = {
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

SPINNER_FRAMES = ("-", "\\", "|", "/")

# csv field -> table/geomean short key
CSV_DISPLAY_KEYS = (
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
)

# (header, status key, higher_is_better)
GEOMEAN_COLUMNS = (
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


def strip_unit(value: str) -> str:
    """accept bare numbers or legacy status values like 12.3ns / 100MHz / 4.5s."""
    text = str(value).strip()
    if not text:
        return ""
    for suffix in ("ns", "MHz", "mhz", "s", "sec", "seconds", "Seconds"):
        if text.lower().endswith(suffix.lower()) and len(text) > len(suffix):
            trimmed = text[: -len(suffix)].strip()
            try:
                float(trimmed)
                return trimmed
            except ValueError:
                break
    return text


def resolve_csv_path(target_dir: Path) -> Path | None:
    """resolve the results CSV path from a compare output directory."""
    marker = target_dir / "status" / "csv_path.txt"
    if marker.is_file():
        text = marker.read_text(encoding="utf-8", errors="replace").strip()
        if text:
            path = Path(text)
            if path.is_file() or path.parent.is_dir():
                return path
    candidates = sorted(target_dir.glob("compare_results*.csv"))
    return candidates[-1] if candidates else None


def row_from_csv(raw: Dict[str, str]) -> Dict[str, str]:
    """map one results-csv row into the short keys used by the tables."""
    design = (raw.get("design") or "").strip()
    flow = (raw.get("flow") or "").strip()
    row: Dict[str, str] = {
        "label": f"{design}_{flow}" if design and flow else design or flow,
        "status": (raw.get("status") or "pending").strip() or "pending",
    }
    for short_key, field in CSV_DISPLAY_KEYS:
        value = raw.get(field, "")
        if value == "" or value is None:
            continue
        row[short_key] = strip_unit(value)
    return row


def load_rows(csv_path: Path) -> List[Dict[str, str]]:
    """load and parse all rows from the results CSV."""
    if not csv_path.is_file():
        return []
    try:
        with open(csv_path, newline="", encoding="utf-8") as handle:
            return [row_from_csv(dict(raw)) for raw in csv.DictReader(handle)]
    except OSError:
        return []


def numeric(value) -> Optional[float]:
    """return the numeric value as a float, or None if not parseable."""
    if value is None:
        return None
    try:
        number = float(strip_unit(value))
    except (TypeError, ValueError):
        return None
    return number if number > 0 else None


def flow_of(label: str) -> Optional[str]:
    """extract the flow suffix from a design_flow label."""
    for flow in ("mosaic", "vanilla_vtr"):
        if label.endswith("_" + flow):
            return flow
    return None


def compute_geomeans(rows: Sequence[Dict[str, str]]):
    """compute per-flow geomeans over designs that completed on every flow."""
    by_design: Dict[str, Dict[str, Dict[str, str]]] = {}
    flows_seen: List[str] = []
    for row in rows:
        flow = flow_of(row.get("label", ""))
        if flow is None:
            continue
        if flow not in flows_seen:
            flows_seen.append(flow)
        design = row["label"][: -(len(flow) + 1)]
        by_design.setdefault(design, {})[flow] = row

    if len(flows_seen) < 2:
        return None

    paired = {
        design: flow_rows
        for design, flow_rows in by_design.items()
        if len(flow_rows) == len(flows_seen)
        and all(is_geomean_ready(flow_rows[f]) for f in flows_seen)
    }
    if not paired:
        return None

    result = {}
    for flow in flows_seen:
        result[flow] = {}
        for _, key, _ in GEOMEAN_COLUMNS:
            values = [
                numeric(flow_rows[flow].get(key)) for flow_rows in paired.values()
            ]
            values = [v for v in values if v is not None]
            if values:
                result[flow][key] = math.exp(
                    sum(math.log(v) for v in values) / len(values)
                )
            else:
                result[flow][key] = None
    return {"flows": flows_seen, "geo": result}


def render_geomean_table(rows: Sequence[Dict[str, str]]) -> str:
    """render the geomean table with percent and ratio diffs vs vanilla_vtr."""
    data = compute_geomeans(rows)
    if data is None:
        return "geomean: waiting for paired ok/cached runs on both flows"

    flows = data["flows"]
    base = "vanilla_vtr" if "vanilla_vtr" in flows else flows[0]
    others = [f for f in flows if f != base]
    geo = data["geo"]

    def fmt(value):
        return f"{value:,.2f}" if value is not None else "-"

    table = PrettyTable()
    table.field_names = ["flow"] + [header for header, _, _ in GEOMEAN_COLUMNS]
    table.align = "l"
    table.add_row(
        [base] + [fmt(geo[base].get(key)) for _, key, _ in GEOMEAN_COLUMNS]
    )
    for flow in others:
        table.add_row(
            [flow] + [fmt(geo[flow].get(key)) for _, key, _ in GEOMEAN_COLUMNS]
        )
        diff_row = [f"% diff {flow}/{base}"]
        ratio_row = [f"x diff {flow}/{base}"]
        for _, key, _ in GEOMEAN_COLUMNS:
            f_val, b_val = geo[flow].get(key), geo[base].get(key)
            if f_val is None or b_val is None:
                diff_row.append("-")
                ratio_row.append("-")
            else:
                diff_row.append(f"{(f_val / b_val - 1.0) * 100.0:+.2f}%")
                ratio_row.append(f"{f_val / b_val:.2f}x")
        table.add_row(diff_row)
        table.add_row(ratio_row)
    return f"geomean:\n{table}"


def colored_status(status: str) -> str:
    """return the status string wrapped in ANSI color codes."""
    if status == "fail":
        return "\033[91mfail\033[0m"
    if status == "pending":
        return "\033[90mpending\033[0m"
    if status == "cached":
        return "\033[96mcached\033[0m"
    color = PHASE_COLORS.get(status, "\033[93m")
    return f"{color}{status or 'started'}\033[0m"


def colored_vcheck(value: str) -> str:
    """return the verilator check value wrapped in ANSI color codes."""
    text = (value or "-").strip() or "-"
    if text == "pass":
        return f"\033[92m{text}\033[0m"
    if text == "fail":
        return f"\033[91m{text}\033[0m"
    if text == "error":
        return f"\033[93m{text}\033[0m"
    return text


def sap(row: Dict[str, str], *keys: str) -> str:
    """join multiple row values with slash separators."""
    return "/".join(str(row.get(key) or "-") for key in keys)


def show_verilator_column(target_dir: Path, rows: Sequence[Dict[str, str]]) -> bool:
    """show vcheck when batch wrote the flag marker or any row has a value."""
    marker = target_dir / "status" / "verilator_check"
    if marker.is_file():
        return True
    return any(
        (row.get(key) or "").strip()
        for row in rows
        for key in ("vcheck", "match", "mismatch", "vectors", "perrors")
    )


def render_table(
    rows: List[Dict[str, str]],
    title: str,
    show_vcheck: bool = False,
) -> str:
    """render the main status table as a formatted string."""
    table = PrettyTable()
    field_names = [
        "run",
        "status",
    ]
    if show_vcheck:
        field_names.extend(["vcheck", "match", "mismatch", "vectors", "perrors"])
    field_names.extend(
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
    table.field_names = field_names
    table.align = "l"
    for row in rows:
        values = [
            row.get("label", ""),
            colored_status(row.get("status", "")),
        ]
        if show_vcheck:
            values.extend(
                [
                    colored_vcheck(row.get("vcheck", "")),
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


def find_output_dirs() -> List[Path]:
    """find all compare_output* directories sorted by mtime."""
    roots = [COMPARE_OUTPUT_ROOT]
    if COMPARE_OUTPUT_ROOT != VTR_ROOT:
        roots.append(VTR_ROOT)
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


def is_done_status(status: str) -> bool:
    """return whether the status represents a terminal state."""
    return status in ("route done", "ok", "cached", "fail")


def is_geomean_ready(row: Dict[str, str]) -> bool:
    """true when a row can enter the live geomean."""
    status = row.get("status", "")
    if status == "fail":
        return False
    if status not in ("route done", "ok", "cached"):
        return False
    return status == "cached" or numeric(row.get("wall")) is not None


def watch_dir(target_dir: Path, interval: float, once: bool) -> None:  # pylint: disable=too-many-locals
    """refresh the live status table until interrupted or --once."""
    logs_dir = target_dir / "logs"
    spinner_period = 0.25
    print(f"watching {target_dir} (interval={interval}s), Ctrl-C to stop")
    frame = 0
    last_csv_load = 0.0
    csv_path: Path | None = None
    rows: List[Dict[str, str]] = []
    try:
        while True:
            now = time.monotonic()
            if once or csv_path is None or (now - last_csv_load) >= interval:
                csv_path = resolve_csv_path(target_dir)
                rows = load_rows(csv_path) if csv_path is not None else []
                last_csv_load = now
            total = len(rows)
            done = sum(1 for row in rows if is_done_status(row.get("status", "")))
            pending = sum(1 for row in rows if row.get("status") == "pending")
            running = total - done - pending
            spinner = SPINNER_FRAMES[frame % len(SPINNER_FRAMES)] if running else ""
            title = (
                f"{target_dir.name}: {done}/{total} done, "
                f"{running} running, {pending} pending"
            )
            if spinner:
                title = f"{title} {spinner}"
            if not once:
                print("\033[H\033[J", end="")
            if csv_path is None:
                print(f"{title}\n(no results csv yet)")
            else:
                show_vcheck = show_verilator_column(target_dir, rows)
                print(render_table(rows, title, show_vcheck=show_vcheck))
                print()
                print(render_geomean_table(rows))
            print(f"\nlogs: {logs_dir}")
            if csv_path is not None:
                print(f"csv:  {csv_path}")
            if once:
                break
            frame += 1
            time.sleep(spinner_period)
    except KeyboardInterrupt:
        print("\nstopped.")


def main(argv=None) -> int:
    """entry point: parse arguments and start the watcher."""
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
        target_dir = Path(args.dir)
        if not target_dir.is_absolute():
            candidate = COMPARE_OUTPUT_ROOT / target_dir
            target_dir = candidate if candidate.is_dir() else (VTR_ROOT / target_dir)
    else:
        candidates = find_output_dirs()
        if not candidates:
            print("no compare_output* directories found", file=sys.stderr)
            return 1
        target_dir = candidates[-1]
        print(f"auto-selected: {target_dir.name}")

    if not target_dir.is_dir():
        print(f"missing outdir: {target_dir}", file=sys.stderr)
        return 1

    watch_dir(target_dir, max(0.2, args.interval), args.once)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
