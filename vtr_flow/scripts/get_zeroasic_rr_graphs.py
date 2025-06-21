#!/usr/bin/env python3
"""
    Script to get the Zero ASIC RR graphs.
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
            Extracts the RR graphs for the Zero ASIC architectures which are
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

    # A list of the zipped RR graphs to uncompress.
    zipped_rr_graphs = [
        f"{args.vtr_flow_dir}/arch/zeroasic/z1000/z1000_rr_graph.zip",
    ]

    # For each zipped RR graph, unzip it into its directory.
    for zipped_rr_graph in zipped_rr_graphs:
        # Check that the file exists.
        if not os.path.exists(zipped_rr_graph):
            print(f"Error: Unable to find zipped RR graph: {zipped_rr_graph}")
            sys.exit(1)

        # Unzip it.
        print(f"Unzipping RR graph: {zipped_rr_graph}")
        subprocess.call(
            f"unzip {zipped_rr_graph} -d {os.path.dirname(zipped_rr_graph)}",
            shell=True,
        )

    sys.exit(0)


if __name__ == "__main__":
    main()
