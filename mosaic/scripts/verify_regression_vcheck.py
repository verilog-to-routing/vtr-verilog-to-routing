#!/usr/bin/env python3
"""Require passing mosaic verilator random-check logs for regression suites."""
# used by R: Mosaic_regression after run_reg_test.py. does not resynthesize.
#
#   python3 mosaic/scripts/verify_regression_vcheck.py
#   python3 mosaic/scripts/verify_regression_vcheck.py --suite vtr_reg_basic_mosaic

from __future__ import annotations

import argparse
import sys
from pathlib import Path
from typing import Dict, List, Optional

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parents[1]
VTR_FLOW = REPO_ROOT / "vtr_flow"
TASKS_ROOT = VTR_FLOW / "tasks"
DEFAULT_SUITE = "vtr_reg_basic_mosaic"
PASS_NEEDLE = "vectors matched across rtl"


def parse_task_config(config_path: Path) -> List[Dict[str, Path]]:
    """Parse one VTR task config.txt into circuit/arch cases."""
    circuits_dir = "benchmarks/verilog"
    archs_dir = "arch"
    circuits: List[str] = []
    archs: List[str] = []
    for raw in config_path.read_text(encoding="utf-8", errors="replace").splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line or "=" not in line:
            continue
        key, value = line.split("=", 1)
        key = key.strip()
        value = value.strip()
        if key == "circuits_dir":
            circuits_dir = value
        elif key == "archs_dir":
            archs_dir = value
        elif key == "circuit_list_add":
            circuits.append(value)
        elif key == "arch_list_add":
            archs.append(value)
    if not circuits or not archs:
        raise ValueError("task config missing circuits or archs: {}".format(config_path))
    cases = []
    for arch_name in archs:
        arch_path = VTR_FLOW / archs_dir / arch_name
        for circuit_name in circuits:
            circuit_path = VTR_FLOW / circuits_dir / circuit_name
            cases.append(
                {
                    "circuit": circuit_path,
                    "arch": arch_path,
                    "label": "{}/{}".format(circuit_path.stem, arch_path.stem),
                }
            )
    return cases


def load_suite(suite_name: str) -> tuple[List[Path], List[Dict[str, Path]]]:
    """Return task dirs and circuit/arch cases from a regression suite task_list.txt."""
    suite_dir = TASKS_ROOT / "regression_tests" / suite_name
    task_list = suite_dir / "task_list.txt"
    if not task_list.is_file():
        raise FileNotFoundError("missing suite task_list: {}".format(task_list))
    task_dirs: List[Path] = []
    cases: List[Dict[str, Path]] = []
    for raw in task_list.read_text(encoding="utf-8", errors="replace").splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        task_dir = TASKS_ROOT / line
        task_dirs.append(task_dir)
        config_path = task_dir / "config" / "config.txt"
        if not config_path.is_file():
            raise FileNotFoundError("missing task config: {}".format(config_path))
        cases.extend(parse_task_config(config_path))
    return task_dirs, cases


def find_latest_run_dir(task_dir: Path) -> Optional[Path]:
    """Return the highest runNNN directory under a VTR task."""
    runs = sorted(path for path in task_dir.glob("run[0-9][0-9][0-9]") if path.is_dir())
    if not runs:
        return None
    return runs[-1]


def vcheck_log_passed(log_path: Path) -> bool:
    """Return true when the random-check log contains the rtl/post-synth pass line."""
    if not log_path.is_file():
        return False
    text = log_path.read_text(encoding="utf-8", errors="replace")
    return PASS_NEEDLE in text


def case_matches_log(case: Dict[str, Path], log_path: Path, run_dir: Path) -> bool:
    """Return true when this log sits under the case's arch and circuit directories."""
    try:
        rel = log_path.relative_to(run_dir).as_posix()
    except ValueError:
        return False
    return case["circuit"].name in rel and case["arch"].name in rel


def verify_latest_suite(suite_name: str) -> int:
    """Require a passing mosaic vcheck log for every suite case in the latest run."""
    task_dirs, cases = load_suite(suite_name)
    failures: List[str] = []
    logs_by_label = {case["label"]: [] for case in cases}
    all_logs: List[Path] = []
    for task_dir in task_dirs:
        if not task_dir.is_dir():
            failures.append("missing task dir {}".format(task_dir))
            continue
        latest = find_latest_run_dir(task_dir)
        if latest is None:
            failures.append("no runNNN dir under {}".format(task_dir))
            continue
        print("latest: {}".format(latest), flush=True)
        for log_path in latest.rglob("verilator_random_check.out"):
            all_logs.append(log_path)
            for case in cases:
                if case_matches_log(case, log_path, latest):
                    logs_by_label[case["label"]].append(log_path)
    if not all_logs:
        print(
            "FAIL: no verilator_random_check.out under latest mosaic regression runs",
            file=sys.stderr,
        )
        return 1
    for log_path in all_logs:
        passed = vcheck_log_passed(log_path)
        status = "pass" if passed else "fail"
        print("{} {}".format(status, log_path), flush=True)
        if not passed:
            failures.append("vcheck not green: {}".format(log_path))
    for case in cases:
        if not logs_by_label[case["label"]]:
            failures.append("missing vcheck log for {}".format(case["label"]))
    if failures:
        print("FAIL: {}".format("; ".join(failures)), file=sys.stderr)
        return 1
    print(
        "PASS: mosaic rtl vs post-synth random-checks matched "
        "({} cases, {} logs)".format(len(cases), len(all_logs))
    )
    return 0


def main(argv=None) -> int:
    """Parse arguments and run the regression verification."""
    parser = argparse.ArgumentParser(
        description="require passing mosaic vcheck logs in the latest regression run dirs"
    )
    parser.add_argument(
        "--suite",
        default=DEFAULT_SUITE,
        help="regression suite under vtr_flow/tasks/regression_tests "
        "(default {})".format(DEFAULT_SUITE),
    )
    args = parser.parse_args(argv)
    try:
        return verify_latest_suite(args.suite)
    except (FileNotFoundError, ValueError) as exc:
        print("error: {}".format(exc), file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
