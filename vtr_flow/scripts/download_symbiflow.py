#!/usr/bin/env python3
"""
    Script to download the SymbiFlow Series-7 architectures
"""

import sys
import os
import argparse
import math
import textwrap
import fnmatch
import tempfile
import shutil
import subprocess
from urllib import request

BASE_URL = "https://storage.googleapis.com/symbiflow-arch-defs-gha/"
GCS_URL = {
    "architectures": BASE_URL + "symbiflow-xc7a50t_test-latest",
    "benchmarks": BASE_URL + "symbiflow-benchmarks-latest",
}

SYMBIFLOW_URL_MIRRORS = {"google": GCS_URL}


class ExtractionError(Exception):
    """
    Extraction error exception class
    """


def parse_args():
    """
    Parses and returns script's arguments
    """

    description = textwrap.dedent(
        """
            Download and extract a symbiflow benchmark release into a
            VTR-style directory structure.

            If a previous matching symbiflow release tar.gz file is found
            does nothing (unless --force is specified).
        """
    )
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter, description=description
    )

    parser.add_argument(
        "--vtr_flow_dir",
        required=True,
        help="The 'vtr_flow' directory under the VTR tree. "
        "If specified this will extract the symbiflow release, "
        "placing benchmarks under vtr_flow/benchmarks/symbiflow ",
    )
    parser.add_argument(
        "--force",
        default=False,
        action="store_true",
        help="Run extraction step even if directores etc. already exist",
    )

    parser.add_argument("--mirror", default="google", choices=["google"], help="Download mirror")

    parser.add_argument(
        "--upgrade_archs",
        action="store_true",
        default=True,
        help="Try to upgrade included architecture files (using the upgrade_archs.py)",
    )

    return parser.parse_args()


def main():
    """
    Main function
    """

    args = parse_args()

    try:
        urls = SYMBIFLOW_URL_MIRRORS[args.mirror]
        archs_tar_xz_url = urls["architectures"]
        benchmarks_tar_xz_url = urls["benchmarks"]

        archs_tar_xz_filename = "archs_symbiflow.tar.xz"
        benchmarks_tar_xz_filename = "benchmarks_symbiflow.tar.xz"

        print("Downloading architectures {}".format(archs_tar_xz_url))
        download_url(archs_tar_xz_filename, archs_tar_xz_url)

        print("Extracting architectures {}".format(archs_tar_xz_filename))
        symbiflow_data_dir = "share/symbiflow/arch/xc7a50t_test"
        extract_to_vtr_flow_dir(args, archs_tar_xz_filename, "arch", symbiflow_data_dir)

        print("Downloading benchmarks {}".format(benchmarks_tar_xz_url))
        download_url(benchmarks_tar_xz_filename, benchmarks_tar_xz_url)

        print("Extracting benchmarks {}".format(benchmarks_tar_xz_filename))
        extract_to_vtr_flow_dir(args, benchmarks_tar_xz_filename, "benchmarks")

    except ExtractionError as error:
        print("Failed to extract data: ", error)
        sys.exit(1)

    sys.exit(0)


def download_url(filename, url):
    """
    Downloads the symbiflow release
    """
    latest_package_url = request.urlopen(url).read().decode("utf-8")
    print("Downloading latest package:\n{}".format(latest_package_url))
    request.urlretrieve(latest_package_url, filename, reporthook=download_progress_callback)


def download_progress_callback(block_num, block_size, expected_size):
    """
    Callback for urllib.urlretrieve which prints a dot for every percent of a file downloaded
    """
    total_blocks = int(math.ceil(expected_size / block_size))
    progress_increment = int(math.ceil(total_blocks / 100))

    if block_num % progress_increment == 0:
        sys.stdout.write(".")
        sys.stdout.flush()
    if block_num * block_size >= expected_size:
        print("")


def extract_to_vtr_flow_dir(args, tar_xz_filename, destination, extract_path=""):
    """
    Extracts the 'benchmarks' directory of the symbiflow release
    into its corresponding vtr directory
    """

    # Reference directories
    dest_dir = os.path.join(args.vtr_flow_dir, destination)
    symbiflow_extract_dir = os.path.join(dest_dir, "symbiflow")

    if not args.force:
        # Check that all expected directories exist
        expected_dirs = [
            args.vtr_flow_dir,
            symbiflow_extract_dir,
        ]
        for directory in expected_dirs:
            if not os.path.isdir(directory):
                raise ExtractionError("{} should be a directory".format(directory))

    # Create a temporary working directory
    tmpdir = tempfile.mkdtemp(suffix="download_symbiflow", dir=".")

    # Extract matching files into the temporary directory
    subprocess.call(
        "tar -C {} -xf {} {}".format(tmpdir, tar_xz_filename, extract_path),
        shell=True,
    )

    # Move the extracted files to the relevant directories, SDC files first (since we
    # need to look up the BLIF name to make it match)
    for dirpath, _, filenames in os.walk(tmpdir):
        for filename in filenames:
            src_file_path = os.path.join(dirpath, filename)
            dst_file_path = None

            if fnmatch.fnmatch(src_file_path, "*/xc7a50t_test/arch.timing.xml"):
                dst_file_path = os.path.join(symbiflow_extract_dir, "arch.timing.xml")

            elif fnmatch.fnmatch(src_file_path, "*/xc7a50t_test/*.bin"):
                dst_file_path = os.path.join(symbiflow_extract_dir, filename)

            elif fnmatch.fnmatch(src_file_path, "**/*.eblif"):
                dst_file_path = os.path.join(symbiflow_extract_dir, filename)

            elif fnmatch.fnmatch(src_file_path, "**/*.sdc"):
                dst_file_path = os.path.join(symbiflow_extract_dir, "sdc", filename)

            elif fnmatch.fnmatch(src_file_path, "**/*.place"):
                dst_file_path = os.path.join(symbiflow_extract_dir, "place_constr", filename)

            if dst_file_path:
                shutil.move(src_file_path, dst_file_path)

    shutil.rmtree(tmpdir)

    print("Done")


if __name__ == "__main__":
    main()
