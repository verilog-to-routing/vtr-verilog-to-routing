#!/usr/bin/python3

"""
This script runs pylint on all of the paths specified by 'paths_to_lint'.
It prints the output of pylint for each path.

The returncode is the number of files that fail pylint.
"""

import pathlib
import sys
import subprocess
import argparse

repo_path = pathlib.Path(__file__).parent.parent.absolute()

################################################################################
################################# Paths to lint ################################
################################################################################

# List of all paths to search for Python files, and whether the search
# should be recursive.
paths_to_lint = [
    (repo_path, False),
    (repo_path / "dev", True),
    (repo_path / "ODIN_II", True),
    (repo_path / "ace2", True),
    (repo_path / "doc", True),
    (repo_path / "vpr", True),
    (repo_path / "vtr_flow", True),
]

# These python files existed before the linter.
# At some point they should be cleaned up and removed from this list
grandfathered_files = [
    repo_path / "sweep_build_configs.py",
    repo_path / "dev/vtr_gdb_pretty_printers.py",
    repo_path / "dev/submit_slurm.py",
    repo_path / "dev/code_format_fixup.py",
    repo_path / "dev/annealing_curve_plotter.py",
    repo_path / "dev/autoformat.py",
    repo_path / "dev/vpr_animate.py",
    repo_path / "dev/external_subtrees.py",
    repo_path / "ODIN_II/usefull_tools/restore_blackboxed_latches_from_blif_file.py",
    repo_path / "ODIN_II/regression_test/parse_result/parse_result.py",
    repo_path / "ODIN_II/regression_test/parse_result/conf/hooks.py",
    repo_path / "ODIN_II/regression_test/tools/parse_odin_result.py",
    repo_path / "ODIN_II/regression_test/tools/odin_script_util.py",
    repo_path / "ODIN_II/regression_test/tools/ODIN_CONFIG.py",
    repo_path / "ODIN_II/regression_test/tools/synth_using_quartus.py",
    repo_path / "ODIN_II/regression_test/tools/odin_config_maker.py",
    repo_path / "ODIN_II/regression_test/tools/synth_using_vl2mv.py",
    repo_path / "ODIN_II/regression_test/tools/synth_using_odin.py",
    repo_path / "ODIN_II/regression_test/tools/asr_vector_maker.py",
    repo_path / "ODIN_II/regression_test/tools/8_bit_arithmetic_power_output.py",
    repo_path / "ODIN_II/regression_test/tools/8_bit_input.py",
    repo_path / "ace2/scripts/extract_clk_from_blif.py",
    repo_path / "doc/src/vtr_version.py",
    repo_path / "doc/src/conf.py",
    repo_path / "doc/_exts/rrgraphdomain/__init__.py",
    repo_path / "doc/_exts/sdcdomain/__init__.py",
    repo_path / "doc/_exts/archdomain/__init__.py",
    repo_path / "vpr/scripts/compare_timing_reports.py",
    repo_path / "vpr/scripts/profile/util.py",
    repo_path / "vpr/scripts/profile/parse_and_plot_detailed.py",
    repo_path / "vtr_flow/scripts/upgrade_arch.py",
    repo_path / "vtr_flow/scripts/download_titan.py",
    repo_path / "vtr_flow/scripts/ispd2vtr.py",
    repo_path / "vtr_flow/scripts/download_ispd.py",
    repo_path / "vtr_flow/scripts/qor_compare.py",
    repo_path / "vtr_flow/scripts/blif_splicer.py",
    repo_path / "vtr_flow/scripts/benchtracker/interface_db.py",
    repo_path / "vtr_flow/scripts/benchtracker/server_db.py",
    repo_path / "vtr_flow/scripts/benchtracker/util.py",
    repo_path / "vtr_flow/scripts/benchtracker/plotter-offline.py",
    repo_path / "vtr_flow/scripts/benchtracker/populate_db.py",
    repo_path / "vtr_flow/scripts/benchtracker/flask_cors/core.py",
    repo_path / "vtr_flow/scripts/benchtracker/flask_cors/version.py",
    repo_path / "vtr_flow/scripts/benchtracker/flask_cors/six.py",
    repo_path / "vtr_flow/scripts/benchtracker/flask_cors/decorator.py",
    repo_path / "vtr_flow/scripts/benchtracker/flask_cors/extension.py",
    repo_path / "vtr_flow/scripts/python_libs/utils.py",
    repo_path / "vtr_flow/scripts/arch_gen/arch_gen.py",
    repo_path / "vtr_flow/scripts/spice/run_spice.py",
]

################################################################################
################################################################################
################################################################################


class TermColor:
    """ Terminal codes for printing in color """

    # pylint: disable=too-few-public-methods

    PURPLE = "\033[95m"
    BLUE = "\033[94m"
    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    RED = "\033[91m"
    END = "\033[0m"
    BOLD = "\033[1m"
    UNDERLINE = "\033[4m"


def error(*msg, returncode=-1):
    """ Print an error message and exit program """

    print(TermColor.RED + "ERROR:", " ".join(str(item) for item in msg), TermColor.END)
    sys.exit(returncode)


def expand_paths():
    """ Build a list of all python files to process by going through 'paths_to_lint' """

    paths = []
    for (path, is_recursive) in paths_to_lint:
        # Make sure all hard-coded paths point to .py files
        if path.is_file():
            if path.suffix.lower() != ".py":
                print(path, "does note have extension '.py'")
                sys.exit(-1)
            paths.append(path)

        # If path is a directory, search for .py files
        elif path.is_dir():
            if is_recursive:
                glob_str = "**/*.py"
            else:
                glob_str = "*.py"
            for glob_path in path.glob(glob_str):
                paths.append(glob_path)

        # Non-existant paths, and unhanlded file types error
        elif not path.exists():
            error("Non-existant path:", path)
        else:
            error("Unhandled path:", path)
    return paths


def main():
    """
    Run pylint on specified files.
    """

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--check_grandfathered",
        action="store_true",
        help="Also check grandfathered files for lint errors.",
    )
    args = parser.parse_args()

    # Expand all paths
    paths = expand_paths()
    print(TermColor.BLUE + "Linting", len(paths), "python files.", TermColor.END)

    # Lint files
    num_error_files = 0
    for path in paths:
        relpath_str = str(path.relative_to(repo_path))

        if (path in grandfathered_files) and not args.check_grandfathered:
            print(
                TermColor.YELLOW + relpath_str, "skipped (grandfathered)", TermColor.END,
            )
            continue

        # Pylint checks to ignore
        ignore_list = []

        # Ignore function argument indenting, which is currently incompabile with black
        # https://github.com/psf/black/issues/48
        ignore_list.append("C0330")

        # Build pylint command
        cmd = ["pylint", path, "-s", "n"]
        if ignore_list:
            cmd.append("--disable=" + ",".join(ignore_list))

        # Run pylint and check output
        process = subprocess.run(cmd, check=False, stdout=subprocess.PIPE)
        if process.returncode:
            print(TermColor.RED + relpath_str, "has lint errors", TermColor.END)
            print(process.stdout.decode().strip())
            num_error_files += 1
        else:
            print(TermColor.GREEN + relpath_str, "passed", TermColor.END)

    if num_error_files:
        error(num_error_files, "file(s) failed lint test.", returncode=num_error_files)
    else:
        print(TermColor.GREEN + "Lint passed.", TermColor.END)


if __name__ == "__main__":
    main()
