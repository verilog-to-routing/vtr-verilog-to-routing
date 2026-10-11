#!/usr/bin/env python3

"""
Runs the Catch2 unit test binaries several times, each time with a different
random test-case order (seed).

Catch2 runs test cases in a random order by default. If one test case leaves
global state (e.g. g_vpr_ctx) behind, a later test case may fail, but only for
some orders. A single `make test` run therefore only catches such bugs some of
the time. Running several seeds in CI makes these bugs much more likely to show
up on the pull request that introduced them.

The tests to run are discovered from ctest, so new Catch2 unit test binaries
(named test_*) are picked up automatically, using the same working directory
and environment that `make test` uses.

On failure, this script prints the exact command needed to reproduce the
failing order locally.

Usage (from the VTR root, after building):
    ./dev/run_shuffled_unit_tests.py
    ./dev/run_shuffled_unit_tests.py --num-seeds 10
    ./dev/run_shuffled_unit_tests.py --seeds 1 2 8
    ./dev/run_shuffled_unit_tests.py -j4
"""

import argparse
import concurrent.futures
import json
import os
import pathlib
import random
import signal
import subprocess
import sys

REPO_PATH = pathlib.Path(__file__).parent.parent.absolute()

# Catch2 accepts any unsigned 32-bit seed.
MAX_SEED = 2**32 - 1


def parse_args():
    """Parses the command-line arguments."""
    parser = argparse.ArgumentParser(
        description="Run the Catch2 unit tests with several random test orders."
    )
    parser.add_argument(
        "--build-dir",
        default=str(REPO_PATH / "build"),
        help="CMake build directory (default: %(default)s)",
    )
    parser.add_argument(
        "--num-seeds",
        type=int,
        default=5,
        help="Number of random seeds to run each test binary with (default: %(default)s)",
    )
    parser.add_argument(
        "--seeds",
        type=int,
        nargs="+",
        help="Run exactly these seeds instead of picking random ones",
    )
    parser.add_argument(
        "-j",
        "--jobs",
        type=int,
        default=1,
        help="Maximum number of working directories to test in parallel. Test binaries that "
        "share a working directory always run one at a time (default: %(default)s)",
    )
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("-j/--jobs must be at least 1")
    return args


def discover_tests(build_dir):
    """
    Returns a list of (name, command, working_dir, env) for every ctest test
    whose executable is a Catch2 test binary (named test_*).
    """
    result = subprocess.run(
        ["ctest", "--show-only=json-v1"],
        cwd=build_dir,
        check=True,
        capture_output=True,
        text=True,
    )

    tests = []
    for test in json.loads(result.stdout)["tests"]:
        command = test.get("command")
        if not command or not pathlib.Path(command[0]).name.startswith("test_"):
            continue

        working_dir = build_dir
        env = dict(os.environ)
        for prop in test.get("properties", []):
            if prop["name"] == "WORKING_DIRECTORY":
                working_dir = prop["value"]
            elif prop["name"] == "ENVIRONMENT":
                for assignment in prop["value"]:
                    key, _, value = assignment.partition("=")
                    env[key] = value

        tests.append((test["name"], command, working_dir, env))

    return tests


def describe_exit(returncode):
    """Returns a human-readable description of a test binary's exit status."""
    if returncode == 0:
        return "PASS"
    if returncode < 0:
        return f"FAIL (crashed with {signal.Signals(-returncode).name})"
    return f"FAIL (exit code {returncode})"


def repro_command(command, working_dir, seed):
    """Returns a shell command, relative to the VTR root, that reproduces this run."""
    try:
        rel_dir = os.path.relpath(working_dir, REPO_PATH)
        rel_exe = os.path.relpath(command[0], working_dir)
    except ValueError:
        rel_dir, rel_exe = working_dir, command[0]

    args = " ".join(command[1:])
    return f"cd {rel_dir} && {rel_exe} {args} --order rand --rng-seed {seed}"


def run_working_dir_group(group, seeds):
    """
    Runs every test in the group (all sharing one working directory) once per
    seed, one at a time. Returns a list of (name, seed, completed_process, repro).
    """
    results = []
    for name, command, working_dir, env in group:
        for seed in seeds:
            run = subprocess.run(
                command + ["--order", "rand", "--rng-seed", str(seed)],
                cwd=working_dir,
                env=env,
                check=False,
                capture_output=True,
                text=True,
            )
            results.append((name, seed, run, repro_command(command, working_dir, seed)))
    return results


def main():
    """Runs every discovered test binary once per seed and reports failures."""
    args = parse_args()
    build_dir = str(pathlib.Path(args.build_dir).absolute())

    seeds = args.seeds or [random.randint(1, MAX_SEED) for _ in range(args.num_seeds)]
    tests = discover_tests(build_dir)
    if not tests:
        print(f"ERROR: No Catch2 unit tests found in '{build_dir}'. Did you build VTR?")
        return 1

    print(f"Running {len(tests)} unit test binaries with seeds: {seeds}\n")

    # Test binaries write output files into their working directory, so two runs
    # in the same directory would race. Only different working directories are
    # run in parallel; runs within one directory always happen one at a time.
    groups = {}
    for test in tests:
        groups.setdefault(test[2], []).append(test)

    failures = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as executor:
        # map() yields results in submission order, so the output is the same
        # regardless of the number of jobs.
        for results in executor.map(lambda g: run_working_dir_group(g, seeds), groups.values()):
            for name, seed, run, repro in results:
                print(f"{name:<20} seed {seed:<12} {describe_exit(run.returncode)}")

                if run.returncode != 0:
                    print(run.stdout[-20000:])
                    print(run.stderr[-20000:])
                    failures.append((name, seed, repro))

    if not failures:
        print("\nAll unit tests passed for every seed.")
        return 0

    print("\n" + "=" * 79)
    print("UNIT TEST FAILURES WITH RANDOM TEST ORDER")
    print("=" * 79)
    print(
        "The following tests failed with a specific test-case order. If they pass\n"
        "with other seeds, a test case is likely leaving global state behind\n"
        "(e.g. in g_vpr_ctx) that breaks a later test case. To reproduce a\n"
        "failure locally with the same order, build VTR\n"
        "(`make`) and run the command below from the root of the VTR repository:\n"
    )
    for name, seed, command in failures:
        print(f"  {name} (seed {seed}):")
        print(f"    {command}\n")
        if os.environ.get("GITHUB_ACTIONS") == "true":
            print(f"::error title={name} failed with seed {seed}::Reproduce with: {command}")

    return 1


if __name__ == "__main__":
    sys.exit(main())
