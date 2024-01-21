#!/usr/bin/env python3

"""
    Module for downloading and extracting NoC MLP benchmarks
"""

import sys
import os
import argparse
import urllib.request
import urllib.parse
import urllib.error
import math
import textwrap
import tarfile
import tempfile
import shutil
import errno

class ExtractionError(Exception):
    """
    Raised when extracting the downlaoded file fails
    """


URL_MIRRORS = {"eecg": "https://www.eecg.utoronto.ca/~vaughn/titan/"}


def parse_args():
    """
    Parses command line arguments
    """
    description = textwrap.dedent(
        """
                    Download and extract a MLP NoC benchmarks into a
                    VTR-style directory structure.

                    If a previous matching tar.gz file is found
                    does nothing (unless --force is specified).
                  """
    )
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter,
                                     description=description)

    parser.add_argument(
        "--vtr_flow_dir",
        required=True,
        help="The 'vtr_flow' directory under the VTR tree. "
        "If specified this will extract benchmarks files, "
        "placing them under vtr_flow/benchmarks/noc/Large_Designs/MLP ",
    )
    parser.add_argument(
        "--force",
        default=False,
        action="store_true",
        help="Run extraction step even if directores etc. already exist",
    )
    parser.add_argument(
        "--full_archive",
        default=False,
        action="store_true",
        help="Download the full archive instead of just downloading the blif archive",
    )

    return parser.parse_args()


def main():
    """
    main() implementation
    """

    args = parse_args()

    try:
        if args.full_archive:
            tar_gz_filename = "MLP_Benchmark_Netlist_Files_vqm_blif" + ".tar.gz"
        else:
            tar_gz_filename = "MLP_Benchmark_Netlist_Files_blif" + ".tar.gz"

        tar_gz_url = urllib.parse.urljoin(URL_MIRRORS["eecg"], tar_gz_filename)

        if not args.force and os.path.isfile(tar_gz_filename):
            print("Found existing {} (skipping download and extraction)".format(tar_gz_filename))
        else:
            print("Downloading {}".format(tar_gz_url))
            download_url(tar_gz_filename, tar_gz_url)

            print("Extracting {}".format(tar_gz_filename))
            extract_to_vtr_flow_dir(args, tar_gz_filename)
    except ExtractionError as e:
        print("Failed to extract NoC MLP benchmarks release:", e)
        sys.exit(3)

    sys.exit(0)


def download_url(filename, url):
    """
    Downloads NoC MLP benchmarks
    """
    urllib.request.urlretrieve(url, filename, reporthook=download_progress_callback)


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


def extract_to_vtr_flow_dir(args, tar_gz_filename):
    """
    Extracts the NoC MLP benchmarks into its corresponding vtr directory
    """

    # Reference directories
    benchmarks_dir = os.path.join(args.vtr_flow_dir, "benchmarks")
    mlp_benchmarks_dir = os.path.join(benchmarks_dir, "noc/Large_Designs/MLP")

    if not args.force:
        # Check that all expected directories exist
        for directory in [args.vtr_flow_dir, benchmarks_dir, mlp_benchmarks_dir]:
            if not os.path.isdir(directory):
                raise ExtractionError("{} should be a directory".format(directory))

    # Create a temporary working directory
    tmpdir = tempfile.mkdtemp(suffix="download_NoC_MLP", dir= os.path.abspath("."))
    try:
        # Extract the contents of the .tar.gz archive directly into the destination directory
        with tarfile.open(tar_gz_filename, "r:gz") as tar:
            tar.extractall(tmpdir)
        tmp_source_blif_dir = os.path.join(tmpdir, "MLP_Benchmark_Netlist_Files")
        for root, _, files in os.walk(tmp_source_blif_dir):
            for file in files:
                source_file = os.path.join(root, file)
                relative_path = os.path.relpath(source_file, tmp_source_blif_dir)
                destination_file = os.path.join(mlp_benchmarks_dir, relative_path)
                os.makedirs(os.path.dirname(destination_file), exist_ok=True)
                shutil.copy2(source_file, destination_file)

        # Create symbolic links to blif files
        find_and_link_files(mlp_benchmarks_dir, ".blif", "blif_files")
        # Create symbolic links to traffic flow files
        find_and_link_files(mlp_benchmarks_dir, ".flows", "traffic_flow_files")

    finally:
        # Clean-up
        shutil.rmtree(tmpdir)

    print("Done")


def find_and_link_files(base_path, target_extension, link_folder_name):
    """
    Finds files with a given extension and make symbolic links to them
    """

    # Create a folder to store symbolic links
    link_folder_path = os.path.join(base_path, link_folder_name)
    os.makedirs(link_folder_path, exist_ok=True)

    # Walk through all subdirectories
    for root, _, files in os.walk(base_path):
        if root == link_folder_path:
            continue

        for file in files:
            if file.endswith(target_extension):
                # Get the full path of the file
                file_path = os.path.join(root, file)

                # Create symbolic link in the link folder
                link_name = os.path.join(link_folder_path, file)
                file_relative_path = os.path.relpath(file_path, start=link_folder_path)

                try:
                    os.symlink(file_relative_path, link_name)
                except OSError as e:
                    if e.errno == errno.EEXIST:
                        os.remove(link_name)
                        os.symlink(file_relative_path, link_name)


if __name__ == "__main__":
    main()
