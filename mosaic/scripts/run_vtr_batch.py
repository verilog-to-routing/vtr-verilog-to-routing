# pylint: disable=too-many-lines
"""batch runner built around run_vtr_flow.py for comparing mosaic and parmys."""
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

SCRIPT_DIR = Path(__file__).resolve().parent
VTR_ROOT = SCRIPT_DIR.parents[1]
# default batch output root keeps compare_output_* under mosaic/scripts.
COMPARE_OUTPUT_ROOT = SCRIPT_DIR
VTR_FLOW = VTR_ROOT / "vtr_flow"
RUN_VTR_FLOW = VTR_FLOW / "scripts" / "run_vtr_flow.py"
YOSYS_BIN = VTR_ROOT / "build" / "bin" / "yosys"
PLUGIN_PATH = VTR_ROOT / "build" / "share" / "yosys" / "plugins" / "mosaic.so"

FLOWS = {
    "vanilla_vtr": {"start": "parmys"},
    "mosaic": {"start": "mosaic"},
}
FLOW_ALIASES = {
    "vtr": "vanilla_vtr",
    "vanilla": "vanilla_vtr",
    "parmys": "vanilla_vtr",
    "frank": "mosaic",
}

VPR_BRAM_BLOCK_TYPES = ("memory", "bram_multimode")
# koios uses dsp_top, classic k6 uses mult_36, and some arches use mae or dsp.
VPR_DSP_BLOCK_TYPES = ("mult_36", "mae", "dsp", "dsp_top")

CSV_FIELDS = (
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
STATUS_RANK = {
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

PHASE_PATTERNS = [
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

STATUS_KEYS = (
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


def resolve_path(value: str, base: Path) -> Path:
    """resolve *value* against *base* if it is not already absolute."""
    path = Path(value).expanduser()
    return path if path.is_absolute() else (base / path)


def discover_designs(bench_dir: Path) -> List[str]:
    """return sorted stems of all *.v files except *_include.v."""
    designs = []
    for path in sorted(bench_dir.glob("*.v")):
        if path.name.endswith("_include.v"):
            continue
        designs.append(path.stem)
    return designs


def resolve_designs(names: Optional[Sequence[str]], bench_dir: Path) -> List[str]:
    """validate explicit design names or auto-discover from *bench_dir*."""
    designs = list(names) if names else discover_designs(bench_dir)
    if not designs:
        raise SystemExit(f"no designs found under {bench_dir}")
    missing = [name for name in designs if not (bench_dir / f"{name}.v").is_file()]
    if missing:
        available = discover_designs(bench_dir)
        hint = ", ".join(available[:12]) if available else "(none)"
        if len(available) > 12:
            hint += ", ..."
        raise SystemExit(
            "missing verilog for design(s): "
            + ", ".join(missing)
            + f"\n  looked in: {bench_dir}"
            + f"\n  available: {hint}"
        )
    return designs


def normalize_flows(names: Optional[Sequence[str]]) -> List[str]:
    """resolve flow aliases and return a deduplicated ordered list."""
    if not names:
        return list(FLOWS)
    selected = []
    for name in names:
        resolved = FLOW_ALIASES.get(name, name)
        if resolved not in FLOWS:
            raise SystemExit(f"unknown flow: {name} (want: {', '.join(FLOWS)})")
        if resolved not in selected:
            selected.append(resolved)
    return selected


def resolve_include_paths(
    include_args: Optional[Sequence[str]], bench_dir: Path,
) -> List[Path]:
    """make include paths absolute, raising if any do not exist."""
    resolved: List[Path] = []
    if not include_args:
        return resolved
    for include_arg in include_args:
        include_path = Path(include_arg)
        if not include_path.is_absolute():
            include_path = bench_dir / include_path
        include_path = include_path.resolve()
        if not include_path.is_file():
            raise FileNotFoundError(f"include file not found: {include_path}")
        resolved.append(include_path)
    return resolved


def check_prerequisites(need_mosaic: bool, arch_file: Path, bench_dir: Path) -> None:
    """abort if required binaries or directories are missing."""
    missing = []
    for path in (RUN_VTR_FLOW, arch_file, YOSYS_BIN):
        if not path.is_file():
            missing.append(str(path))
    if not bench_dir.is_dir():
        missing.append(str(bench_dir))
    if need_mosaic and not PLUGIN_PATH.is_file():
        missing.append(str(PLUGIN_PATH))
    if missing:
        print("missing prerequisites:", file=sys.stderr)
        for path in missing:
            print(f"  {path}", file=sys.stderr)
        raise SystemExit(1)


def status_sort_key(status: str) -> int:
    """return numeric rank for a status string (higher = further along)."""
    if status in STATUS_RANK:
        return STATUS_RANK[status]
    return STATUS_RANK.get("started", 1)


def prefer_status(current: str, proposed: str) -> str:
    """keep the furthest progress status (never regress from place done to synth)."""
    if not proposed:
        return current or "pending"
    if not current:
        return proposed
    if status_sort_key(proposed) >= status_sort_key(current):
        return proposed
    return current


def detect_phase(run_dir: Path) -> str:
    """infer current phase from the newest log tails in a run dir."""
    for name in (
        "vpr.out",
        "vpr_stdout.log",
        "mosaic.out",
        "parmys.out",
        "output.txt",
    ):
        path = run_dir / name
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
        for pattern, label in PHASE_PATTERNS:
            if pattern.search(text):
                return label
    return "started"


def with_csv_lock(csv_path: Path, callback):
    """execute *callback* under an exclusive file lock on *csv_path*."""
    csv_path.parent.mkdir(parents=True, exist_ok=True)
    lock_path = csv_path.with_suffix(csv_path.suffix + ".lock")
    with open(lock_path, "a+", encoding="utf-8") as lock_file:
        if fcntl is not None:
            fcntl.flock(lock_file.fileno(), fcntl.LOCK_EX)
        try:
            return callback()
        finally:
            if fcntl is not None:
                fcntl.flock(lock_file.fileno(), fcntl.LOCK_UN)


def load_csv_rows(csv_path: Path) -> List[Dict]:
    """return every row from *csv_path* as a list of dicts."""
    if not csv_path.is_file():
        return []
    try:
        with open(csv_path, newline="", encoding="utf-8") as handle:
            return [dict(row) for row in csv.DictReader(handle)]
    except OSError:
        return []


def write_csv(
    csv_path: Path,
    rows: List[Dict],
    order: Optional[Dict[Tuple[str, str], int]] = None,
) -> None:
    """write *rows* to *csv_path* in the canonical CSV_FIELDS order."""
    csv_path.parent.mkdir(parents=True, exist_ok=True)

    def sort_key(row: Dict):
        key = (row.get("design", ""), row.get("flow", ""))
        if order is not None:
            return (order.get(key, 10**9), key)
        return key

    with open(csv_path, "w", newline="", encoding="utf-8") as out_file:
        writer = csv.DictWriter(out_file, fieldnames=CSV_FIELDS, extrasaction="ignore")
        writer.writeheader()
        for row in sorted(rows, key=sort_key):
            writer.writerow({field: row.get(field, "") for field in CSV_FIELDS})


def upsert_csv_row(
    csv_path: Path,
    row: Dict,
    order: Optional[Dict[Tuple[str, str], int]] = None,
) -> None:
    """merge one row into the live results csv under an exclusive lock."""
    design = row.get("design", "")
    flow = row.get("flow", "")
    if not design or not flow:
        return

    def merge() -> None:
        index: Dict[Tuple[str, str], Dict] = {}
        for existing in load_csv_rows(csv_path):
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
            merged["status"] = prefer_status(
                str(previous.get("status", "")), str(row["status"])
            )
        elif "status" not in merged:
            merged["status"] = "pending"
        merged["design"] = design
        merged["flow"] = flow
        index[key] = merged
        write_csv(csv_path, list(index.values()), order=order)

    with_csv_lock(csv_path, merge)


def format_summary(run_label: str, row: Dict, status: Optional[str] = None) -> str:
    """return a one-line human-readable summary of a run result."""
    if status is None:
        status = row.get("status") or (
            "route done" if row.get("success") else "fail"
        )
    parts = [f"{run_label}: {status}"]
    for short_key, field in STATUS_KEYS:
        value = row.get(field, "")
        if value == "" or value is None:
            continue
        parts.append(f"{short_key}={value}")
    return " ".join(parts)


def count_blif_names(blif_path: Path) -> str:
    """count .names lines in a BLIF file."""
    if not blif_path.is_file():
        return ""
    try:
        count = 0
        with open(blif_path, encoding="utf-8", errors="replace") as blif_file:
            for line in blif_file:
                if line.startswith(".names"):
                    count += 1
        return str(count)
    except OSError:
        return ""


def count_blif_latches(blif_path: Path) -> str:
    """count .latch lines in a BLIF file."""
    if not blif_path.is_file():
        return ""
    try:
        count = 0
        with open(blif_path, encoding="utf-8", errors="replace") as blif_file:
            for line in blif_file:
                if line.startswith(".latch"):
                    count += 1
        return str(count)
    except OSError:
        return ""


def parse_yosys_ff_count(text: str) -> str:
    """sum $_DFF_*/$_DFFE_* counts from a yosys stat dump."""
    total = 0
    found = False
    for line in text.splitlines():
        cell_match = re.match(r"^\s+(\$_DFF\S*|\$_DFFE\S*)\s+(\d+)\s*$", line)
        if cell_match:
            total += int(cell_match.group(2))
            found = True
    return str(total) if found else ""


def stage_lut_counts(temp_dir: Path, design: str) -> Dict[str, str]:
    """collect synth/abc LUT and FF counts from the run directory."""
    counts = {"synth_luts": "", "abc_luts": "", "synth_ff": ""}
    for key, names in (
        ("synth_luts", (f"{design}.mosaic.blif", f"{design}.parmys.blif")),
        ("abc_luts", (f"{design}.pre-vpr.blif",)),
    ):
        for name in names:
            path = temp_dir / name
            value = count_blif_names(path)
            if value:
                counts[key] = value
                break
    # mosaic abc is in-yosys, so pre-vpr is the post-abc netlist. when only
    # mosaic.blif exists, treat that as synth and leave abc blank.
    if counts["synth_luts"] and not counts["abc_luts"]:
        pre_vpr = temp_dir / f"{design}.pre-vpr.blif"
        frank = temp_dir / f"{design}.mosaic.blif"
        if pre_vpr.is_file() and frank.is_file():
            counts["abc_luts"] = count_blif_names(pre_vpr)
    for log_name in ("mosaic.out", "parmys.out"):
        log_path = temp_dir / log_name
        if not log_path.is_file():
            continue
        try:
            ff_count = parse_yosys_ff_count(
                log_path.read_text(encoding="utf-8", errors="replace")
            )
        except OSError:
            ff_count = ""
        if ff_count:
            counts["synth_ff"] = ff_count
            break
    if not counts["synth_ff"]:
        synth_blif = temp_dir / f"{design}.mosaic.blif"
        if not synth_blif.is_file():
            synth_blif = temp_dir / f"{design}.parmys.blif"
        counts["synth_ff"] = count_blif_latches(synth_blif)
    return counts


def frontend_ready(temp_dir: Path, design: str, flow_name: str) -> bool:
    """return True when the frontend has written a BLIF that VPR can consume."""
    if flow_name == "mosaic":
        return (temp_dir / f"{design}.mosaic.blif").is_file() or (
            temp_dir / f"{design}.pre-vpr.blif"
        ).is_file()
    return (temp_dir / f"{design}.pre-vpr.blif").is_file()


def abc_wall_from_mtimes(temp_dir: Path, design: str) -> str:
    """estimate abc wall-clock from mtime gap between parmys and pre-vpr BLIFs."""
    parmys_blif = temp_dir / f"{design}.parmys.blif"
    pre_vpr = temp_dir / f"{design}.pre-vpr.blif"
    if not parmys_blif.is_file() or not pre_vpr.is_file():
        return ""
    try:
        abc_sec = max(0.0, pre_vpr.stat().st_mtime - parmys_blif.stat().st_mtime)
    except OSError:
        return ""
    return f"{abc_sec:.2f}"


def times_from_timestamps(  # pylint: disable=too-many-arguments
    start_unix: float,
    synth_finish_unix: Optional[float],
    vpr_finish_unix: float,
    temp_dir: Path,
    design: str,
    vpr_runtime_sec: str = "",
) -> Dict[str, str]:
    """derive wall / synth / vpr timings from unix timestamps."""
    if synth_finish_unix is None:
        synth_finish_unix = vpr_finish_unix
    synth_finish_unix = min(max(synth_finish_unix, start_unix), vpr_finish_unix)
    wall_sec = max(0.0, vpr_finish_unix - start_unix)
    synth_sec = max(0.0, synth_finish_unix - start_unix)
    if vpr_runtime_sec:
        try:
            vpr_sec = max(0.0, float(vpr_runtime_sec))
        except ValueError:
            vpr_sec = max(0.0, vpr_finish_unix - synth_finish_unix)
    else:
        vpr_sec = max(0.0, vpr_finish_unix - synth_finish_unix)
    return {
        "start_unix": f"{start_unix:.3f}",
        "synth_finish_unix": f"{synth_finish_unix:.3f}",
        "vpr_finish_unix": f"{vpr_finish_unix:.3f}",
        "wall_time_sec": f"{wall_sec:.2f}",
        "synth_wall_sec": f"{synth_sec:.2f}",
        "vpr_wall_sec": f"{vpr_sec:.2f}",
        "synthesis_sec": f"{synth_sec:.2f}",
        "abc_wall_sec": abc_wall_from_mtimes(temp_dir, design),
    }


def latest_results_csv(out_dir: Path, exclude: Optional[Path] = None) -> Optional[Path]:
    """return the newest prior compare_results_*.csv in *out_dir* for --no-rerun."""
    exclude_resolved = exclude.resolve() if exclude is not None else None
    candidates = sorted(out_dir.glob("compare_results*.csv"))
    for path in reversed(candidates):
        if exclude_resolved is not None and path.resolve() == exclude_resolved:
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


def load_csv_index(csv_path: Path) -> Dict[Tuple[str, str], Dict]:
    """index prior csv rows by (design, flow) for --no-rerun cache hits."""
    index: Dict[Tuple[str, str], Dict] = {}
    if not csv_path.is_file():
        return index
    try:
        with open(csv_path, newline="", encoding="utf-8") as handle:
            for row in csv.DictReader(handle):
                design = (row.get("design") or "").strip()
                flow = (row.get("flow") or "").strip()
                if design and flow:
                    index[(design, flow)] = dict(row)
    except OSError:
        return {}
    return index


def row_from_prior_csv(prior_row: Dict) -> Dict:
    """normalize a csv row reloaded for a cached --no-rerun hit."""
    row = {field: prior_row.get(field, "") for field in CSV_FIELDS}
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


def parse_packed_luts(vpr_text: str) -> str:
    """extract packed LUT count from VPR output."""
    match = re.search(r"^\s+\.names\s*:\s*(\d+)\s*$", vpr_text, re.MULTILINE)
    return match.group(1) if match else ""


def parse_vpr_ff_count(vpr_text: str) -> str:
    """extract flip-flop count from VPR output."""
    ff_pb_match = re.search(r"^\s+ff\s+:\s*(\d+)\s*$", vpr_text, re.MULTILINE)
    if ff_pb_match:
        return ff_pb_match.group(1)
    latch_match = re.search(r"^\s+\.latch\s*:\s*(\d+)\s*$", vpr_text, re.MULTILINE)
    return latch_match.group(1) if latch_match else ""


def parse_vpr_qor(  # pylint: disable=too-many-locals,too-many-branches
    temp_dir: Path,
) -> Dict[str, str]:
    """parse VPR quality-of-results metrics from vpr.out and crit_path.out."""
    vpr_out = temp_dir / "vpr.out"
    metrics = {
        field: ""
        for field in CSV_FIELDS
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
    if not vpr_out.is_file() or vpr_out.stat().st_size == 0:
        return metrics

    text = vpr_out.read_text(encoding="utf-8", errors="replace")
    if re.search(r"Final critical path", text) or (
        "The entire flow of VPR took" in text and "Netlist clb blocks:" in text
    ):
        metrics["vpr_status"] = "ok"
    elif re.search(r"\bfailed\b", text, re.IGNORECASE):
        metrics["vpr_status"] = "fail"

    block_counts: Dict[str, int] = {}
    for block_match in re.finditer(r"Netlist (\S+) blocks:\s*(\d+)", text):
        block_counts[block_match.group(1)] = int(block_match.group(2))
    if "clb" in block_counts:
        metrics["num_clb"] = str(block_counts["clb"])
    metrics["num_memory"] = str(sum(block_counts.get(b, 0) for b in VPR_BRAM_BLOCK_TYPES))
    metrics["num_dsp"] = str(sum(block_counts.get(b, 0) for b in VPR_DSP_BLOCK_TYPES))

    io_in = re.search(r"Netlist inputs pins:\s*(\d+)", text)
    if io_in:
        metrics["num_io_in"] = io_in.group(1)
    io_out = re.search(r"Netlist output pins:\s*(\d+)", text)
    if io_out:
        metrics["num_io_out"] = io_out.group(1)

    metrics["packed_luts"] = parse_packed_luts(text)
    metrics["num_ff"] = parse_vpr_ff_count(text)

    adder_pb_match = re.search(r"^\s+adder\s+:\s*(\d+)", text, re.MULTILINE)
    if adder_pb_match:
        metrics["num_adder"] = adder_pb_match.group(1)

    wns_match = re.search(
        r"worst.negative.slack[^:\n]*:\s*([0-9.-]+)\s*ns", text, re.IGNORECASE
    )
    if wns_match:
        metrics["worst_slack_ns"] = wns_match.group(1)

    wire_match = re.search(r"Total wirelength:\s*(\d+)", text)
    if wire_match:
        metrics["total_wire_length"] = wire_match.group(1)

    runtime_match = re.search(r"The entire flow of VPR took ([0-9.]+) seconds", text)
    vpr_runtime = runtime_match.group(1) if runtime_match else ""

    crit_file = temp_dir / "vpr.crit_path.out"
    crit_text = (
        crit_file.read_text(encoding="utf-8", errors="replace")
        if crit_file.is_file()
        else text
    )
    crit_match = re.search(
        r"Final critical path[^:]*:\s*([0-9.]+)\s*ns(?:,\s*Fmax:\s*([0-9.]+)\s*MHz)?",
        crit_text,
    )
    if crit_match:
        cpd = float(crit_match.group(1))
        metrics["crit_path_delay_ns"] = crit_match.group(1)
        if crit_match.group(2):
            metrics["fmax_mhz"] = crit_match.group(2)
        elif cpd > 0:
            metrics["fmax_mhz"] = f"{1000.0 / cpd:.2f}"

    metrics["_vpr_runtime_sec"] = vpr_runtime
    return metrics


def extract_fail_reason(temp_dir: Path, flow_name: str, return_code: int) -> str:
    """scan log files for the last error message to summarize a failure."""
    stage_name = "mosaic" if flow_name == "mosaic" else "parmys"
    candidates = (
        temp_dir / "output.txt",
        temp_dir / f"{stage_name}.out",
        temp_dir / "vpr.out",
    )
    error_re = re.compile(
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
        matches = error_re.findall(text)
        if not matches:
            continue
        snippet = re.sub(r"\s+", " ", matches[-1]).strip()
        if len(snippet) > 120:
            snippet = snippet[:117] + "..."
        return f"{path.name}: {snippet}"
    if return_code == 139:
        return "rc=139 (segfault)"
    if return_code != 0:
        return f"rc={return_code}"
    return "missing"


def single_core_env() -> Dict[str, str]:
    """return an env dict that pins each child process to one worker/thread."""
    env = os.environ.copy()
    env["VPR_NUM_WORKERS"] = "1"
    env["OMP_NUM_THREADS"] = "1"
    env["MKL_NUM_THREADS"] = "1"
    env["OPENBLAS_NUM_THREADS"] = "1"
    return env


def run_one(  # pylint: disable=too-many-locals,too-many-branches,too-many-statements
    task: Tuple,
) -> Dict:
    """execute a single run_vtr_flow invocation and collect results."""
    (
        design,
        flow_name,
        out_dir,
        arch_file,
        bench_dir,
        no_clean,
        no_rerun,
        include_files,
        route_chan_width,
        prior_row,
        csv_path,
        order,
        verilator_check,
        verilator_vectors,
        verilator_seed,
    ) = task
    run_label = f"{design}_{flow_name}"
    temp_dir = (out_dir / "runs" / run_label).resolve()
    log_path = (out_dir / "logs" / f"{run_label}.log").resolve()
    success_marker = temp_dir / ".success"
    circuit_path = bench_dir / f"{design}.v"

    def publish(row: Dict) -> None:
        upsert_csv_row(csv_path, row, order=order)

    if no_rerun and success_marker.is_file():
        if prior_row:
            row = row_from_prior_csv(prior_row)
        else:
            row = {
                "design": design,
                "flow": flow_name,
                "status": "cached",
                "success": True,
                "vpr_status": "cached",
                "return_code": 0,
            }
        publish(row)
        return row

    if not circuit_path.is_file():
        row = {
            "design": design,
            "flow": flow_name,
            "status": "fail",
            "success": False,
            "vpr_status": "missing_verilog",
            "wall_time_sec": "",
            "return_code": 1,
        }
        publish(row)
        return row

    if temp_dir.exists() and not no_clean:
        shutil.rmtree(temp_dir)
    temp_dir.mkdir(parents=True, exist_ok=True)
    log_path.parent.mkdir(parents=True, exist_ok=True)

    # default vtr call uses -j/--num_workers 1 so vpr stays on one core per run.
    cmd = [
        sys.executable,
        str(RUN_VTR_FLOW.resolve()),
        str(circuit_path.resolve()),
        str(arch_file.resolve()),
        "-start",
        FLOWS[flow_name]["start"],
        "-temp_dir",
        str(temp_dir),
        "-name",
        run_label,
        "-track_memory_usage",
        "--pack",
        "--place",
        "--route",
        "--analysis",
        "--route_chan_width",
        str(route_chan_width),
        "-crit_path_router_iterations",
        "100",
        "--num_workers",
        "1",
    ]
    if include_files:
        cmd += ["-include", *[str(path) for path in include_files]]
    if verilator_check:
        cmd += [
            "-verilator_check",
            "-verilator_check_vectors",
            str(verilator_vectors),
            "-verilator_check_seed",
            str(verilator_seed),
        ]

    # wall is process lifetime. synth is until frontend blif. vpr is VPR's own runtime
    start_unix = time.time()
    synth_finish_unix = None
    live_status = "started"
    publish({"design": design, "flow": flow_name, "status": live_status})
    with open(log_path, "w", encoding="utf-8", errors="replace") as log_file:
        log_file.write("CMD: " + " ".join(cmd) + "\n\n")
        log_file.flush()
        proc = subprocess.Popen(  # pylint: disable=consider-using-with
            cmd,
            cwd=str(VTR_ROOT),
            stdout=log_file,
            stderr=subprocess.STDOUT,
            text=True,
            env=single_core_env(),
        )
        while True:
            return_code = proc.poll()
            if synth_finish_unix is None and frontend_ready(temp_dir, design, flow_name):
                synth_finish_unix = time.time()
            phase = detect_phase(temp_dir)
            next_status = prefer_status(live_status, phase)
            if next_status != live_status:
                live_status = next_status
                publish({"design": design, "flow": flow_name, "status": live_status})
            if return_code is not None:
                break
            time.sleep(0.25)
    vpr_finish_unix = time.time()
    if synth_finish_unix is None and frontend_ready(temp_dir, design, flow_name):
        synth_finish_unix = vpr_finish_unix

    qor = parse_vpr_qor(temp_dir)
    vpr_runtime = qor.pop("_vpr_runtime_sec", "")
    vpr_out = temp_dir / "vpr.out"
    if not vpr_runtime and (not vpr_out.is_file() or vpr_out.stat().st_size == 0):
        vpr_runtime = "0"
    stage_times = times_from_timestamps(
        start_unix,
        synth_finish_unix,
        vpr_finish_unix,
        temp_dir,
        design,
        vpr_runtime_sec=vpr_runtime,
    )
    qor.update(stage_lut_counts(temp_dir, design))
    qor.update(stage_times)
    if qor.get("vpr_status") == "missing":
        qor["vpr_status"] = extract_fail_reason(temp_dir, flow_name, return_code)
    success = return_code == 0 and qor["vpr_status"] == "ok"
    vcheck = read_verilator_metrics(temp_dir, verilator_check)
    # final identity/status fields override any same-named keys from qor
    row = {
        **qor,
        "design": design,
        "flow": flow_name,
        "status": "route done" if success else "fail",
        "success": success,
        "return_code": return_code,
        **vcheck,
    }
    if success:
        try:
            success_marker.touch()
        except OSError:
            pass

    publish(row)
    return row


def read_verilator_metrics(temp_dir: Path, enabled: bool) -> Dict[str, str]:
    """parse verilator random-check results: pass / fail / error."""
    empty = {
        "verilator_status": "",
        "verilator_matched": "",
        "verilator_mismatched": "",
        "verilator_vectors": "",
        "verilator_port_errors": "",
    }
    if not enabled:
        return empty
    log_path = temp_dir / "verilator_random_check.out"
    if not log_path.is_file():
        return {**empty, "verilator_status": "error"}
    try:
        text = log_path.read_text(encoding="utf-8", errors="replace")
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

    tool_error = bool(
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
    elif tool_error:
        metrics["verilator_status"] = "error"
    else:
        metrics["verilator_status"] = "error"
    return metrics


def run_pool(
    tasks: List[Tuple],
    jobs: int,
    _csv_path: Path,
    order: Dict[Tuple[str, str], int],
    quiet: bool,
) -> List[Dict]:
    """dispatch tasks to a process pool and collect results."""
    live_results: List[Dict] = []

    if not tasks:
        return []

    if jobs <= 1 or len(tasks) <= 1:
        for task in tasks:
            live_results.append(run_one(task))
            if not quiet:
                print(format_summary(f"{task[0]}_{task[1]}", live_results[-1]))
        return live_results

    with ProcessPoolExecutor(max_workers=jobs) as pool:
        futures = {pool.submit(run_one, task): task for task in tasks}
        for future in as_completed(futures):
            row = future.result()
            live_results.append(row)
            if not quiet:
                print(format_summary(f"{row['design']}_{row['flow']}", row))
    live_results.sort(key=lambda row: order.get((row["design"], row["flow"]), 0))
    return live_results


def start_watch(out_dir: Path, interval: float) -> subprocess.Popen:
    """spawn the watch_compare.py watcher subprocess."""
    watch_script = SCRIPT_DIR / "watch_compare.py"
    return subprocess.Popen(  # pylint: disable=consider-using-with
        [
            sys.executable,
            str(watch_script),
            "--dir",
            str(out_dir),
            "--interval",
            str(interval),
        ],
        cwd=str(VTR_ROOT),
    )


def stop_watch(proc: Optional[subprocess.Popen]) -> None:
    """terminate the watcher subprocess gracefully."""
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


def main(  # pylint: disable=too-many-locals,too-many-branches,too-many-statements
    argv: Optional[Sequence[str]] = None,
) -> int:
    """parse arguments, launch batch runs, write final csv."""
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

    arch_file = resolve_path(args.arch, VTR_ROOT)
    bench_dir = resolve_path(args.benchmark_dir, VTR_ROOT)
    designs = resolve_designs(args.designs, bench_dir)
    selected_flows = normalize_flows(args.flows)
    vcheck_flows = normalize_flows(args.verilator_check) if args.verilator_check else []
    missing_vcheck = [flow for flow in vcheck_flows if flow not in selected_flows]
    if missing_vcheck:
        parser.error(
            "--verilator-check flow not in --flows: "
            + ", ".join(missing_vcheck)
            + " (selected: "
            + ", ".join(selected_flows)
            + ")"
        )
    jobs = max(1, args.jobs)
    include_files = resolve_include_paths(args.include, bench_dir)
    out_dir = resolve_path(
        args.outdir or f"compare_output_{arch_file.stem}", COMPARE_OUTPUT_ROOT
    )
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_path = (
        resolve_path(args.csv, VTR_ROOT)
        if args.csv
        else (out_dir / f"compare_results_{stamp}.csv")
    )

    check_prerequisites("mosaic" in selected_flows, arch_file, bench_dir)

    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "runs").mkdir(exist_ok=True)
    (out_dir / "logs").mkdir(exist_ok=True)
    status_dir = out_dir / "status"
    if status_dir.exists() and not args.no_rerun:
        shutil.rmtree(status_dir, ignore_errors=True)
    status_dir.mkdir(exist_ok=True)

    labels = [f"{design}_{flow}" for design in designs for flow in selected_flows]
    (status_dir / "manifest.txt").write_text("\n".join(labels) + "\n", encoding="utf-8")
    (status_dir / "csv_path.txt").write_text(str(csv_path.resolve()) + "\n", encoding="utf-8")
    if vcheck_flows:
        (status_dir / "verilator_check").write_text(
            " ".join(vcheck_flows) + "\n", encoding="utf-8"
        )
    else:
        marker = status_dir / "verilator_check"
        if marker.is_file():
            marker.unlink()

    # --no-rerun reloads timing/qor for cached runs from the newest prior csv.
    prior_index: Dict[Tuple[str, str], Dict] = {}
    if args.no_rerun:
        prior_csv = latest_results_csv(out_dir, exclude=csv_path)
        if prior_csv is not None:
            prior_index = load_csv_index(prior_csv)
            print(f"prior:   {prior_csv} ({len(prior_index)} rows)")

    order = {
        (design, flow_name): i
        for i, (design, flow_name) in enumerate(
            (design, flow_name)
            for design in designs
            for flow_name in selected_flows
        )
    }
    # seed the csv with every run so watch_compare can read status-only rows.
    seed_rows = [
        {
            "design": design,
            "flow": flow_name,
            "status": "pending",
            "success": "",
        }
        for design in designs
        for flow_name in selected_flows
    ]
    write_csv(csv_path, seed_rows, order=order)

    tasks = [
        (
            design,
            flow_name,
            out_dir,
            arch_file,
            bench_dir,
            args.no_clean,
            args.no_rerun,
            include_files,
            args.route_chan_width,
            prior_index.get((design, flow_name)),
            csv_path,
            order,
            flow_name in vcheck_flows,
            args.verilator_vectors,
            args.verilator_seed,
        )
        for design in designs
        for flow_name in selected_flows
    ]

    print(f"arch:    {arch_file}")
    print(f"bench:   {bench_dir}")
    print(f"designs: {', '.join(designs)}")
    print(f"flows:   {', '.join(selected_flows)}")
    if vcheck_flows:
        print(f"vcheck:  {', '.join(vcheck_flows)}")
    if include_files:
        print(f"include: {', '.join(path.name for path in include_files)}")
    print(f"jobs:    {jobs} concurrent runs x 1 core each")
    print(f"outdir:  {out_dir}")
    print(f"csv:     {csv_path}")
    if not args.watch:
        print(
            f"watch:   python3 mosaic/scripts/watch_compare.py --dir {out_dir}"
        )
    print(
        f"launching {len(designs)} designs x {len(selected_flows)} flows "
        f"= {len(tasks)} runs"
    )
    print()

    watch_proc = None
    if args.watch:
        watch_proc = start_watch(out_dir, max(0.2, args.watch_interval))

    try:
        rows = run_pool(tasks, jobs, csv_path, order, quiet=args.watch)
    except KeyboardInterrupt:
        stop_watch(watch_proc)
        print("\ninterrupted.")
        return 130
    finally:
        stop_watch(watch_proc)

    # live upserts already wrote each row; rewrite once in stable launch order.
    for row in rows:
        upsert_csv_row(csv_path, row, order=order)
    write_csv(csv_path, load_csv_rows(csv_path), order=order)
    ok = sum(1 for row in rows if row.get("success"))
    fail = len(rows) - ok
    print()
    print(f"done: {ok} ok, {fail} failed  ->  {csv_path}")
    return 0 if fail == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
