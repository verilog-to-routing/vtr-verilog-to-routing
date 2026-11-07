#!/usr/bin/env python3
"""
Summarize how the golden QoR results of regression suites changed between two
git revisions as GitHub-flavoured markdown tables, one table per suite.

Each table has one row per task plus a GEOMEAN row. The metric columns hold the
geometric mean, over all circuits/parameter variants of the task, of
new / old for that metric. Rows that are missing on either side, or whose value
is not a positive number on both sides (e.g. -1 for a failed run), are skipped.

Typical use when a PR changes QoR (e.g. a Yosys or ABC bump):

    # compare the working tree against master for the suites CI runs on PRs
    ./vtr_flow/scripts/golden_qor_diff.py --base master \\
        vtr_reg_basic vtr_reg_strong vtr_reg_parallel

    # compare two commits
    ./vtr_flow/scripts/golden_qor_diff.py --base 4aa4420851 --head HEAD vtr_reg_nightly_test1

Paste the output into the pull request description.
"""

import argparse
import csv
import io
import math
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
TASKS_DIR = REPO_ROOT / "vtr_flow" / "tasks"

DEFAULT_METRICS = [
    "num_pre_packed_blocks",
    "num_clb",
    "min_chan_width",
    "routed_wirelength",
    "critical_path_delay",
]

KEY_COLUMNS = ("arch", "circuit", "script_params")


def parse_args(argv):
    """Parse the command line."""
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "suites",
        nargs="+",
        help="Regression suites (e.g. vtr_reg_strong) or task paths relative to "
        "vtr_flow/tasks (e.g. regression_tests/vtr_reg_strong/strong_timing)",
    )
    parser.add_argument(
        "--base",
        default="master",
        help="Git revision holding the old golden results (default: %(default)s)",
    )
    parser.add_argument(
        "--head",
        default=None,
        help="Git revision holding the new golden results (default: the working tree)",
    )
    parser.add_argument(
        "--metrics",
        nargs="+",
        default=DEFAULT_METRICS,
        help="Metrics to summarize (default: %(default)s)",
    )
    parser.add_argument(
        "--per-circuit",
        action="store_true",
        help="Also print one row per circuit instead of only per-task geomeans",
    )
    parser.add_argument(
        "-o",
        "--output",
        default=None,
        help="Write the markdown to this file instead of stdout",
    )
    return parser.parse_args(argv)


def list_tasks(suite):
    """Return the task paths (relative to vtr_flow/tasks) that make up a suite."""
    direct = TASKS_DIR / suite
    if (direct / "config" / "golden_results.txt").is_file():
        return [suite]
    task_list = TASKS_DIR / "regression_tests" / suite / "task_list.txt"
    if not task_list.is_file():
        raise FileNotFoundError(f"{suite}: neither a task nor a suite with a task_list.txt")
    tasks = []
    for line in task_list.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if line and not line.startswith("#"):
            tasks.append(line.rstrip("/"))
    return tasks


def golden_path(task):
    """Repository-relative path of a task's golden results."""
    return Path("vtr_flow") / "tasks" / task / "config" / "golden_results.txt"


def read_golden(task, revision):
    """
    Load golden results as {(arch, circuit, script_params): {metric: value}}.
    revision None reads the working tree; otherwise the file is taken from git.
    """
    path = golden_path(task)
    if revision is None:
        full_path = REPO_ROOT / path
        if not full_path.is_file():
            return {}
        text = full_path.read_text(encoding="utf-8")
    else:
        result = subprocess.run(
            ["git", "show", f"{revision}:{path.as_posix()}"],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            check=False,
        )
        if result.returncode != 0:
            return {}
        text = result.stdout

    rows = {}
    for raw in csv.DictReader(io.StringIO(text), delimiter="\t"):
        row = {(k or "").strip(): (v or "").strip() for k, v in raw.items()}
        rows[tuple(row.get(k, "") for k in KEY_COLUMNS)] = row
    return rows


def ratio(old_row, new_row, metric):
    """new/old for a metric, or None if either side is unusable."""
    try:
        old = float(old_row.get(metric, ""))
        new = float(new_row.get(metric, ""))
    except ValueError:
        return None
    if not (math.isfinite(old) and math.isfinite(new)) or old <= 0 or new <= 0:
        return None
    return new / old


def geomean(values):
    """Geometric mean of a non-empty list, else None."""
    if not values:
        return None
    return math.exp(sum(math.log(v) for v in values) / len(values))


def fmt(value):
    """Format a ratio for the table."""
    return "-" if value is None else f"{value:.3f}"


def task_label(task):
    """Short task name: strip the leading regression_tests/<suite>/ prefix."""
    parts = task.split("/")
    return "/".join(parts[2:]) if len(parts) > 2 else task


def compare_task(task, args):
    """
    Compare one task's old and new goldens.
    Returns (number of matched rows, {metric: [ratios]}, per-circuit table lines).
    """
    old_rows = read_golden(task, args.base)
    new_rows = read_golden(task, args.head)
    keys = sorted(set(old_rows) & set(new_rows))
    ratios = {m: [] for m in args.metrics}
    lines = []
    for key in keys:
        row_ratios = {m: ratio(old_rows[key], new_rows[key], m) for m in args.metrics}
        for metric, value in row_ratios.items():
            if value is not None:
                ratios[metric].append(value)
        if args.per_circuit:
            label = f"{task_label(task)} / {key[1]}"
            if key[2] not in ("", "common"):
                label += f" ({key[2]})"
            cells = " | ".join(fmt(row_ratios[m]) for m in args.metrics)
            lines.append(f"| {label} | 1 | {cells} |")
    return len(keys), ratios, lines


def suite_table(suite, tasks, args):
    """Build the markdown table lines for one suite."""
    metrics = args.metrics
    lines = [f"### {suite}", ""]
    lines.append("| task | circuits | " + " | ".join(metrics) + " |")
    lines.append("|---|---:|" + "|".join("---:" for _ in metrics) + "|")

    suite_ratios = {m: [] for m in metrics}
    compared = 0
    for task in tasks:
        matched, task_ratios, circuit_lines = compare_task(task, args)
        if matched == 0:
            continue
        for metric in metrics:
            suite_ratios[metric] += task_ratios[metric]
        compared += matched
        lines += circuit_lines
        cells = " | ".join(fmt(geomean(task_ratios[m])) for m in metrics)
        lines.append(f"| **{task_label(task)}** | {matched} | {cells} |")

    cells = " | ".join(fmt(geomean(suite_ratios[m])) for m in metrics)
    lines.append(f"| **GEOMEAN** | {compared} | {cells} |")
    lines.append("")
    return lines


def main(argv=None):
    """Entry point."""
    args = parse_args(argv)
    head_name = args.head if args.head else "working tree"
    lines = [
        f"QoR of golden results, {head_name} relative to `{args.base}` "
        "(geometric mean of new/old; 1.000 = unchanged, <1 = smaller/faster).",
        "",
    ]
    for suite in args.suites:
        lines += suite_table(suite, list_tasks(suite), args)

    text = "\n".join(lines)
    if args.output:
        Path(args.output).write_text(text + "\n", encoding="utf-8")
    else:
        print(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
