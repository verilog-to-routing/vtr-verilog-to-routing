#!/usr/bin/env python3
# require a passing mosaic verilator_random_check.out for every case in the
# latest vtr_reg_basic_mosaic (or other suite) run dirs.
#
# used by R: Mosaic_regression after run_reg_test.py. does not resynthesize.
#
#   python3 mosaic/scripts/verify_regression_vcheck.py
#   python3 mosaic/scripts/verify_regression_vcheck.py --suite vtr_reg_basic_mosaic

from __future__ import annotations

import argparse
import sys
from pathlib import Path
from typing import Dict, List, Optional

scriptDir = Path(__file__).resolve().parent
repoRoot = scriptDir.parents[1]
vtrFlow = repoRoot / "vtr_flow"
tasksRoot = vtrFlow / "tasks"
defaultSuite = "vtr_reg_basic_mosaic"
passNeedle = "vectors matched across rtl"


def parseTaskConfig(configPath: Path) -> List[Dict[str, Path]]:
    # parse one vtr task config.txt into circuit/arch cases
    circuitsDir = "benchmarks/verilog"
    archsDir = "arch"
    circuits: List[str] = []
    archs: List[str] = []
    for raw in configPath.read_text(encoding="utf-8", errors="replace").splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line or "=" not in line:
            continue
        key, value = line.split("=", 1)
        key = key.strip()
        value = value.strip()
        if key == "circuits_dir":
            circuitsDir = value
        elif key == "archs_dir":
            archsDir = value
        elif key == "circuit_list_add":
            circuits.append(value)
        elif key == "arch_list_add":
            archs.append(value)
    if not circuits or not archs:
        raise ValueError("task config missing circuits or archs: {}".format(configPath))
    cases = []
    for archName in archs:
        archPath = vtrFlow / archsDir / archName
        for circuitName in circuits:
            circuitPath = vtrFlow / circuitsDir / circuitName
            cases.append(
                {
                    "circuit": circuitPath,
                    "arch": archPath,
                    "label": "{}/{}".format(circuitPath.stem, archPath.stem),
                }
            )
    return cases


def loadSuite(suiteName: str) -> tuple[List[Path], List[Dict[str, Path]]]:
    # task dirs and circuit/arch cases from a regression suite task_list.txt
    suiteDir = tasksRoot / "regression_tests" / suiteName
    taskList = suiteDir / "task_list.txt"
    if not taskList.is_file():
        raise FileNotFoundError("missing suite task_list: {}".format(taskList))
    taskDirs: List[Path] = []
    cases: List[Dict[str, Path]] = []
    for raw in taskList.read_text(encoding="utf-8", errors="replace").splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        taskDir = tasksRoot / line
        taskDirs.append(taskDir)
        configPath = taskDir / "config" / "config.txt"
        if not configPath.is_file():
            raise FileNotFoundError("missing task config: {}".format(configPath))
        cases.extend(parseTaskConfig(configPath))
    return taskDirs, cases


def findLatestRunDir(taskDir: Path) -> Optional[Path]:
    # highest runNNN directory under a vtr task
    runs = sorted(path for path in taskDir.glob("run[0-9][0-9][0-9]") if path.is_dir())
    if not runs:
        return None
    return runs[-1]


def vcheckLogPassed(logPath: Path) -> bool:
    # true when the random-check log contains the rtl/post-synth pass line
    if not logPath.is_file():
        return False
    text = logPath.read_text(encoding="utf-8", errors="replace")
    return passNeedle in text


def caseMatchesLog(case: Dict[str, Path], logPath: Path, runDir: Path) -> bool:
    # true when this log sits under the case's arch and circuit directories
    try:
        rel = logPath.relative_to(runDir).as_posix()
    except ValueError:
        return False
    return case["circuit"].name in rel and case["arch"].name in rel


def verifyLatestSuite(suiteName: str) -> int:
    # require a passing mosaic vcheck log for every suite case in the latest run
    taskDirs, cases = loadSuite(suiteName)
    failures: List[str] = []
    logsByLabel = {case["label"]: [] for case in cases}
    allLogs: List[Path] = []
    for taskDir in taskDirs:
        if not taskDir.is_dir():
            failures.append("missing task dir {}".format(taskDir))
            continue
        latest = findLatestRunDir(taskDir)
        if latest is None:
            failures.append("no runNNN dir under {}".format(taskDir))
            continue
        print("latest: {}".format(latest), flush=True)
        for logPath in latest.rglob("verilator_random_check.out"):
            allLogs.append(logPath)
            for case in cases:
                if caseMatchesLog(case, logPath, latest):
                    logsByLabel[case["label"]].append(logPath)
    if not allLogs:
        print(
            "FAIL: no verilator_random_check.out under latest mosaic regression runs",
            file=sys.stderr,
        )
        return 1
    for logPath in allLogs:
        passed = vcheckLogPassed(logPath)
        status = "pass" if passed else "fail"
        print("{} {}".format(status, logPath), flush=True)
        if not passed:
            failures.append("vcheck not green: {}".format(logPath))
    for case in cases:
        if not logsByLabel[case["label"]]:
            failures.append("missing vcheck log for {}".format(case["label"]))
    if failures:
        print("FAIL: {}".format("; ".join(failures)), file=sys.stderr)
        return 1
    print(
        "PASS: mosaic rtl vs post-synth random-checks matched "
        "({} cases, {} logs)".format(len(cases), len(allLogs))
    )
    return 0


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(
        description="require passing mosaic vcheck logs in the latest regression run dirs"
    )
    parser.add_argument(
        "--suite",
        default=defaultSuite,
        help="regression suite under vtr_flow/tasks/regression_tests "
        "(default {})".format(defaultSuite),
    )
    args = parser.parse_args(argv)
    try:
        return verifyLatestSuite(args.suite)
    except (FileNotFoundError, ValueError) as exc:
        print("error: {}".format(exc), file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
