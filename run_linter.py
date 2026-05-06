#!/usr/bin/env python3
"""
Small project wrapper around LLVM's run-clang-tidy script. Run using './run_linter.py vpr' to run linter for vpr.
This script was generated using LLMs. While we have reviewed it and tested the output for some usecases, you should
not be blindly trusting its output.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys


def parse_args() -> tuple[argparse.Namespace, list[str]]:
    parser = argparse.ArgumentParser(
        description="Run clang-tidy through LLVM's run-clang-tidy wrapper.",
        epilog="Extra arguments are forwarded to run-clang-tidy.",
    )
    parser.add_argument(
        "paths",
        nargs="*",
        help="Files or directories to lint. Example: ./run_linter.py vpr",
    )
    parser.add_argument(
        "-p",
        "--build-dir",
        default="build",
        help="Directory containing compile_commands.json.",
    )
    parser.add_argument(
        "-j",
        "--jobs",
        default=str(os.cpu_count() or 1),
        help="Number of clang-tidy instances to run in parallel.",
    )
    parser.add_argument(
        "--run-clang-tidy",
        default=os.environ.get("RUN_CLANG_TIDY", "run-clang-tidy"),
        help="LLVM run-clang-tidy executable to use.",
    )
    parser.add_argument(
        "--clang-tidy",
        default=os.environ.get("CLANG_TIDY"),
        help="clang-tidy executable to pass to run-clang-tidy.",
    )
    parser.add_argument(
        "--fix",
        action="store_true",
        help="Forward -fix to run-clang-tidy.",
    )
    return parser.parse_known_args()


def path_regex(paths: list[str], repo_root: Path) -> str | None:
    if not paths:
        return None

    patterns = []
    for path in paths:
        candidate = Path(path)
        if not candidate.is_absolute():
            candidate = repo_root / candidate
        candidate = candidate.resolve()

        if candidate.is_dir():
            patterns.append(f"{re.escape(candidate.as_posix())}(/|$)")
        else:
            patterns.append(re.escape(candidate.as_posix()))

    return f"^({'|'.join(patterns)})"


def has_forwarded_option(args: list[str], option: str) -> bool:
    return any(arg == option or arg.startswith(f"{option}=") for arg in args)


def main() -> int:
    args, forwarded_args = parse_args()
    repo_root = Path(__file__).resolve().parent
    runner = shutil.which(args.run_clang_tidy)

    if runner is None:
        print(
            f"error: run-clang-tidy executable not found: {args.run_clang_tidy}",
            file=sys.stderr,
        )
        return 2

    command = [
        runner,
        "-p",
        str((repo_root / args.build_dir).resolve()),
        "-j",
        str(args.jobs),
        "-quiet",
    ]

    if args.clang_tidy:
        command.extend(["-clang-tidy-binary", args.clang_tidy])
    if args.fix:
        command.append("-fix")
    if not has_forwarded_option(forwarded_args, "-warnings-as-errors"):
        command.extend(["-warnings-as-errors", "*"])

    filter_regex = path_regex(args.paths, repo_root)
    if filter_regex is not None:
        if not has_forwarded_option(forwarded_args, "-header-filter"):
            command.extend(["-header-filter", filter_regex])
        command.append(filter_regex)

    command.extend(forwarded_args)
    return subprocess.call(command, cwd=repo_root)


if __name__ == "__main__":
    sys.exit(main())
