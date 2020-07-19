#!/usr/bin/python3

"""
This script runs pylint on all of the paths specified by 'paths_to_lint'.
It prints the output of pylint for each path.

The returncode is the number of file that fail pylint.
"""

import pathlib
import sys
import subprocess
import os

repo_path = pathlib.Path(__file__).parent.parent.absolute()
pylib_path = repo_path / "vtr_flow" / "scripts" / "python_libs"

sys.path.append(str(pylib_path))

# pylint: disable=wrong-import-position
from utils import termcolor, error

# pylint: enable=wrong-import-position

################################################################################
################################# Paths to lint ################################
# If path is a directory, all python files (recursively) will be linted
paths_to_lint = [repo_path / "dev" / "pylint_check.py"]

################################################################################
################################################################################


def main():
    """
    Run pylint on specified files.
    """
    paths = []

    # Expand all paths
    for path in paths_to_lint:
        if path.is_file():
            if path.suffix.lower() != ".py":
                print(path, "does note have extension '.py'")
                sys.exit(-1)
            paths.append(path)
        elif path.is_dir():
            for glob_path in path.glob("**/*.py"):
                paths.append(glob_path)
        elif not path.exists():
            error("Non-existant path:", path)
        else:
            error("Unhandled path:", path)

    print(termcolor.BLUE + "Linting", len(paths), "python files.", termcolor.END)

    # Lint files
    num_error_files = 0
    for path in paths:
        print(termcolor.PURPLE + "Linting", path, termcolor.END)

        env = os.environ
        if "PYTHONPATH" not in env:
            env["PYTHONPATH"] = str(pylib_path)
        else:
            env["PYTHONPATH"] = str(pylib_path) + os.pathsep + env["PYTHONPATH"]

        cmd = ["pylint", path, "-s", "n"]
        path = subprocess.run(cmd, env=env, check=False)
        if path.returncode:
            num_error_files += 1

    sys.exit(num_error_files)


if __name__ == "__main__":
    main()
