#!/usr/bin/env python3
"""
    Script to get the Flat Placement Files.
"""

import sys
import os
import argparse
import subprocess
import textwrap


def parse_args():
    """
    Parses and returns script's arguments
    """

    description = textwrap.dedent(
        """
            Extracts the flat placemnets for the Titan benchmarks which are
            stored in zip format to reduce the amount of space they take up.
        """
    )
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter, description=description
    )

    parser.add_argument(
        "--vtr_flow_dir",
        required=True,
        help="The 'vtr_flow' directory under the VTR tree.",
    )

    return parser.parse_args()


def main():
    """
    Main function
    """

    args = parse_args()

    # A list of the zipped flat placements to uncompress.
    flat_placement_files = [
        f"{args.vtr_flow_dir}/tasks/regression_tests/vtr_reg_nightly_test7/ap_reconstruction/constraints/flat_placements.zip",
    ]

    # For each zipped flat placement, unzip it into its directory.
    for flat_placement_file in flat_placement_files:
        # Check that the file exists.
        if not os.path.exists(flat_placement_file):
            print(f"Error: Unable to find zipped flat placement: {flat_placement_file}")
            sys.exit(1)

        # Unzip it.
        print(f"Unzipping Flat Placement File: {flat_placement_file}")
        subprocess.call(
            f"unzip {flat_placement_file} -d {os.path.dirname(flat_placement_file)}",
            shell=True,
        )

    sys.exit(0)


if __name__ == "__main__":
    main()
