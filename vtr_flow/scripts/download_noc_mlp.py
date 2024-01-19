#!/usr/bin/env python3
import sys
import os
import argparse
import urllib.parse
import urllib.request, urllib.parse, urllib.error
import urllib.request, urllib.error, urllib.parse
import math
import textwrap
import tarfile
import tempfile
import shutil


class DownloadError(Exception):
    pass


class ChecksumError(Exception):
    pass


class ExtractionError(Exception):
    pass


TITAN_URL_MIRRORS = {"eecg": "https://www.eecg.utoronto.ca/~vaughn/titan/"}


def parse_args():
    description = textwrap.dedent(
        """
                    Download and extract a MLP NoC benchmarks into a
                    VTR-style directory structure.

                    If a previous matching titan release tar.gz file is found
                    does nothing (unless --force is specified).
                  """
    )
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument(
        "--vtr_flow_dir",
        required=True,
        help="The 'vtr_flow' directory under the VTR tree. "
        "If specified this will extract the titan release, "
        "placing benchmarks under vtr_flow/benchmarks/titan ",
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

    args = parse_args()

    try:
        if args.full_archive:
            tar_gz_filename = "MLP_Benchmark_Netlist_Files_vqm_blif" + ".tar.gz"
        else:
            tar_gz_filename = "MLP_Benchmark_Netlist_Files_blif" + ".tar.gz"

        tar_gz_url = urllib.parse.urljoin(TITAN_URL_MIRRORS["eecg"], tar_gz_filename)

        if not args.force and os.path.isfile(tar_gz_filename):
            print("Found existing {} (skipping download and extraction)".format(tar_gz_filename))
        else:
            print("Downloading {}".format(tar_gz_url))
            # download_url(tar_gz_filename, tar_gz_url)

            print("Extracting {}".format(tar_gz_filename))
            extract_to_vtr_flow_dir(args, tar_gz_filename)

    except DownloadError as e:
        print("Failed to download:", e)
        sys.exit(1)
    except ExtractionError as e:
        print("Failed to extract titan release:", e)
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
    # arch_dir = os.path.join(args.vtr_flow_dir, "arch")
    benchmarks_dir = os.path.join(args.vtr_flow_dir, "benchmarks")
    mlp_benchmarks_dir = os.path.join(benchmarks_dir, "noc/Large_Designs/MLP")


    if not args.force:
        # Check that all expected directories exist
        expected_dirs = [
            args.vtr_flow_dir,
            benchmarks_dir,
            mlp_benchmarks_dir,
        ]
        for dir in expected_dirs:
            if not os.path.isdir(dir):
                raise ExtractionError("{} should be a directory".format(dir))

    # Create a temporary working directory
    tmpdir = tempfile.mkdtemp(suffix="download_NoC_MLP", dir= os.path.abspath("."))
    try:
        # Extract the contents of the .tar.gz archive directly into the destination directory
        with tarfile.open(tar_gz_filename, "r:gz") as tar:
            tar.extractall(tmpdir)
        tmp_source_blif_dir = os.path.join(tmpdir, "MLP_Benchmark_Netlist_Files")
        for root, dirs, files in os.walk(tmp_source_blif_dir):
            for file in files:
                source_file = os.path.join(root, file)
                relative_path = os.path.relpath(source_file, tmp_source_blif_dir)
                destination_file = os.path.join(mlp_benchmarks_dir, relative_path)
                print(source_file, destination_file)
                os.makedirs(os.path.dirname(destination_file), exist_ok=True)
                shutil.copy2(source_file, destination_file)   

    finally:
        # Clean-up
        shutil.rmtree(tmpdir)

    print("Done")



if __name__ == "__main__":
    main()
